/*****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * ( at your option ) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "dialogs/extensions/extensions_manager.hpp"
#include "player_controller.hpp"
#include "util/playback_seek_policy.hpp"
#include "player_controller_p.hpp"
#include "util/shared_input_item.hpp"

#include <vlc_actions.h>           /* ACTION_ID */
#include <vlc_url.h>            /* vlc_uri_decode */
#include <vlc_strings.h>        /* vlc_strfplayer */
#include <vlc_aout.h>           /* audio_output_t */
#include <vlc_es.h>
#include <vlc_cxx_helpers.hpp>
#include <vlc_vout.h>
#include <vlc_preparser.h>

#include <QFile>
#include <QDir>
#include <QSignalMapper>
#include <QThreadPool>
#ifndef QT_HAS_LIBATOMIC
#include <QReadLocker>
#include <QWriteLocker>
#endif

#include <cassert>
#include <cmath>

#define POSITION_MIN_UPDATE_INTERVAL VLC_TICK_FROM_MS(15)

//PlayerController private implementation

using SharedEsId = ::vlc::vlc_shared_data_ptr<vlc_es_id_t,
                                              &vlc_es_id_Hold,
                                              &vlc_es_id_Release>;

using SharedTitleList = ::vlc::vlc_shared_data_ptr<vlc_player_title_list,
                                                   &vlc_player_title_list_Hold,
                                                   &vlc_player_title_list_Release>;

PlayerControllerPrivate::~PlayerControllerPrivate()
{
    assert(m_smpteTimerRequestCount == 0);

    vlc_player_locker locker{m_player}; //this also locks the player
    vlc_player_vout_RemoveListener( m_player, m_player_vout_listener );
    vlc_player_aout_RemoveListener( m_player, m_player_aout_listener );
    vlc_player_RemoveListener( m_player, m_player_listener );
    vlc_player_RemoveTimer( m_player, m_player_timer );
    if (m_preparser != nullptr)
        vlc_preparser_Delete(m_preparser);
}

bool PlayerControllerPrivate::isCurrentItemSynced()
{
    /* The media can change before the UI gets updated and thus a player
     * request can be submitted on the wrong media or worse, no media at
     * all. Here d->m_currentItem is read and modified under the player
     * lock. */
    return m_currentItem.get() == vlc_player_GetCurrentMedia( m_player );
}

void PlayerControllerPrivate::refreshAudioTimingContextLocked()
{
    const auto category = vlc_player_GetAudioDelay(m_player);
    const auto *track = vlc_player_GetSelectedTrack(m_player, AUDIO_ES);
    const auto own = track ? vlc_player_GetEsIdDelay(m_player, track->es_id) : INT64_MAX;
    const auto rate = vlc_player_GetRate(m_player);
    const auto state = vlc_player_GetState(m_player);
    if ((m_observedAudioCategoryDelay && *m_observedAudioCategoryDelay != category)
        || (m_observedAudioTrackDelay && *m_observedAudioTrackDelay != own)
        || (m_observedAudioTimingRate && *m_observedAudioTimingRate != rate)
        || (m_observedAudioTimingState && *m_observedAudioTimingState != state))
        ++m_audioTimingRevision;
    m_observedAudioCategoryDelay = category;
    m_observedAudioTrackDelay = own;
    m_observedAudioTimingRate = rate;
    m_observedAudioTimingState = state;
}


void PlayerControllerPrivate::UpdateName(input_item_t* media)
{
    Q_Q(PlayerController);
    /* Update text, name and nowplaying */
    QString name;
    if (! media)
        return;

    /* Try to get the nowplaying */
    char *format = var_InheritString( p_intf, "input-title-format" );
    char *formatted = NULL;
    if (format != NULL)
    {
        vlc_player_Lock( m_player );
        formatted = vlc_strfplayer( m_player, media, format );
        vlc_player_Unlock( m_player );
        free( format );
        if( formatted != NULL )
        {
            name = qfu(formatted);
            free( formatted );
        }
    }

    /* If we have Nothing */
    if( name.simplified().isEmpty() )
    {
        char *uri = input_item_GetURI( media );
        char *file = uri ? strrchr( uri, '/' ) : NULL;
        if( file != NULL )
        {
            vlc_uri_decode( ++file );
            name = qfu(file);
        }
        else
            name = qfu(uri);
        free( uri );
    }

    name = name.trimmed();

    if( m_name != name )
    {
        m_name = std::move(name);
        // Property bindings read the getter when notified, not the signal argument.
        emit q->nameChanged( m_name );
    }
}


void PlayerControllerPrivate::UpdateArt(input_item_t *p_item)
{
    Q_Q(PlayerController);
    if (! p_item)
        return;

    QString url = PlayerController::decodeArtURL( p_item );

    /* the art hasn't changed, no need to update */
    if(m_artUrl == url)
        return;

    /* Update Art meta */
    m_artUrl = std::move(url);
    emit q->artChanged( m_artUrl );
}

void PlayerControllerPrivate::UpdateStats( const input_stats_t& stats )
{
    Q_Q(PlayerController);
    emit q->statisticsUpdated( stats );
}

void PlayerControllerPrivate::UpdateProgram(enum vlc_player_list_action action, const struct vlc_player_program* prgm)
{
    Q_Q(PlayerController);
    m_programList.updatePrograms( action, prgm );

    bool hasPrograms = (m_programList.getCount() > 1);

    if (m_hasPrograms != hasPrograms)
    {
        m_hasPrograms = hasPrograms;

        emit q->hasProgramsChanged(hasPrograms);
    }

    emit q->isEncryptedChanged( prgm->scrambled );
}

void PlayerControllerPrivate::UpdateTrackSelection(vlc_es_id_t *trackid, bool selected)
{
    if (trackid == NULL)
        return;
    es_format_category_e cat = vlc_es_id_GetCat(trackid);
    TrackListModel* tracklist;
    switch (cat) {
    case VIDEO_ES: tracklist = &m_videoTracks; break;
    case AUDIO_ES: tracklist = &m_audioTracks; break;
    case SPU_ES: tracklist = &m_subtitleTracks; break;
    default: return;
    }
    tracklist->updateTrackSelection(trackid, selected);
}

void PlayerControllerPrivate::UpdateMeta( input_item_t *p_item )
{
    Q_Q(PlayerController);

    {
        vlc_mutex_locker locker(&p_item->lock);

        if (p_item->p_meta)
        {
            m_title   = vlc_meta_Get(p_item->p_meta, vlc_meta_Title);
            m_artist  = vlc_meta_Get(p_item->p_meta, vlc_meta_Artist);
            m_album   = vlc_meta_Get(p_item->p_meta, vlc_meta_Album);
            m_artwork = vlc_meta_Get(p_item->p_meta, vlc_meta_ArtworkURL);

            /* Capture the raw sylt-data buffer; the entry list is built on
             * first read so we don't pay for it when lyrics are never shown.
             * Later meta refreshes may not carry sylt-data again, so keep the
             * cached buffer until the current media changes. */
            const char *psz_sylt_data = vlc_meta_GetExtra( p_item->p_meta, "sylt-data" );
            if (!psz_sylt_data)
                psz_sylt_data = vlc_meta_GetExtra( p_item->p_meta, "uslt-data" );

            if (psz_sylt_data != NULL)
            {
                QByteArray newRaw( psz_sylt_data );
                const auto hash = qHash(newRaw);
                if (!m_syltRaw || hash != m_syltRaw->second)
                {
                    m_syltRaw = qMakePair(std::move( newRaw ), hash);
                    m_syltLyrics.reset();
                }
            }
        }
    }

    if (m_syltRaw) {
        emit q->hasLyricsChanged(true);
        emit q->syltLyricsChanged();
    }

    emit q->currentMetaChanged( p_item  );
}

QList<TimedText>
PlayerControllerPrivate::syltLyrics() const
{
    if (!m_syltLyrics.has_value())
    {
        QList<TimedText> lyrics;

        if (m_syltRaw)
        {
            assert(!m_syltRaw->first.isEmpty());
            const QString sylt = QString::fromUtf8(m_syltRaw->first);
            m_syltRaw->first.clear();

            const auto entries = QStringView( sylt ).split( QChar( 0x1E ), Qt::SkipEmptyParts );

            for (const QStringView &entry : entries)
            {
                const qsizetype sep = entry.indexOf( QChar( 0x1F ) );
                if (sep <= 0)
                    continue;

                const vlc_tick_t time =
                    VLC_TICK_FROM_MS( entry.first( sep ).toLongLong() );
                lyrics.push_back( TimedText( entry.sliced( sep + 1 ).toString(), VLCTime( time ) ) );
            }
            m_syltLyrics = std::move( lyrics );
        }
    }
    return *m_syltLyrics;
}

void PlayerControllerPrivate::UpdateInfo( input_item_t *p_item )
{
    Q_Q(PlayerController);
    emit q->infoChanged( p_item );
}

void PlayerControllerPrivate::UpdateVouts(vout_thread_t **vouts, size_t i_vouts, enum vlc_player_vout_action action)
{
    Q_Q(PlayerController);
    bool hadVideo = m_hasVideo;

    if (action == VLC_PLAYER_VOUT_STARTED)
        ++m_vout_ref;
    else if (action == VLC_PLAYER_VOUT_STOPPED)
        --m_vout_ref;
    assert(m_vout_ref >= 0);
    m_hasVideo = (m_vout_ref > 0);

    vout_thread_t* main_vout = nullptr;
    if (m_hasVideo)
        main_vout = vouts[0];

    m_videoStereoMode.resetObject( main_vout );
    m_zoom.resetObject( main_vout );
    m_aspectRatio.resetObject( main_vout );
    m_crop.resetObject(  main_vout );
    m_fit.resetObject(  main_vout );
    m_deinterlace.resetObject( main_vout );
    m_deinterlaceMode.resetObject( main_vout );
    m_autoscale.resetObject( main_vout );
    m_videoTransform.resetOutput( main_vout );
    m_subtitleAppearance.resetOutput( main_vout );

    emit q->voutListChanged(vouts, i_vouts);
    if( hadVideo != m_hasVideo )
        emit q->hasVideoOutputChanged(m_hasVideo);
}

vlc_es_id_t *PlayerControllerPrivate::selectedSecondarySubtitle() const
{
    for (size_t i = 0; i < vlc_player_GetTrackCount(m_player, SPU_ES); ++i)
    {
        const auto *track = vlc_player_GetTrackAt(m_player, SPU_ES, i);
        if (!track->selected)
            continue;
        vlc_vout_order order = VLC_VOUT_ORDER_NONE;
        vlc_player_GetEsIdVout(m_player, track->es_id, &order);
        if (order == VLC_VOUT_ORDER_SECONDARY)
            return track->es_id;
    }
    return nullptr;
}

void PlayerControllerPrivate::RefreshSubtitleDelays()
{
    Q_Q(PlayerController);
    const auto previousCategory = m_subtitleDelay;
    const auto previousSecondary = q->getSecondarySubtitleDelay();
    const auto previousRaw = m_secondarySubtitleDelay;
    const auto previousId = m_secondarySpuEsId;
    {
        vlc_player_locker lock{m_player};
        if (m_currentItem && isCurrentItemSynced())
        {
            m_subtitleDelay = vlc_player_GetSubtitleDelay(m_player);
            auto *secondary = selectedSecondarySubtitle();
            m_secondarySpuEsId.reset(secondary, true);
            const auto raw = secondary ? vlc_player_GetEsIdDelay(m_player, secondary) : INT64_MAX;
            m_secondarySubtitleDelay = raw == INT64_MAX ? VLCDuration() : VLCDuration(raw);
        }
        else
        {
            // Queued events may belong to a finished input. They must neither
            // publish an old role nor carry its override into another input.
            m_subtitleDelay = VLCDuration();
            m_secondarySpuEsId.reset(nullptr);
            m_secondarySubtitleDelay = VLCDuration();
        }
    }
    if (!(previousCategory == m_subtitleDelay))
        emit q->subtitleDelayChanged(q->getSubtitleDelay());
    if (previousId != m_secondarySpuEsId)
        emit q->secondarySubtitleSelectionChanged();
    if (previousId != m_secondarySpuEsId || !(previousRaw == m_secondarySubtitleDelay)
        || !(previousSecondary == q->getSecondarySubtitleDelay()))
        emit q->secondarySubtitleDelayChanged(q->getSecondarySubtitleDelay());
}

void PlayerControllerPrivate::resetTimer(bool hasInput)
{
    Q_Q(PlayerController);
    const bool wasLive = m_player_time.live;
    m_player_time = vlc::qt::invalidPlayerTimerPoint();
    if (wasLive)
        emit q->liveChanged(false);
    seeking = false;
    m_position_timer.stop();
    m_time_timer.stop();
    m_position = 0.0;
    m_time = hasInput ? VLCTime(VLC_TICK_0) : VLCTime();
    m_length = VLCDuration();
    emit q->lengthChanged(m_length);
    q->updatePosition();
    q->updateTime(VLC_TICK_INVALID, false);
}

int PlayerControllerPrivate::interpolateTime(vlc_tick_t system_now)
{
    vlc_tick_t new_time;
    if (vlc::qt::interpolatePlayerTimerPoint(m_player_time, system_now,
                                            &new_time, &m_position) == VLC_SUCCESS)
    {
        m_time = new_time;
        return VLC_SUCCESS;
    }
    return VLC_EGENERIC;
}

extern "C" {

//player callbacks

static  void on_player_current_media_changed(vlc_player_t *, input_item_t *new_media, void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    ++that->m_audioTimingRevision;
    that->m_observedAudioCategoryDelay.reset();
    that->m_observedAudioTrackDelay.reset();
    that->m_observedAudioTimingRate.reset();
    that->m_observedAudioTimingState.reset();
    msg_Dbg( that->p_intf, "on_player_current_media_changed");

    if (!new_media)
    {
        that->callAsync([that] () {
            {
                vlc_player_locker lock{ that->m_player };
                that->m_currentItem.reset(nullptr);
            }
            that->RefreshSubtitleDelays();

            that->m_syltRaw.reset();
            that->m_syltLyrics.reset();
            that->m_currentLyricIndex = -1;
            emit that->q_func()->hasLyricsChanged(false);
            emit that->q_func()->syltLyricsChanged();
            emit that->q_func()->currentLyricIndexChanged(-1);
            emit that->q_func()->currentLyricChanged();
            that->resetTimer(false);
            emit that->q_func()->inputChanged(false);
        });
        return;
    }

    SharedInputItem newMediaPtr = SharedInputItem( new_media );
    that->callAsync([that,newMediaPtr] () {
        PlayerController* q = that->q_func();

        that->m_syltRaw.reset();
        that->m_syltLyrics.reset();
        that->m_currentLyricIndex = -1;
        emit q->hasLyricsChanged(false);
        emit q->syltLyricsChanged();
        emit q->currentLyricIndexChanged(-1);
        emit q->currentLyricChanged();
        that->resetTimer(true);

        that->UpdateArt( newMediaPtr.get() );
        that->UpdateMeta( newMediaPtr.get() );
        that->UpdateInfo( newMediaPtr.get() );
        that->UpdateName( newMediaPtr.get() );
        that->m_url = vlc::wrap_cptr( input_item_GetURI( newMediaPtr.get() ) ).get();

        {
            vlc_player_locker lock{ that->m_player };
            that->m_currentItem = std::move(newMediaPtr);
        }
        that->RefreshSubtitleDelays();

        that->m_canRestorePlayback = false;
        emit q->playbackRestoreQueried();

        emit q->inputChanged(true);
    });
}

static void on_player_length_changed(vlc_player_t *, vlc_tick_t length, void *data)
{
    auto *that = static_cast<PlayerControllerPrivate *>(data);
    that->callAsync([that, length] {
        PlayerController *q = that->q_func();
        const auto duration = length > 0 ? VLCDuration(length) : VLCDuration();
        if (that->m_length == duration)
            return;
        // A paused audio input can know its length before an output clock point.
        that->m_length = duration;
        emit q->lengthChanged(that->m_length);
        q->updatePosition();
        q->updateTime(VLC_TICK_INVALID, false);
    });
}

static void on_player_state_changed(vlc_player_t *, enum vlc_player_state state, void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    msg_Dbg( that->p_intf, "on_player_state_changed");

    const bool timingChanged = that->m_observedAudioTimingState
        && *that->m_observedAudioTimingState != state;
    if (timingChanged)
        ++that->m_audioTimingRevision;
    that->m_observedAudioTimingState = state;
    if (timingChanged)
        that->callAsync([that] { emit that->q_func()->audioTimingContextChanged(); });

    that->callAsync([that,state] () {
        PlayerController* q = that->q_func();
        that->m_playing_status = static_cast<PlayerController::PlayingState>(state);
        switch ( state ) {
        case VLC_PLAYER_STATE_STARTED:
            msg_Dbg( that->p_intf, "on_player_state_changed VLC_PLAYER_STATE_STARTED");
            break;
        case VLC_PLAYER_STATE_PLAYING:
        {
            msg_Dbg( that->p_intf, "on_player_state_changed VLC_PLAYER_STATE_PLAYING");
            SharedAOut aout = q->getAout();
            that->m_audioStereoMode.resetObject( aout.get() );
            that->m_audioMixMode.resetObject( aout.get() );
            that->m_audioVisualization.resetObject( aout.get() );
            that->m_equalizer.refresh();
            break;
        }
        case VLC_PLAYER_STATE_PAUSED:
            msg_Dbg( that->p_intf, "on_player_state_changed VLC_PLAYER_STATE_PAUSED");
            break;
        case VLC_PLAYER_STATE_STOPPING:
            msg_Dbg( that->p_intf, "on_player_state_changed VLC_PLAYER_STATE_STOPPING");
            break;
        case VLC_PLAYER_STATE_STOPPED:
        {
            msg_Dbg( that->p_intf, "on_player_state_changed VLC_PLAYER_STATE_STOPPED");

            that->m_audioStereoMode.resetObject((audio_output_t*)nullptr);
            that->m_audioMixMode.resetObject((audio_output_t*)nullptr);
            that->m_audioVisualization.resetObject((audio_output_t*)nullptr);

            /* reset the state on stop */
            that->resetTimer(false);
            that->m_rate = 1.0f;
            emit q->rateChanged( 1.0f );
            that->m_name = "";
            emit q->nameChanged( "" );
            that->m_hasChapters = false;
            emit q->hasChaptersChanged( false );
            that->m_hasTitles = false;
            emit q->hasTitlesChanged( false );
            that->m_hasMenu= false;
            emit q->hasMenuChanged( false );
            that->m_isMenu= false;
            emit q->isMenuChanged( false );
            that->m_isInteractive= false;
            emit q->isInteractiveChanged( false );
            that->m_canRestorePlayback = false;
            emit q->playbackRestoreQueried();

            that->m_teletextAvailable = false;
            emit q->teletextAvailableChanged( false );
            that->m_ABLoopState = PlayerController::ABLOOP_STATE_NONE;
            that->m_ABLoopA = VLCTime();
            that->m_ABLoopB = VLCTime();
            emit q->ABLoopStateChanged(PlayerController::ABLOOP_STATE_NONE);
            emit q->ABLoopAChanged(that->m_ABLoopA);
            emit q->ABLoopBChanged(that->m_ABLoopB);
            that->m_hasVideo = false;
            emit q->hasVideoOutputChanged( false );

            emit q->voutListChanged( NULL, 0 );

            /* Reset all InfoPanels but stats */
            that->m_artUrl = "";
            emit q->artChanged( NULL );
            emit q->artChanged( "" );
            emit q->infoChanged( NULL );
            emit q->currentMetaChanged( (input_item_t *)NULL );

            if (that->m_syltRaw || that->m_currentLyricIndex != -1)
            {
                that->m_syltRaw.reset();
                that->m_syltLyrics.reset();
                that->m_currentLyricIndex = -1;
                emit q->hasLyricsChanged( false );
                emit q->syltLyricsChanged();
                emit q->currentLyricIndexChanged( -1 );
                emit q->currentLyricChanged();
            }

            that->m_hasPrograms =false;
            emit q->hasProgramsChanged( false );

            that->m_encrypted =false;
            emit q->isEncryptedChanged( false );
            that->m_recording =false;
            emit q->recordingChanged( false );

            that->m_buffering = 0.f;
            emit q->bufferingChanged( 0.0 );

            break;
        }
        }
        emit q->playingStateChanged( that->m_playing_status );
    });
}

void on_player_error_changed(vlc_player_t *, enum vlc_player_error , void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    msg_Dbg( that->p_intf, "on_player_error_changed");
}

static void on_player_buffering(vlc_player_t *, float new_buffering, void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    msg_Dbg( that->p_intf, "on_player_buffering");
    that->callAsync([that,new_buffering](){
        that->m_buffering = new_buffering;
        emit that->q_func()->bufferingChanged( new_buffering );
    });
}

static void on_player_capabilities_changed(vlc_player_t *, int old_caps, int new_caps, void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    msg_Dbg( that->p_intf, "on_player_capabilities_changed");
    that->callAsync([that, old_caps, new_caps]() {
        PlayerController* q = that->q_func();
        that->m_capabilities = new_caps;

        bool oldSeekable = old_caps & VLC_PLAYER_CAP_SEEK;
        bool newSeekable = new_caps & VLC_PLAYER_CAP_SEEK;
        if (newSeekable != oldSeekable)
        {
            emit q->seekableChanged( newSeekable );
            emit q->displayTimeChanged();
        }

        bool oldRewindable = old_caps & VLC_PLAYER_CAP_REWIND;
        bool newRewindable = new_caps & VLC_PLAYER_CAP_REWIND;
        if (newRewindable != oldRewindable)
            emit q->rewindableChanged( newRewindable );

        bool oldPauseable = old_caps & VLC_PLAYER_CAP_PAUSE;
        bool newPauseable = new_caps & VLC_PLAYER_CAP_PAUSE;
        if (newPauseable != oldPauseable)
            emit q->pausableChanged( newPauseable );

        bool oldChangeRate = old_caps & VLC_PLAYER_CAP_CHANGE_RATE;
        bool newChangeRate = new_caps & VLC_PLAYER_CAP_CHANGE_RATE;
        if (newChangeRate != oldChangeRate)
            emit q->rateChangableChanged( newChangeRate );
    });

    //FIXME other events?
}

static void on_player_rate_changed(vlc_player_t *, float new_rate, void *data)
{
    auto *that = static_cast<PlayerControllerPrivate *>(data);
    const bool timingChanged = that->m_observedAudioTimingRate
        && *that->m_observedAudioTimingRate != new_rate;
    if (timingChanged)
        ++that->m_audioTimingRevision;
    that->m_observedAudioTimingRate = new_rate;
    if (timingChanged)
        that->callAsync([that] { emit that->q_func()->audioTimingContextChanged(); });
    that->callAsync([that] {
        float rate;
        {
            vlc_player_locker lock{that->m_player};
            rate = vlc_player_GetRate(that->m_player);
        }
        // A paused player need not publish a new timer point. Read the current
        // native rate rather than replaying a queued, possibly superseded edit.
        if (!qFuzzyCompare(that->m_rate, rate))
        {
            that->m_rate = rate;
            emit that->q_func()->rateChanged(rate);
        }
    });
}

static void on_player_track_list_changed(vlc_player_t *, enum vlc_player_list_action action, const struct vlc_player_track *track, void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    struct vlc_player_track* dup = vlc_player_track_Dup(track);
    if (!dup)
        return;
    std::shared_ptr<struct vlc_player_track> trackPtr(dup, vlc_player_track_Delete);

    msg_Dbg( that->p_intf, "on_player_track_list_changed");
    that->callAsync([that,action,trackPtr] () {
        switch (trackPtr.get()->fmt.i_cat) {
        case VIDEO_ES:
            msg_Dbg( that->p_intf, "on_player_track_list_changed (video)");
            that->m_videoTracks.updateTracks( action, trackPtr.get() );
            break;
        case AUDIO_ES:
            msg_Dbg( that->p_intf, "on_player_track_list_changed (audio)");
            that->m_audioTracks.updateTracks( action, trackPtr.get() );
            break;
        case SPU_ES:
            msg_Dbg( that->p_intf, "on_player_track_list_changed (spu)");
            that->m_subtitleTracks.updateTracks( action, trackPtr.get() );
            that->RefreshSubtitleDelays();
            break;
        default:
            //we don't handle other kind of tracks
            msg_Dbg( that->p_intf, "on_player_track_list_changed (other)");
            break;
        }
    });
}

static void on_player_track_selection_changed(vlc_player_t *, vlc_es_id_t * unselected, vlc_es_id_t *selected, void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    if (unselected != selected
        && ((unselected && vlc_es_id_GetCat(unselected) == AUDIO_ES)
            || (selected && vlc_es_id_GetCat(selected) == AUDIO_ES)))
    {
        ++that->m_audioTimingRevision;
        that->m_observedAudioTrackDelay.reset();
    }
    msg_Dbg( that->p_intf, "on_player_track_selection_changed");

    SharedEsId unselectedPtr = SharedEsId(unselected);
    SharedEsId selectedPtr = SharedEsId(selected);

    that->callAsync([that,unselectedPtr,selectedPtr] () {
        if (unselectedPtr)
            that->UpdateTrackSelection( unselectedPtr.get(), false );
        if (selectedPtr)
            that->UpdateTrackSelection( selectedPtr.get(), true );
        that->RefreshSubtitleDelays();
        that->m_subtitleAppearance.refresh();
        if ((unselectedPtr && vlc_es_id_GetCat(unselectedPtr.get()) == SPU_ES)
         || (selectedPtr && vlc_es_id_GetCat(selectedPtr.get()) == SPU_ES))
            emit that->q_func()->subtitleSelectionChanged();
        if ((unselectedPtr && vlc_es_id_GetCat(unselectedPtr.get()) == AUDIO_ES)
         || (selectedPtr && vlc_es_id_GetCat(selectedPtr.get()) == AUDIO_ES))
            emit that->q_func()->audioSelectionChanged();
    });
}


static void on_player_program_list_changed(vlc_player_t *, enum vlc_player_list_action action, const struct vlc_player_program *prgm, void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    msg_Dbg( that->p_intf, "on_player_program_list_changed");
    struct vlc_player_program* dup = vlc_player_program_Dup(prgm);
    if (!dup)
        return;
    std::shared_ptr<struct vlc_player_program> prgmPtr(dup, vlc_player_program_Delete);

    that->callAsync([that,action,prgmPtr] (){
        that->UpdateProgram(action, prgmPtr.get());
    });
}

static void on_player_program_selection_changed(vlc_player_t *, int unselected, int selected, void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    msg_Dbg( that->p_intf, "on_player_program_selection_changed");

    that->callAsync([that,unselected,selected] (){
        that->m_programList.updateProgramSelection(unselected, false);
        that->m_programList.updateProgramSelection(selected, true);
    });
}

static void on_player_titles_changed(vlc_player_t *, struct vlc_player_title_list *titles, void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    msg_Dbg( that->p_intf, "on_player_title_array_changed");

    SharedTitleList sharedTitleList = SharedTitleList(titles);

    that->callAsync([that,sharedTitleList] (){
        struct vlc_player_title_list *titles = sharedTitleList.get();
        that->m_chapterList.resetTitle(nullptr);
        that->m_titleList.resetTitles(titles);

        bool hasMenu = false;
        bool hasTitles = false;
        if (titles)
        {
            size_t nbTitles = vlc_player_title_list_GetCount(titles);
            for( size_t i = 0; i < nbTitles; i++)
            {
                const vlc_player_title* title = vlc_player_title_list_GetAt(titles, i);
                if( (title ->flags & INPUT_TITLE_MENU) != 0 )
                {
                    hasMenu = true;
                    break;
                }
            }
            hasTitles = nbTitles != 0;
        }
        if (hasTitles != that->m_hasTitles)
        {
            that->m_hasTitles = hasTitles;
            emit that->q_func()->hasTitlesChanged(hasTitles);
        }
        if (!hasTitles && that->m_hasChapters)
        {
            that->m_hasChapters = false;
            emit that->q_func()->hasChaptersChanged(false);
        }
        if (hasMenu != that->m_hasMenu)
        {
            that->m_hasMenu = hasMenu;
            emit that->q_func()->hasMenuChanged(hasMenu);
        }
    });
}

static void on_player_title_selection_changed(vlc_player_t *,
                                              const struct vlc_player_title *new_title, size_t new_idx, void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    msg_Dbg( that->p_intf, "on_player_title_selection_changed");

    bool hasChapter = (new_title != nullptr && new_title->chapter_count != 0);
    that->callAsync([that,new_title,new_idx,hasChapter] (){
        PlayerController* q = that->q_func();
        that->m_chapterList.resetTitle(new_title);
        that->m_titleList.setCurrent(new_idx);
        that->m_hasChapters  = hasChapter;
        that->m_isInteractive = new_title && (new_title->flags & VLC_PLAYER_TITLE_INTERACTIVE);
        that->m_isMenu = new_title && (new_title->flags & VLC_PLAYER_TITLE_MENU);
        emit q->isMenuChanged( that->m_isMenu );
        emit q->isInteractiveChanged( that->m_isInteractive );
        emit q->hasChaptersChanged( hasChapter );
    });
}

static void on_player_chapter_selection_changed(vlc_player_t *,
                                                const struct vlc_player_title *, size_t,
                                                const struct vlc_player_chapter *, size_t chapter_idx,
                                                void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    msg_Dbg( that->p_intf, "on_player_chapter_selection_changed");
    that->callAsync([that,chapter_idx] (){
        that->m_chapterList.setCurrent(chapter_idx);
    });
}


static void on_player_teletext_menu_changed(vlc_player_t *, bool has_teletext_menu, void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    msg_Dbg( that->p_intf, "on_player_teletext_menu_changed, %s", has_teletext_menu ? "available" : "unavailable" );
    that->callAsync([that,has_teletext_menu] () {
        that->m_teletextAvailable = has_teletext_menu;
        emit that->q_func()->teletextAvailableChanged(has_teletext_menu);
    });
}

static void on_player_teletext_enabled_changed(vlc_player_t *, bool enabled, void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    msg_Dbg( that->p_intf, "on_player_teletext_enabled_changed %s", enabled ? "enabled" : "disabled");
    that->callAsync([that,enabled] () {
        that->m_teletextEnabled = enabled;
        emit that->q_func()->teletextEnabledChanged(enabled);
    });
}

static void on_player_teletext_page_changed(vlc_player_t *, unsigned new_page, void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    msg_Dbg( that->p_intf, "on_player_teletext_page_changed %u", new_page);
    that->callAsync([that,new_page] () {
        that->m_teletextPage = new_page;
        emit that->q_func()->teletextPageChanged(new_page);
    });
}

static void on_player_teletext_transparency_changed(vlc_player_t *, bool enabled, void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    msg_Dbg( that->p_intf, "on_player_teletext_transparency_changed %s", enabled ? "enabled" : "disabled");
    that->callAsync([that,enabled] () {
        that->m_teletextTransparent = enabled;
        emit that->q_func()->teletextTransparencyChanged(enabled);
    });
}

static void on_player_category_delay_changed(vlc_player_t *,
                               enum es_format_category_e cat, vlc_tick_t new_delay,
                               void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    msg_Dbg( that->p_intf, "on_player_category_delay_changed: %d", cat );
    if (cat == AUDIO_ES)
    {
        if (that->m_observedAudioCategoryDelay && *that->m_observedAudioCategoryDelay != new_delay)
            ++that->m_audioTimingRevision;
        that->m_observedAudioCategoryDelay = new_delay;
    }
    that->callAsync([that,cat,new_delay] (){
        switch (cat)
        {
            case AUDIO_ES:
                that->m_audioDelay = new_delay;
                emit that->q_func()->audioDelayChanged( new_delay );
                break;
            case SPU_ES:
                // A queued event payload may precede a preset or role change.
                that->RefreshSubtitleDelays();
                break;
            default:
                break;
        }
    });
}

static void on_player_track_delay_changed(vlc_player_t *player,
                                vlc_es_id_t *es_id, vlc_tick_t new_delay,
                                void *data)
{
    auto *that = static_cast<PlayerControllerPrivate *>(data);
    if (vlc_es_id_GetCat(es_id) == SPU_ES)
        that->callAsync([that] { that->RefreshSubtitleDelays(); });
    else if (vlc_es_id_GetCat(es_id) == AUDIO_ES)
    {
        const auto *selected = vlc_player_GetSelectedTrack(player, AUDIO_ES);
        if (!selected || selected->es_id != es_id)
            return;
        if (that->m_observedAudioTrackDelay && *that->m_observedAudioTrackDelay != new_delay)
            ++that->m_audioTimingRevision;
        that->m_observedAudioTrackDelay = new_delay;
        that->callAsync([that] { emit that->q_func()->audioTimingContextChanged(); });
    }
}

static void on_player_associated_subs_fps_changed(vlc_player_t *, float subs_fps, void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    msg_Dbg( that->p_intf, "on_associated_subs_fps_changed");
    that->callAsync([that,subs_fps] (){
        that->m_subtitleFPS = subs_fps;
        emit that->q_func()->subtitleFPSChanged( subs_fps );
    });
}

void on_player_renderer_changed(vlc_player_t *, vlc_renderer_item_t *new_item, void *data)
{
    VLC_UNUSED(new_item);
    VLC_UNUSED(data);
}


static void on_player_record_changed(vlc_player_t *, bool recording, void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    msg_Dbg( that->p_intf, "on_player_record_changed");
    that->callAsync([that,recording] (){
        that->m_recording = recording;
        emit that->q_func()->recordingChanged( recording );
    });
}

static void on_player_signal_changed(vlc_player_t *, float quality, float strength, void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    VLC_UNUSED(quality);
    VLC_UNUSED(strength);
    msg_Dbg( that->p_intf, "on_player_signal_changed");
}

static void on_player_stats_changed(vlc_player_t *, const struct input_stats_t *stats, void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    struct input_stats_t stats_tmp = *stats;
    that->callAsync([that,stats_tmp] () {
        that->m_stats = stats_tmp;
        emit that->q_func()->statisticsUpdated( that->m_stats );
    });
}

static void on_player_atobloop_changed(vlc_player_t *, enum vlc_player_abloop state, vlc_tick_t time, double, void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    msg_Dbg( that->p_intf, "on_player_atobloop_changed");
    that->callAsync([that,state,time] (){
        PlayerController* q = that->q_func();
        switch (state) {
        case VLC_PLAYER_ABLOOP_NONE:
            that->m_ABLoopA = VLCTime();
            that->m_ABLoopB = VLCTime();
            emit q->ABLoopAChanged(that->m_ABLoopA);
            emit q->ABLoopBChanged(that->m_ABLoopB);
            break;
        case VLC_PLAYER_ABLOOP_A:
            that->m_ABLoopA = time;
            emit q->ABLoopAChanged(that->m_ABLoopA);
            break;
        case VLC_PLAYER_ABLOOP_B:
            that->m_ABLoopB = time;
            emit q->ABLoopBChanged(that->m_ABLoopB);
            break;
        }
        that->m_ABLoopState = static_cast<PlayerController::ABLoopState>(state);
        emit q->ABLoopStateChanged(that->m_ABLoopState);
    });
}

static void on_player_media_meta_changed(vlc_player_t *, input_item_t *media, void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    msg_Dbg( that->p_intf, "on_player_item_meta_changed");

    SharedInputItem mediaPtr(media);
    //call on object thread
    that->callAsync([that,mediaPtr] () {
        that->UpdateName(mediaPtr.get());
        that->UpdateArt(mediaPtr.get());
        that->UpdateMeta(mediaPtr.get());
        that->UpdateInfo(mediaPtr.get());
    });
}

static void on_player_media_epg_changed(vlc_player_t *, input_item_t *, void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    msg_Dbg( that->p_intf, "on_player_item_epg_changed");
    that->callAsync([that] () {
        emit that->q_func()->epgChanged();
    });
}

static void on_player_subitems_changed(vlc_player_t *, input_item_t *, const input_item_node_t *, void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    msg_Dbg( that->p_intf, "on_player_subitems_changed");
}


static void on_player_vout_changed(vlc_player_t *player, enum vlc_player_vout_action action,
    vout_thread_t *, enum vlc_vout_order, vlc_es_id_t *es_id, void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    msg_Dbg( that->p_intf, "on_player_vout_list_changed");

    switch (vlc_es_id_GetCat(es_id))
    {
        case VIDEO_ES:
        {
            //player is locked within callbacks*
            size_t i_vout = 0;
            vout_thread_t **vouts = vlc_player_vout_HoldAll(player, &i_vout);

            std::shared_ptr<vout_thread_t*> voutsPtr( vouts, [i_vout]( vout_thread_t**vouts ) {
                for (size_t i = 0; i < i_vout; i++)
                    vout_Release( vouts[i] );
                free(vouts);
            });

            //call on object thread
            that->callAsync([that,voutsPtr,i_vout,action] () {
                that->UpdateVouts(voutsPtr.get(), i_vout, action);
            });
            break;
        }
        case SPU_ES:
        {
            that->callAsync([that] { that->RefreshSubtitleDelays(); });
            break;
        }
        default:
            break;
    }
}

//player vout callbacks

static void on_player_vout_fullscreen_changed(vout_thread_t* vout, bool is_fullscreen, void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    msg_Dbg( that->p_intf, "on_player_vout_fullscreen_changed %s", is_fullscreen ? "fullscreen" : "windowed");

    SharedVOutThread vOutThread(vout);
    that->callAsync([that,vOutThread,is_fullscreen] () {
        PlayerController* q = that->q_func();
        const PlayerController::VOutThreadList voutList = q->getVouts();
        if (vOutThread == nullptr //property sets for all vout
            || (voutList.size() == 1 && vOutThread.get() == voutList[0].get()) ) //on the only vout
        {
            that->m_fullscreen = is_fullscreen;
            emit q->fullscreenChanged(is_fullscreen);
        }
    });
}

static void on_player_vout_wallpaper_mode_changed(vout_thread_t* vout, bool enabled, void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    msg_Dbg( that->p_intf, "on_player_vout_wallpaper_mode_changed %s", enabled ? "enabled" : "disabled");

    SharedVOutThread vOutThread(vout);
    that->callAsync([that,vOutThread, enabled] () {
        PlayerController* q = that->q_func();
        const PlayerController::VOutThreadList voutList = q->getVouts();
        if (vOutThread == nullptr  //property sets for all vout
            || (voutList.size() == 1 && vOutThread.get() == voutList[0].get()) ) //on the only vout
        {
            that->m_wallpaperMode = enabled;
            emit q->wallpaperModeChanged(enabled);
        }
    });
}

//player aout callbacks

static void on_player_aout_volume_changed(audio_output_t *, float volume, void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    msg_Dbg( that->p_intf, "on_player_aout_volume_changed");
    that->callAsync([that,volume](){
        that->m_volume = volume;
        emit that->q_func()->volumeChanged( volume );
    });
}

static void on_player_aout_mute_changed(audio_output_t *, bool muted, void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    msg_Dbg( that->p_intf, "on_player_aout_mute_changed");
    that->callAsync([that,muted](){
        that->m_muted = muted;
        emit that->q_func()->soundMuteChanged(muted);
    });
}

static void on_player_corks_changed(vlc_player_t *, unsigned, void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    msg_Dbg( that->p_intf, "on_player_corks_changed");
}

static void on_player_playback_restore_queried(vlc_player_t *, void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    msg_Dbg( that->p_intf, "on_playback_restore_queried");
    that->callAsync([that](){
        that->m_canRestorePlayback = true;
        emit that->q_func()->playbackRestoreQueried();
    });
}

static void on_player_timer_update(const struct vlc_player_timer_point *point,
                                   void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    if (that->seeking)
        return;

    that->callAsync([that,point_copy = *point](){
        PlayerController* q = that->q_func();

        const bool wasLive = that->m_player_time.live;
        that->m_player_time = point_copy;
        if (wasLive != point_copy.live)
            emit q->liveChanged(point_copy.live);
        bool lengthOrRateChanged = false;

        if (that->m_length.toVLCTick() != that->m_player_time.length)
        {
            that->m_length = that->m_player_time.length > 0 ? VLCDuration(that->m_player_time.length) : VLCDuration();
            emit q->lengthChanged(that->m_length);

            lengthOrRateChanged = true;
        }
        if (that->m_rate != that->m_player_time.rate)
        {
            that->m_rate = that->m_player_time.rate;
            emit q->rateChanged(that->m_rate);

            lengthOrRateChanged = true;
        }

        vlc_tick_t system_now = vlc_tick_now();
        if (that->interpolateTime(system_now) == VLC_SUCCESS)
        {
            if (lengthOrRateChanged || !that->m_position_timer.isActive())
            {
                q->updatePosition();

                if (that->m_player_time.system_date != VLC_TICK_MAX)
                {
                    // Setup the position update interval, depending on media
                    // length and rate.  XXX: VLC_TICK_FROM_MS(1) is an educated
                    // guess, it should be also calculated according to the slider
                    // size.

                    vlc_tick_t interval =
                        that->m_length.toVLCTick() / that->m_player_time.rate / VLC_TICK_FROM_MS(1);
                    if (interval < POSITION_MIN_UPDATE_INTERVAL)
                        interval = POSITION_MIN_UPDATE_INTERVAL;

                    that->m_position_timer.start(MS_FROM_VLC_TICK(interval));
                }
            }
            q->updateTime(system_now, lengthOrRateChanged);
        }

    });
}

static void on_player_timer_paused(vlc_tick_t system_date, void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    that->callAsync([that,system_date](){
        PlayerController* q = that->q_func();

        if (system_date != VLC_TICK_INVALID
         && that->interpolateTime(system_date) == VLC_SUCCESS)
        {
            // The discontinuity event got a valid system date, update the time
            // properties.
            q->updatePosition();
            q->updateTime(system_date, false);
        }

        // And stop the timers.
        that->m_position_timer.stop();
        that->m_time_timer.stop();
    });
}

static void on_player_timer_seek(const struct vlc_player_timer_point *point,
                                 void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate*>(data);
    if (point != NULL)
    {
        that->seeking = true;
        that->callAsync([that,point_copy = *point](){
            PlayerController* q = that->q_func();
            emit q->seekStarted();

            const bool wasLive = that->m_player_time.live;
            that->m_player_time = point_copy;
            if (wasLive != point_copy.live)
                emit q->liveChanged(point_copy.live);
            if (that->m_player_time.position >= 0)
                that->m_position = that->m_player_time.position;
            that->m_time = vlc::qt::playerSeekTime(point_copy, that->m_time.toVLCTick());
            // A pause may arrive before the first post-seek output point. Do
            // not interpolate the seek request's relative ts as a stream clock.
            that->m_player_time.ts = that->m_time.toVLCTick();

            q->updatePosition();
            q->updateTime(VLC_TICK_INVALID, false);

            that->m_position_timer.stop();
            that->m_time_timer.stop();
        });
    }
    else
        that->seeking = false;
}
static void on_player_timer_smpte_update(const struct vlc_player_timer_smpte_timecode *tc,
                                         void *data)
{
    PlayerControllerPrivate* that = static_cast<PlayerControllerPrivate *>(data);
#ifndef QT_HAS_LIBATOMIC
    QWriteLocker lock(&that->m_highResolutionTimeLock);
#endif
    that->m_highResolutionTime = *tc;
}


static void on_preparse_ended_callback(vlc_preparser_req *req,
                                       int, void *userdata)
{
    input_item_t *p_item = vlc_preparser_req_GetItem(req);
    PlayerControllerPrivate *me = reinterpret_cast<PlayerControllerPrivate *>(userdata);
    me->onArtFetchEnded(p_item, input_item_IsArtFetched(p_item));
}


} //extern "C"

static const struct vlc_player_cbs player_cbs = [] {
    struct vlc_player_cbs cbs {};
    cbs.on_current_media_changed = on_player_current_media_changed;
    cbs.on_state_changed = on_player_state_changed;
    cbs.on_length_changed = on_player_length_changed;
    cbs.on_error_changed = on_player_error_changed;
    cbs.on_buffering_changed = on_player_buffering;
    cbs.on_capabilities_changed = on_player_capabilities_changed;
    cbs.on_rate_changed = on_player_rate_changed;
    cbs.on_track_list_changed = on_player_track_list_changed;
    cbs.on_track_selection_changed = on_player_track_selection_changed;
    cbs.on_track_delay_changed = on_player_track_delay_changed;
    cbs.on_program_list_changed = on_player_program_list_changed;
    cbs.on_program_selection_changed = on_player_program_selection_changed;
    cbs.on_titles_changed = on_player_titles_changed;
    cbs.on_title_selection_changed = on_player_title_selection_changed;
    cbs.on_chapter_selection_changed = on_player_chapter_selection_changed;
    cbs.on_teletext_menu_changed = on_player_teletext_menu_changed;
    cbs.on_teletext_enabled_changed = on_player_teletext_enabled_changed;
    cbs.on_teletext_page_changed = on_player_teletext_page_changed;
    cbs.on_teletext_transparency_changed = on_player_teletext_transparency_changed;
    cbs.on_category_delay_changed = on_player_category_delay_changed;
    cbs.on_associated_subs_fps_changed = on_player_associated_subs_fps_changed;
    cbs.on_renderer_changed = on_player_renderer_changed;
    cbs.on_recording_changed = on_player_record_changed;
    cbs.on_signal_changed = on_player_signal_changed;
    cbs.on_statistics_changed = on_player_stats_changed;
    cbs.on_atobloop_changed = on_player_atobloop_changed;
    cbs.on_media_meta_changed = on_player_media_meta_changed;
    cbs.on_media_epg_changed = on_player_media_epg_changed;
    cbs.on_media_subitems_changed = on_player_subitems_changed;
    cbs.on_vout_changed = on_player_vout_changed;
    cbs.on_cork_changed = on_player_corks_changed;
    cbs.on_playback_restore_queried = on_player_playback_restore_queried;
    return cbs;
}();

static const vlc_player_vout_cbs player_vout_cbs = []{
    struct vlc_player_vout_cbs cbs{};
    cbs.on_fullscreen_changed = on_player_vout_fullscreen_changed;
    cbs.on_wallpaper_mode_changed = on_player_vout_wallpaper_mode_changed;
    return cbs;
}();

static const vlc_player_aout_cbs player_aout_cbs = []{
    struct vlc_player_aout_cbs cbs{};
    cbs.on_volume_changed = on_player_aout_volume_changed;
    cbs.on_mute_changed = on_player_aout_mute_changed;
    return cbs;
}();

static const vlc_player_timer_cbs player_timer_cbs = []{
    struct vlc_player_timer_cbs cbs {};
    cbs.on_update = on_player_timer_update;
    cbs.on_paused = on_player_timer_paused;
    cbs.on_seek = on_player_timer_seek;
    return cbs;
}();

static const struct vlc_player_timer_smpte_cbs player_timer_smpte_cbs = []{
    struct vlc_player_timer_smpte_cbs cbs {};
    cbs.on_update = on_player_timer_smpte_update;
    return cbs;
}();

// art fetcher callbacks

static const struct vlc_preparser_cbs art_fetcher_cbs = []{
    struct vlc_preparser_cbs cbs{};
    cbs.on_ended = on_preparse_ended_callback;
    return cbs;
}();


PlayerControllerPrivate::PlayerControllerPrivate(PlayerController *playercontroller, qt_intf_t *p_intf)
    : q_ptr(playercontroller)
    , p_intf(p_intf)
    , m_player(p_intf->p_player)
    , m_videoTracks(m_player)
    , m_audioTracks(m_player)
    , m_subtitleTracks(m_player)
    , m_titleList(m_player)
    , m_chapterList(m_player)
    , m_programList(m_player)
    , m_videoStereoMode((vout_thread_t*)nullptr, "video-stereo-mode")
    , m_zoom((vout_thread_t*)nullptr, "zoom")
    , m_aspectRatio((vout_thread_t*)nullptr, "aspect-ratio")
    , m_crop((vout_thread_t*)nullptr, "crop")
    , m_fit((vout_thread_t*)nullptr, "fit")
    , m_deinterlace((vout_thread_t*)nullptr, "deinterlace")
    , m_deinterlaceMode((vout_thread_t*)nullptr, "deinterlace-mode")
    , m_autoscale((vout_thread_t*)nullptr, "autoscale")
    , m_subtitleAppearance(m_player)
    , m_subtitleTimeline(m_player)
    , m_audioStereoMode((audio_output_t*)nullptr, "stereo-mode")
    , m_audioMixMode((audio_output_t*)nullptr, "mix-mode")
    , m_audioDeviceList(m_player)
    , m_audioVisualization((audio_output_t*)nullptr, "visual")
    , m_equalizer(m_player)
    , m_waveform(VLC_OBJECT(p_intf))
    , m_musicFolder(VLC_OBJECT(p_intf), p_intf->p_playlist)
    , m_rendererManager(p_intf, m_player)
{
    {
        vlc_player_locker locker{m_player};
        m_subtitleFPS = vlc_player_GetAssociatedSubsFPS(m_player);
        m_player_listener = vlc_player_AddListener( m_player, &player_cbs, this );
        m_player_aout_listener = vlc_player_aout_AddListener( m_player, &player_aout_cbs, this );
        m_player_vout_listener = vlc_player_vout_AddListener( m_player, &player_vout_cbs, this );
        m_player_timer = vlc_player_AddTimer( m_player, VLC_TICK_FROM_MS(500), &player_timer_cbs, this );
    }

    QObject::connect( &m_autoscale, &QVLCBool::valueChanged, q_ptr, &PlayerController::autoscaleChanged );
    QObject::connect( &m_audioVisualization, &VLCVarChoiceModel::hasCurrentChanged, q_ptr, &PlayerController::hasAudioVisualizationChanged );

    m_time_timer.setSingleShot( true );
    m_time_timer.setTimerType( Qt::PreciseTimer );

    // Initialise fullscreen to match the player state
    m_fullscreen = vlc_player_vout_IsFullscreen( m_player );

    m_volume = vlc_player_aout_GetVolume( m_player );
}

void PlayerControllerPrivate::addSMPTETimer()
{
    assert( !m_player_timer_smpte );

    vlc_player_locker lock{ m_player };
    m_player_timer_smpte = vlc_player_AddSmpteTimer( m_player, &player_timer_smpte_cbs, this );
    assert( m_player_timer_smpte );
}

void PlayerControllerPrivate::removeSMPTETimer()
{
    assert( m_player_timer_smpte );

    vlc_player_locker lock{ m_player };
    vlc_player_RemoveTimer( m_player, m_player_timer_smpte );
    m_player_timer_smpte = nullptr;
}

PlayerController::PlayerController( qt_intf_t *_p_intf )
    : QObject(NULL)
    , d_ptr( new PlayerControllerPrivate(this, _p_intf) )
{
    connect( &d_ptr->m_position_timer, &QTimer::timeout, this, &PlayerController::updatePositionFromTimer );
    connect( &d_ptr->m_time_timer, &QTimer::timeout, this, &PlayerController::updateTimeFromTimer );
}

PlayerController::~PlayerController()
{
}

// PLAYBACK

vlc_player_t * PlayerController::getPlayer() const
{
    Q_D(const PlayerController);

    return d->m_player;
}

input_item_t *PlayerController::getInput() const
{
    Q_D(const PlayerController);
    vlc_player_locker locker{ d->m_player };
    return vlc_player_GetCurrentMedia( d->m_player );
}

bool PlayerController::isStarted() const
{
    Q_D(const PlayerController);
    vlc_player_locker locker{ d->m_player };
    return vlc_player_IsStarted( d->m_player );
}

bool PlayerController::hasInput() const
{
    return getInput();
}

void PlayerController::reverse()
{
    Q_D(PlayerController);
    msg_Dbg( d->p_intf, "reverse");
    vlc_player_locker lock{ d->m_player };
    if ( vlc_player_CanChangeRate( d->m_player ) )
    {
        float f_rate_ = vlc_player_GetRate( d->m_player );
        vlc_player_ChangeRate( d->m_player, -f_rate_ );
    }
}

void PlayerController::setRate( float new_rate )
{
    Q_D(PlayerController);

    if (qFuzzyCompare(d->m_rate, new_rate))
        return;

    msg_Dbg( d->p_intf, "setRate %f", new_rate);
    {
        vlc_player_locker lock{ d->m_player };
        if (!vlc_player_CanChangeRate(d->m_player))
            return;
        vlc_player_ChangeRate( d->m_player, new_rate );

        // Until the timer notifies the change, we need to still change
        // the rate, because player timer update may take a long time.
        // For position, it is supposed to be interpolated but for rate
        // the new value can be taken as is:
        d->m_rate = new_rate;
    }
    emit rateChanged(d->m_rate);
}

void PlayerController::slower()
{
    Q_D(PlayerController);
    msg_Dbg( d->p_intf, "slower");
    vlc_player_locker lock{ d->m_player };
    if ( vlc_player_CanChangeRate( d->m_player ) )
        vlc_player_DecrementRate( d->m_player );
}

void PlayerController::faster()
{
    Q_D(PlayerController);
    msg_Dbg( d->p_intf, "faster");
    vlc_player_locker lock{ d->m_player };
    if ( vlc_player_CanChangeRate( d->m_player ) )
        vlc_player_IncrementRate( d->m_player );
}

void PlayerController::littlefaster()
{
    Q_D(PlayerController);
    msg_Dbg( d->p_intf, "littlefaster");
    var_SetInteger( vlc_object_instance(d->p_intf), "key-action", ACTIONID_RATE_FASTER_FINE );
}

void PlayerController::littleslower()
{
    Q_D(PlayerController);
    msg_Dbg( d->p_intf, "littleslower");
    var_SetInteger( vlc_object_instance(d->p_intf), "key-action", ACTIONID_RATE_SLOWER_FINE );
}

void PlayerController::normalRate()
{
    Q_D(PlayerController);
    msg_Dbg( d->p_intf, "normalRate");
    vlc_player_locker lock{ d->m_player };
    if ( vlc_player_CanChangeRate( d->m_player ) )
        vlc_player_ChangeRate( d->m_player, 1.0f );
}


void PlayerController::setTime(VLCTime new_time)
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    if( !d->isCurrentItemSynced() )
        return;
    vlc_player_SeekByTime( d->m_player, new_time.toVLCTick(), qtPlaybackSeekSpeed(VLC_OBJECT(d->p_intf)), VLC_PLAYER_WHENCE_ABSOLUTE );
}

void PlayerController::setPosition(double position)
{
    Q_D(PlayerController);
    if (qFuzzyCompare(d->m_position, position))
        return;

    vlc_player_locker lock{ d->m_player };
    if( !d->isCurrentItemSynced() )
        return;
    vlc_player_SeekByPos( d->m_player, position, qtPlaybackSeekSpeed(VLC_OBJECT(d->p_intf)), VLC_PLAYER_WHENCE_ABSOLUTE );
}

void PlayerController::jumpFwd()
{
    Q_D(PlayerController);
    msg_Dbg( d->p_intf, "jumpFwd");
    int i_interval = var_InheritInteger( d->p_intf, "short-jump-size" );
    vlc_player_locker lock{ d->m_player };
    if( !d->isCurrentItemSynced() )
        return;
    vlc_player_SeekByTime( d->m_player, vlc_tick_from_sec( i_interval ), qtPlaybackSeekSpeed(VLC_OBJECT(d->p_intf)), VLC_PLAYER_WHENCE_RELATIVE );
}

void PlayerController::jumpBwd()
{
    Q_D(PlayerController);
    msg_Dbg( d->p_intf, "jumpBwd");
    int i_interval = var_InheritInteger( d->p_intf, "short-jump-size" );
    vlc_player_locker lock{ d->m_player };
    if( !d->isCurrentItemSynced() )
        return;
    vlc_player_SeekByTime( d->m_player, vlc_tick_from_sec( -i_interval ), qtPlaybackSeekSpeed(VLC_OBJECT(d->p_intf)), VLC_PLAYER_WHENCE_RELATIVE );
}

void PlayerController::jumpTime(VLCTime i_time)
{
    Q_D(PlayerController);
    msg_Dbg( d->p_intf, "jumpTime");
    vlc_player_locker lock{ d->m_player };
    if( !d->isCurrentItemSynced() )
        return;
    vlc_player_SeekByTime( d->m_player, i_time.toVLCTick(), qtPlaybackSeekSpeed(VLC_OBJECT(d->p_intf)), VLC_PLAYER_WHENCE_RELATIVE );
}

void PlayerController::jumpSeconds(int seconds)
{
    // Fixed-duration palette tiles retain their displayed 10/15 seconds even
    // when the separate keyboard short-jump preference has been customized.
    jumpTime(VLCTime(vlc_tick_from_sec(seconds)));
}

void PlayerController::jumpToPos( double new_pos )
{
    Q_D(PlayerController);
    {
        vlc_player_locker lock{ d->m_player };
        if( !d->isCurrentItemSynced() )
            return;
        if( vlc_player_IsStarted( d->m_player ) )
            vlc_player_SeekByPos( d->m_player, new_pos, qtPlaybackSeekSpeed(VLC_OBJECT(d->p_intf)), VLC_PLAYER_WHENCE_ABSOLUTE );
    }
    emit seekRequested( new_pos );
}

void PlayerController::frameNext()
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    if( !d->isCurrentItemSynced() )
        return;
    vlc_player_NextVideoFrame( d->m_player );
}

void PlayerController::framePrev()
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    if( !d->isCurrentItemSynced() )
        return;
    vlc_player_PreviousVideoFrame( d->m_player );
}

//TRACKS

void PlayerController::setAudioDelay(VLCDuration delay)
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    if( !d->isCurrentItemSynced() )
        return;
    vlc_player_SetAudioDelay( d->m_player, delay.toVLCTick(), VLC_PLAYER_WHENCE_ABSOLUTE );
}

/*Q_INVOKABLE*/void PlayerController::addAudioDelay(VLCDuration delay)
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    if (!delay.valid() || !d->m_currentItem || !d->isCurrentItemSynced())
        return;
    vlc_player_SetAudioDelay( d->m_player, delay.toVLCTick(), VLC_PLAYER_WHENCE_RELATIVE );
    // The native callback publishes the final value after releasing the lock.
}

QString PlayerController::beginAudioTiming()
{
    Q_D(PlayerController);
    vlc_player_locker lock{d->m_player};
    if (!d->m_currentItem || !d->isCurrentItemSynced())
        return {};
    d->refreshAudioTimingContextLocked();
    return QString::number(d->m_audioTimingRevision);
}

bool PlayerController::isAudioTimingCurrent(const QString &token)
{
    Q_D(PlayerController);
    vlc_player_locker lock{d->m_player};
    bool ok = false;
    const auto revision = token.toULongLong(&ok);
    if (!ok || !d->m_currentItem || !d->isCurrentItemSynced())
        return false;
    d->refreshAudioTimingContextLocked();
    return revision == d->m_audioTimingRevision;
}

bool PlayerController::addAudioTimingIfCurrent(VLCDuration wallDelay, const QString &token)
{
    Q_D(PlayerController);
    vlc_player_locker lock{d->m_player};
    bool ok = false;
    const auto revision = token.toULongLong(&ok);
    if (!ok || !wallDelay.valid() || !d->m_currentItem || !d->isCurrentItemSynced())
        return false;
    d->refreshAudioTimingContextLocked();
    if (revision != d->m_audioTimingRevision)
        return false;
    // The GUI rate may lag the native event; scale under the same lock used
    // for the revision check and the resulting category update.
    const long double scaled = static_cast<long double>(wallDelay.toVLCTick())
        * static_cast<long double>(vlc_player_GetRate(d->m_player));
    if (!std::isfinite(scaled) || scaled >= static_cast<long double>(INT64_MAX)
        || scaled <= static_cast<long double>(INT64_MIN))
        return false;
    const auto current = vlc_player_GetAudioDelay(d->m_player);
    const auto delta = static_cast<vlc_tick_t>(scaled);
    if ((delta > 0 && current > INT64_MAX - delta)
        || (delta < 0 && current < INT64_MIN - delta))
        return false;
    return vlc_player_SetCategoryDelay(d->m_player, AUDIO_ES, delta,
                                       VLC_PLAYER_WHENCE_RELATIVE) == VLC_SUCCESS;
}

void PlayerController::setSubtitleDelay(VLCDuration delay)
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    if(!d->isCurrentItemSynced() )
        return;
    vlc_player_SetSubtitleDelay( d->m_player, delay.toVLCTick(), VLC_PLAYER_WHENCE_ABSOLUTE );
}

/*Q_INVOKABLE*/void PlayerController::addSubtitleDelay(VLCDuration delay)
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    if (!delay.valid() || !d->m_currentItem || !d->isCurrentItemSynced())
        return;
    vlc_player_SetSubtitleDelay( d->m_player, delay.toVLCTick(), VLC_PLAYER_WHENCE_RELATIVE);
    // A delta is not the resulting delay. Direct observers may also re-lock
    // the player, so leave notification to the queued native callback.
}

void PlayerController::setSecondarySubtitleDelay(VLCDuration delay)
{
    Q_D(PlayerController);
    vlc_player_locker lock{d->m_player};
    if (!delay.valid() || !d->m_currentItem || !d->isCurrentItemSynced())
        return;
    // Read the current role while locked; the GUI's last role can be stale.
    if (auto *secondary = d->selectedSecondarySubtitle())
        vlc_player_SetEsIdDelay(d->m_player, secondary, delay.toVLCTick(),
                               VLC_PLAYER_WHENCE_ABSOLUTE);
}

/*Q_INVOKABLE*/void PlayerController::addSecondarySubtitleDelay(VLCDuration delay)
{
    Q_D(PlayerController);
    vlc_player_locker lock{d->m_player};
    if (!delay.valid() || !d->m_currentItem || !d->isCurrentItemSynced())
        return;
    auto *secondary = d->selectedSecondarySubtitle();
    if (!secondary)
        return;
    const auto raw = vlc_player_GetEsIdDelay(d->m_player, secondary);
    const auto effective = raw == INT64_MAX ? vlc_player_GetSubtitleDelay(d->m_player) : raw;
    const auto delta = delay.toVLCTick();
    if ((delta > 0 && effective >= INT64_MAX - delta)
        || (delta < 0 && effective < INT64_MIN - delta))
        return;
    // Native relative delay starts at zero for an inherited track. A UI nudge
    // must start from its effective category delay and create an own override.
    vlc_player_SetEsIdDelay(d->m_player, secondary, effective + delta,
                           VLC_PLAYER_WHENCE_ABSOLUTE);
    // The queued native callback publishes the final value after unlocking.
}

int PlayerController::getAudioDelayMS() const
{
    return getAudioDelay().toMilliseconds();
}

void PlayerController::setAudioDelayMS(int ms)
{
    setAudioDelay( VLCDuration::fromMS(ms) );
}

int PlayerController::getSubtitleDelayMS() const
{
    return getSubtitleDelay().toMilliseconds();
}

void PlayerController::setSubtitleDelayMS(int ms)
{
    setSubtitleDelay( VLCDuration::fromMS(ms) );
}

int PlayerController::getSecondarySubtitleDelayMS() const
{
    return getSecondarySubtitleDelay().toMilliseconds();
}

void PlayerController::setSecondarySubtitleDelayMS(int ms)
{
    setSecondarySubtitleDelay( VLCDuration::fromMS(ms) );
}

void PlayerController::setSubtitleFPS(float fps)
{
    if (!std::isfinite(fps) || fps < 0.f)
        return;
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    if( !d->isCurrentItemSynced() )
        return;
    vlc_player_SetAssociatedSubsFPS( d->m_player, fps );
}

//TITLE/CHAPTER/MENU

void PlayerController::sectionPrev()
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    if( !d->isCurrentItemSynced() )
        return;
    if( vlc_player_IsStarted( d->m_player ) )
    {
        if (vlc_player_GetSelectedChapter( d->m_player ) != NULL)
            vlc_player_SelectPrevChapter( d->m_player );
        else
            vlc_player_SelectPrevTitle( d->m_player );
    }
}

void PlayerController::sectionNext()
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    if( !d->isCurrentItemSynced() )
        return;
    if( vlc_player_IsStarted( d->m_player ) )
    {
        if (vlc_player_GetSelectedChapter( d->m_player ) != NULL)
            vlc_player_SelectNextChapter( d->m_player );
        else
            vlc_player_SelectNextTitle( d->m_player );
    }
}

void PlayerController::sectionMenu()
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    if( !d->isCurrentItemSynced() )
        return;
    if( vlc_player_IsStarted( d->m_player ) )
        vlc_player_Navigate( d->m_player, VLC_PLAYER_NAV_MENU );
}

void PlayerController::navigateUp()
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    if( !d->isCurrentItemSynced() )
        return;
    if( vlc_player_IsStarted( d->m_player ) )
        vlc_player_Navigate( d->m_player, VLC_PLAYER_NAV_UP);
}

void PlayerController::navigateDown()
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    if( !d->isCurrentItemSynced() )
        return;
    if( vlc_player_IsStarted( d->m_player ) )
        vlc_player_Navigate( d->m_player, VLC_PLAYER_NAV_DOWN);
}

void PlayerController::navigateLeft()
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    if( !d->isCurrentItemSynced() )
        return;
    if( vlc_player_IsStarted( d->m_player ) )
        vlc_player_Navigate( d->m_player, VLC_PLAYER_NAV_LEFT);
}

void PlayerController::navigateRight()
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    if( !d->isCurrentItemSynced() )
        return;
    if( vlc_player_IsStarted( d->m_player ) )
        vlc_player_Navigate( d->m_player, VLC_PLAYER_NAV_RIGHT);
}

void PlayerController::navigateActivate()
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    if( !d->isCurrentItemSynced() )
        return;
    if( vlc_player_IsStarted( d->m_player ) )
        vlc_player_Navigate( d->m_player, VLC_PLAYER_NAV_ACTIVATE);
}

void PlayerController::chapterNext()
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    if( !d->isCurrentItemSynced() )
        return;
    if (vlc_player_IsStarted(d->m_player ))
        vlc_player_SelectNextChapter( d->m_player );
}

void PlayerController::chapterPrev()
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    if( !d->isCurrentItemSynced() )
        return;
    if (vlc_player_IsStarted(d->m_player ))
        vlc_player_SelectPrevChapter( d->m_player );
}

void PlayerController::titleNext()
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    if( !d->isCurrentItemSynced() )
        return;
    if (vlc_player_IsStarted(d->m_player ))
        vlc_player_SelectNextTitle( d->m_player );
}

void PlayerController::titlePrev()
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    if( !d->isCurrentItemSynced() )
        return;
    if (vlc_player_IsStarted(d->m_player ))
        vlc_player_SelectPrevTitle( d->m_player );
}

//PROGRAMS

void PlayerController::changeProgram( int program )
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    if( !d->isCurrentItemSynced() )
        return;
    if( vlc_player_IsStarted( d->m_player ) )
        vlc_player_SelectProgram( d->m_player, program );
}

//TELETEXT


void PlayerController::enableTeletext( bool enable )
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    if( !d->isCurrentItemSynced() )
        return;
    if (vlc_player_IsStarted(d->m_player ))
        vlc_player_SetTeletextEnabled( d->m_player, enable );
}

void PlayerController::setTeletextPage(int page)
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    if( !d->isCurrentItemSynced() )
        return;
    if (vlc_player_IsTeletextEnabled( d->m_player ))
        vlc_player_SelectTeletextPage( d->m_player, page );
}

void PlayerController::setTeletextTransparency( bool transparent )
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    if( !d->isCurrentItemSynced() )
        return;
    if (vlc_player_IsTeletextEnabled( d->m_player ))
        vlc_player_SetTeletextTransparency( d->m_player, transparent );
}

//VOUT PROPERTIES

PlayerController::VOutThreadList PlayerController::getVouts() const
{
    Q_D(const PlayerController);
    vout_thread_t **pp_vout;
    VOutThreadList VoutList;
    size_t i_vout;
    {
        vlc_player_locker lock{ d->m_player };
        if( !vlc_player_IsStarted( d->m_player ) )
            return VOutThreadList{};
        i_vout = 0;
        pp_vout = vlc_player_vout_HoldAll( d->m_player, &i_vout );
        if ( i_vout <= 0 )
            return VOutThreadList{};
    }
    VoutList.reserve( i_vout );
    for( size_t i = 0; i < i_vout; i++ )
    {
        assert( pp_vout[i] );
        //pass ownership
        VoutList.append(SharedVOutThread(pp_vout[i], false));
    }
    free( pp_vout );

    return VoutList;
}

SharedVOutThread PlayerController::getVout()
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    vout_thread_t* vout = vlc_player_vout_Hold( d->m_player );
    if( vout == NULL )
        return SharedVOutThread{};
    return SharedVOutThread{vout, false};
}

void PlayerController::setFullscreen( bool new_val )
{
    Q_D(PlayerController);
    msg_Dbg(d->p_intf, "setFullscreen %s", new_val? "fullscreen" : "windowed");
    vlc_player_locker lock{ d->m_player };
    vlc_player_vout_SetFullscreen( d->m_player, new_val );
}

void PlayerController::toggleFullscreen()
{
    Q_D(PlayerController);
    setFullscreen( ! d->m_fullscreen );
}

void PlayerController::setWallpaperMode( bool new_val )
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    vlc_player_vout_SetWallpaperModeEnabled( d->m_player, new_val );
}

bool PlayerController::getAutoscale( ) const
{
    Q_D(const PlayerController);
    return d->m_autoscale.getValue();
}

void PlayerController::setAutoscale( bool new_val )
{
    Q_D(PlayerController);
    d->m_autoscale.setValue( new_val );
}

//AOUT PROPERTIES

SharedAOut PlayerController::getAout()
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    return SharedAOut( vlc_player_aout_Hold( d->m_player ), false );
}

AudioEqualizer *PlayerController::getEqualizer() const
{
    Q_D(const PlayerController);
    return const_cast<AudioEqualizer *>(&d->m_equalizer);
}

AudioWaveform *PlayerController::getWaveform() const
{
    Q_D(const PlayerController);
    return const_cast<AudioWaveform *>(&d->m_waveform);
}

LocalMusicFolderModel *PlayerController::getMusicFolder() const
{
    Q_D(const PlayerController);
    return const_cast<LocalMusicFolderModel *>(&d->m_musicFolder);
}

VideoTransform *PlayerController::getVideoTransform() const
{
    Q_D(const PlayerController);
    return const_cast<VideoTransform *>(&d->m_videoTransform);
}

SubtitleAppearance *PlayerController::getSubtitleAppearance() const
{
    Q_D(const PlayerController);
    return const_cast<SubtitleAppearance *>(&d->m_subtitleAppearance);
}

SubtitleCueTimeline *PlayerController::getSubtitleTimeline() const
{
    Q_D(const PlayerController);
    return const_cast<SubtitleCueTimeline *>(&d->m_subtitleTimeline);
}

void PlayerController::setVolume(float volume)
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    vlc_player_aout_SetVolume( d->m_player, volume );
}

void PlayerController::setVolumeUp(int steps)
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    vlc_player_aout_IncrementVolume( d->m_player, steps, NULL );
}

void PlayerController::setVolumeDown(int steps)
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    vlc_player_aout_DecrementVolume( d->m_player, steps, NULL );
}

void PlayerController::setMuted(bool muted)
{
    Q_D(PlayerController);
    if( d->m_muted == muted )
        return;
    vlc_player_locker lock{ d->m_player };
    vlc_player_aout_Mute( d->m_player, muted );
}

void PlayerController::toggleMuted()
{
    Q_D(PlayerController);
    setMuted( !d->m_muted );
}

bool PlayerController::hasAudioVisualization() const
{
    Q_D(const PlayerController);
    return d->m_audioVisualization.hasCurrent();
}

void PlayerController::updatePosition()
{
    Q_D(PlayerController);

    // Update position properties
    emit positionChanged(d->m_position);
    emit positionUpdated(d->m_position, d->m_time, d->m_length);
}

void PlayerController::updatePositionFromTimer()
{
    Q_D(PlayerController);

    vlc_tick_t system_now = vlc_tick_now();
    if (d->interpolateTime(system_now) == VLC_SUCCESS)
        updatePosition();
}

void PlayerController::updateTime(vlc_tick_t system_now, bool forceUpdate)
{
    Q_D(PlayerController);

    // Update time properties
    emit timeChanged(d->m_time);
    emit displayTimeChanged();
    if (d->m_time.valid() && d->m_length.valid() && d->m_time <= VLCTime(d->m_length))
        d->m_remainingTime = VLCTime(d->m_length) - d->m_time;
    else
        d->m_remainingTime = VLCDuration();
    emit remainingTimeChanged(d->m_remainingTime);

    if (d->m_syltRaw.has_value() && d->m_time.valid())
    {
        const auto& lyrics = d->syltLyrics();
        int currentIndex = -1;
        const vlc_tick_t currentTime = d->m_time.toVLCTick();

        for (int i = 0; i < lyrics.size(); ++i)
        {
            if ((lyrics[i].time().toVLCTick() - 1) > currentTime)
                break;
            currentIndex = i;
        }

        if (currentIndex != d->m_currentLyricIndex)
        {
            d->m_currentLyricIndex = currentIndex;
            emit currentLyricIndexChanged( currentIndex );
            emit currentLyricChanged();
        }
    }

    if (system_now != VLC_TICK_INVALID
     && d->m_player_time.system_date != VLC_TICK_MAX
     && (forceUpdate || !d->m_time_timer.isActive()))
    {
        // Tell the timer to wait until the next second is reached.
        vlc_tick_t next_update_date =
            vlc_player_timer_point_GetNextIntervalDate(&d->m_player_time, system_now,
                                                       d->m_time.toVLCTick(), VLC_TICK_FROM_SEC(1));

        vlc_tick_t next_update_interval = next_update_date - system_now;

        if (next_update_interval > 0)
        {
            // The timer can be triggered a little before. In that case, it's
            // likely that we didn't reach the next next second. It's better to
            // add a very small delay in order to be triggered after the next
            // seconds.
            static const unsigned imprecision_delay_ms = 30;

            d->m_time_timer.start(MS_FROM_VLC_TICK(next_update_interval)
                                  + imprecision_delay_ms);
        }
    }
}

void PlayerController::openVLsub()
{
    Q_D(PlayerController);

    const auto extensionManager = ExtensionsManager::getInstance( d->p_intf );
    if ( !extensionManager->isLoaded() )
        extensionManager->loadExtensions();

    extensionManager->openVLsub();
}

void PlayerController::updateTimeFromTimer()
{
    Q_D(PlayerController);

    vlc_tick_t system_now = vlc_tick_now();
    if (d->interpolateTime(system_now) == VLC_SUCCESS)
        updateTime(system_now, false);
}

void PlayerController::restorePlaybackPos()
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    if (!d->isCurrentItemSynced())
        return;
    vlc_player_RestorePlaybackPos( d->m_player );
}

void PlayerController::acknowledgeRestoreCallback()
{
    Q_D(PlayerController);
    if (d->m_canRestorePlayback) {
        d->m_canRestorePlayback = false;
        emit playbackRestoreQueried();
    }
}

//MISC

void PlayerController::setABloopState(ABLoopState state)
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    vlc_player_SetAtoBLoop( d->m_player, static_cast<vlc_player_abloop>(state));
}

void PlayerController::toggleABloopState()
{
    Q_D(PlayerController);
    switch (d->m_ABLoopState) {
    case ABLOOP_STATE_NONE:
        setABloopState(ABLOOP_STATE_A);
        break;
    case ABLOOP_STATE_A:
        setABloopState(ABLOOP_STATE_B);
        break;
    case ABLOOP_STATE_B:
        setABloopState(ABLOOP_STATE_NONE);
        break;
    }
}

void PlayerController::toggleRecord()
{
    Q_D(PlayerController);
    setRecording(!d->m_recording);
}

void PlayerController::toggleVisualization()
{
    Q_D(PlayerController);

    if ( d->m_audioVisualization.rowCount() < 1 )
        return;

    if ( !d->m_audioVisualization.hasCurrent()
         || d->m_audioVisualization.currentRow() == 0 /*0th row is "Disable"*/)
    {
        const int r = rand() % d->m_audioVisualization.rowCount();
        d->m_audioVisualization.setData( d->m_audioVisualization.index( r )
                                         , QVariant::fromValue<bool>( true )
                                         , Qt::CheckStateRole );
    }
    else
    {
        d->m_audioVisualization.setData( d->m_audioVisualization.index(0)
                                         , QVariant::fromValue<bool>( true )
                                         , Qt::CheckStateRole);
    }
}

void PlayerController::requestAddSMPTETimer()
{
    Q_D(PlayerController);

    if ( ++d->m_smpteTimerRequestCount == 1 )
    {
        d->addSMPTETimer();
    }
}

void PlayerController::requestRemoveSMPTETimer()
{
    Q_D(PlayerController);

    assert( d->m_smpteTimerRequestCount > 0 );
    if ( --d->m_smpteTimerRequestCount == 0 )
    {
        d->removeSMPTETimer();
    }
}

void PlayerController::setRecording( bool recording )
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    vlc_player_SetRecordingEnabled( d->m_player, recording, NULL );
}

void PlayerController::snapshot()
{
    SharedVOutThread vout = getVout();
    if (vout)
    {
        /* Passing a lambda directly would require Qt 5.15:
         * <https://doc.qt.io/qt-5/qthreadpool.html#start-1>
         */
        struct SnapshotTask : public QRunnable
        {
            SharedVOutThread vout;
            SnapshotTask(SharedVOutThread vout) : vout(std::move(vout)) {}
            void run() override
            {
                var_TriggerCallback(vout.get(), "video-snapshot");
            }
        };

        QThreadPool::globalInstance()->start(new SnapshotTask(std::move(vout)));
    }
}

QString PlayerController::highResolutionTime() const
{
    Q_D(const PlayerController);

    // Sample:
    vlc_player_timer_smpte_timecode sample;
    {
#ifndef QT_HAS_LIBATOMIC
        QReadLocker lock(&d->m_highResolutionTimeLock);
#endif
        sample = d->m_highResolutionTime;
    }

    const vlc_player_timer_smpte_timecode& lastSample = d->m_highResolutionTimeSample.first;

    if (sample.frames == lastSample.frames &&
        sample.frame_resolution == lastSample.frame_resolution &&
        sample.seconds == lastSample.seconds &&
        sample.minutes == lastSample.minutes &&
        sample.hours == lastSample.hours &&
        sample.drop_frame == lastSample.drop_frame)
        return d->m_highResolutionTimeSample.second;

    d->m_highResolutionTimeSample.second = QString::asprintf("%02u:%02u:%02u%c%02u",
                                                             sample.hours,
                                                             sample.minutes,
                                                             sample.seconds,
                                                             sample.drop_frame ? '.' : ':',
                                                             sample.frames);
    d->m_highResolutionTimeSample.first = std::move(sample);

    return d->m_highResolutionTimeSample.second;
}

//OTHER


/* Playlist Control functions */

void PlayerController::requestArtUpdate( input_item_t *p_item )
{
    Q_D(PlayerController);

    if (d->m_preparser == nullptr)
    {
        vlc_tick_t default_timeout =
            VLC_TICK_FROM_MS(var_InheritInteger(d->p_intf, "preparse-timeout"));
        if (default_timeout < 0)
            default_timeout = 0;

        const struct vlc_preparser_cfg cfg = [default_timeout]{
            struct vlc_preparser_cfg cfg{};
            cfg.types = VLC_PREPARSER_TYPE_FETCHMETA_ALL;
            cfg.max_parser_threads = 1;
            cfg.timeout = default_timeout;
            cfg.external_process = false;
            return cfg;
        }();
        d->m_preparser = vlc_preparser_New(VLC_OBJECT(d->p_intf), &cfg);
        if (unlikely(d->m_preparser == nullptr))
            return;
    }

    int fetch_options = var_InheritBool( d->p_intf, "metadata-network-access" ) ?
            VLC_PREPARSER_TYPE_FETCHMETA_ALL :
            VLC_PREPARSER_TYPE_FETCHMETA_LOCAL;

    vlc_preparser_req *req =
        vlc_preparser_req_NewParse( d->m_preparser, p_item, fetch_options,
                                    &art_fetcher_cbs, d );
    if (unlikely(req == nullptr))
        return;

    /* This request is never cancelled from here, so submit it and drop the
       handle straight away: on success the running task owns it, on failure
       this release is the one that frees it. */
    vlc_preparser_Submit( d->m_preparser, req );
    vlc_preparser_req_Release( req );
}

void PlayerControllerPrivate::onArtFetchEnded(input_item_t *p_item, bool)
{
    Q_Q(PlayerController);

    vlc_player_locker lock{ m_player };
    bool b_current_item = (p_item == vlc_player_GetCurrentMedia( m_player ));
    /* No input will signal the cover art to update,
         * let's do it ourself */
    if ( b_current_item )
        UpdateArt( p_item );
    else
        emit q->artChanged( p_item );
}

const QString PlayerController::decodeArtURL( input_item_t *p_item )
{
    assert( p_item );

    char *psz_art = input_item_GetArtURL( p_item );
    if( psz_art )
    {
        char *psz = vlc_uri2path( psz_art );
        free( psz_art );
        psz_art = psz;
    }

#if 0
    /* Taglib seems to define a attachment://, It won't work yet */
    url = url.replace( "attachment://", "" );
#endif

    QString path = qfu( psz_art ? psz_art : "" );
    free( psz_art );
    return path;
}

void PlayerController::setArt( input_item_t *p_item, QString fileUrl )
{
    Q_D(PlayerController);
    if( hasInput() )
    {
        char *psz_cachedir = config_GetUserDir( VLC_CACHE_DIR );
        QString old_url = decodeArtURL( p_item );
        old_url = QDir( old_url ).canonicalPath();

        if( psz_cachedir != nullptr && old_url.startsWith( QString::fromUtf8( psz_cachedir ) ) )
            QFile( old_url ).remove(); /* Purge cached artwork */

        free( psz_cachedir );

        input_item_SetArtURL( p_item , fileUrl.toUtf8().constData() );
        d->UpdateArt( p_item );
    }
}

void PlayerController::refreshMediaMeta( input_item_t *p_item )
{
    if (p_item == nullptr)
        return;

    Q_D(PlayerController);

    const bool isCurrentItem = [d, p_item]() {
        vlc_player_locker lock{ d->m_player };
        return p_item == vlc_player_GetCurrentMedia( d->m_player );
    }();

    if (!isCurrentItem)
        return;

    d->UpdateName( p_item );
    d->UpdateArt( p_item );
    d->UpdateMeta( p_item );
    d->UpdateInfo( p_item );
}

bool PlayerController::associateSubtitleFile(const QString &uri)
{
    return AddAssociatedMedia(SPU_ES, uri, true, true, true) == VLC_SUCCESS;
}

int PlayerController::AddAssociatedMedia(es_format_category_e cat, const QString &uri, bool select, bool notify, bool check_ext)
{
    Q_D(PlayerController);
    vlc_player_locker lock{ d->m_player };
    return vlc_player_AddAssociatedMedia( d->m_player, cat, qtu(uri), select, notify, check_ext );
}

#define QABSTRACTLIST_GETTER( type, fun, var ) \
    type* PlayerController::fun() \
    { \
        Q_D(PlayerController); \
        return &d->var; \
    }


QABSTRACTLIST_GETTER( TrackListModel, getVideoTracks, m_videoTracks)
QABSTRACTLIST_GETTER( TrackListModel, getAudioTracks, m_audioTracks)
QABSTRACTLIST_GETTER( TrackListModel, getSubtitleTracks, m_subtitleTracks)
QABSTRACTLIST_GETTER( TitleListModel, getTitles, m_titleList)
QABSTRACTLIST_GETTER( ChapterListModel,getChapters, m_chapterList)
QABSTRACTLIST_GETTER( AudioDeviceModel, getAudioDevices, m_audioDeviceList)
QABSTRACTLIST_GETTER( ProgramListModel, getPrograms, m_programList)
QABSTRACTLIST_GETTER( VLCVarChoiceModel, getVideoStereoMode, m_videoStereoMode)
QABSTRACTLIST_GETTER( VLCVarChoiceModel, getZoom, m_zoom)
QABSTRACTLIST_GETTER( VLCVarChoiceModel, getAspectRatio, m_aspectRatio)
QABSTRACTLIST_GETTER( VLCVarChoiceModel, getCrop, m_crop)
QABSTRACTLIST_GETTER( VLCVarChoiceModel, getFit, m_fit)
QABSTRACTLIST_GETTER( VLCVarChoiceModel, getDeinterlace, m_deinterlace)
QABSTRACTLIST_GETTER( VLCVarChoiceModel, getDeinterlaceMode, m_deinterlaceMode)
QABSTRACTLIST_GETTER( VLCVarChoiceModel, getAudioStereoMode, m_audioStereoMode)
QABSTRACTLIST_GETTER( VLCVarChoiceModel, getAudioMixMode, m_audioMixMode)
QABSTRACTLIST_GETTER( VLCVarChoiceModel, getAudioVisualizations, m_audioVisualization)
QABSTRACTLIST_GETTER( RendererManager, getRendererManager, m_rendererManager)

#undef QABSTRACTLIST_GETTER

#define PRIMITIVETYPE_GETTER( type, fun, var ) \
    type PlayerController::fun() const \
    { \
        Q_D(const PlayerController); \
        return d->var; \
    }


PRIMITIVETYPE_GETTER(PlayerController::PlayingState, getPlayingState, m_playing_status)
PRIMITIVETYPE_GETTER(QString, getName, m_name)
PRIMITIVETYPE_GETTER(VLCTime, getTime, m_time)
PRIMITIVETYPE_GETTER(bool, isLive, m_player_time.live)
VLCTime PlayerController::getDisplayTime() const
{
    Q_D(const PlayerController);
    return VLCTime(vlc::qt::playerTimelineTime(d->m_player_time,
        d->m_time.toVLCTick(), d->m_position, d->m_capabilities & VLC_PLAYER_CAP_SEEK));
}
PRIMITIVETYPE_GETTER(VLCDuration, getRemainingTime, m_remainingTime)
PRIMITIVETYPE_GETTER(double, getPosition, m_position)
PRIMITIVETYPE_GETTER(VLCDuration, getLength, m_length)
PRIMITIVETYPE_GETTER(VLCDuration, getAudioDelay, m_audioDelay)
PRIMITIVETYPE_GETTER(VLCDuration, getSubtitleDelay, m_subtitleDelay)
VLCDuration PlayerController::getSecondarySubtitleDelay() const
{
    Q_D(const PlayerController);
    if (!d->m_secondarySpuEsId)
        return VLCDuration(0);
    return d->m_secondarySubtitleDelay.valid() ? d->m_secondarySubtitleDelay : d->m_subtitleDelay;
}
bool PlayerController::hasSecondarySubtitle() const
{
    Q_D(const PlayerController);
    return d->m_secondarySpuEsId != nullptr;
}
PRIMITIVETYPE_GETTER(bool, isSeekable, m_capabilities & VLC_PLAYER_CAP_SEEK)
PRIMITIVETYPE_GETTER(bool, isRewindable, m_capabilities & VLC_PLAYER_CAP_REWIND)
PRIMITIVETYPE_GETTER(bool, isPausable, m_capabilities & VLC_PLAYER_CAP_PAUSE)
PRIMITIVETYPE_GETTER(bool, isRateChangable, m_capabilities & VLC_PLAYER_CAP_CHANGE_RATE)
PRIMITIVETYPE_GETTER(bool, canRestorePlayback, m_canRestorePlayback);
PRIMITIVETYPE_GETTER(float, getSubtitleFPS, m_subtitleFPS)
PRIMITIVETYPE_GETTER(bool, hasVideoOutput, m_hasVideo)
PRIMITIVETYPE_GETTER(float, getBuffering, m_buffering)
PRIMITIVETYPE_GETTER(float, getVolume, m_volume)
PRIMITIVETYPE_GETTER(bool, isMuted, m_muted)
PRIMITIVETYPE_GETTER(bool, isFullscreen, m_fullscreen)
PRIMITIVETYPE_GETTER(bool, getWallpaperMode, m_wallpaperMode)
PRIMITIVETYPE_GETTER(float, getRate, m_rate)
PRIMITIVETYPE_GETTER(bool, hasTitles, m_hasTitles)
PRIMITIVETYPE_GETTER(bool, hasChapters, m_hasChapters)
PRIMITIVETYPE_GETTER(bool, hasMenu, m_hasMenu)
PRIMITIVETYPE_GETTER(bool, isMenu, m_isMenu)
PRIMITIVETYPE_GETTER(bool, isInteractive, m_isInteractive)
PRIMITIVETYPE_GETTER(bool, hasPrograms, m_hasPrograms)
PRIMITIVETYPE_GETTER(bool, isEncrypted, m_encrypted)
PRIMITIVETYPE_GETTER(bool, isRecording, m_recording)
PRIMITIVETYPE_GETTER(PlayerController::ABLoopState, getABloopState, m_ABLoopState)
PRIMITIVETYPE_GETTER(VLCTime, getABLoopA, m_ABLoopA)
PRIMITIVETYPE_GETTER(VLCTime, getABLoopB, m_ABLoopB)
PRIMITIVETYPE_GETTER(bool, isTeletextEnabled, m_teletextEnabled)
PRIMITIVETYPE_GETTER(bool, isTeletextAvailable, m_teletextAvailable)
PRIMITIVETYPE_GETTER(int, getTeletextPage, m_teletextPage)
PRIMITIVETYPE_GETTER(bool, getTeletextTransparency, m_teletextTransparent)
PRIMITIVETYPE_GETTER(QString, getTitle, m_title)
PRIMITIVETYPE_GETTER(QString, getArtist, m_artist)
PRIMITIVETYPE_GETTER(QString, getAlbum, m_album)
PRIMITIVETYPE_GETTER(QUrl, getArtwork, m_artwork)
PRIMITIVETYPE_GETTER(QUrl, getUrl, m_url)

TimedText PlayerController::getCurrentLyric() const
{
    Q_D(const PlayerController);
    if (d->m_currentLyricIndex < 0)
        return {};
    const auto& lyrics = d->syltLyrics();
    if (d->m_currentLyricIndex >= lyrics.size())
        return {};
    return lyrics[d->m_currentLyricIndex];
}

bool PlayerController::hasLyrics() const
{
    Q_D(const PlayerController);
    return d->m_syltRaw.has_value();
}

QList<TimedText> PlayerController::getSyltLyrics() const
{
    Q_D(const PlayerController);
    return d->syltLyrics();
}

int PlayerController::getCurrentLyricIndex() const
{
    Q_D(const PlayerController);
    return d->m_currentLyricIndex;
}

#undef PRIMITIVETYPE_GETTER
