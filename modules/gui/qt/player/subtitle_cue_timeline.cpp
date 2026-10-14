// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "subtitle_cue_timeline.hpp"
#include "qt.hpp"
#include <vlc_cxx_helpers.hpp>
using vlc_player_locker = vlc_locker<vlc_player_t, vlc_player_Lock, vlc_player_Unlock>;
#include <QVariantList>

SubtitleCueTimeline::SubtitleCueTimeline(vlc_player_t *player, QObject *parent)
    : QObject(parent), m_player(player)
{
    m_pool.setMaxThreadCount(1);
    m_poll.setInterval(100);
    connect(&m_poll, &QTimer::timeout, this, &SubtitleCueTimeline::refresh);
    if (m_player)
    {
        static const auto callbacks = [] {
            vlc_player_cbs cbs{};
            cbs.on_state_changed = [](vlc_player_t *, vlc_player_state, void *data) { static_cast<SubtitleCueTimeline *>(data)->invalidateNative(); };
            cbs.on_error_changed = [](vlc_player_t *, vlc_player_error, void *data) { static_cast<SubtitleCueTimeline *>(data)->invalidateNative(); };
            cbs.on_capabilities_changed = [](vlc_player_t *, int, int, void *data) { static_cast<SubtitleCueTimeline *>(data)->invalidateNative(); };
            cbs.on_current_media_changed = [](vlc_player_t *, input_item_t *, void *data) { static_cast<SubtitleCueTimeline *>(data)->invalidateNative(); };
            cbs.on_track_list_changed = [](vlc_player_t *, vlc_player_list_action, const vlc_player_track *track, void *data) { if (track->fmt.i_cat == SPU_ES) static_cast<SubtitleCueTimeline *>(data)->invalidateNative(); };
            cbs.on_track_selection_changed = [](vlc_player_t *, vlc_es_id_t *old, vlc_es_id_t *selected, void *data) { if ((old && vlc_es_id_GetCat(old) == SPU_ES) || (selected && vlc_es_id_GetCat(selected) == SPU_ES)) static_cast<SubtitleCueTimeline *>(data)->invalidateNative(); };
            cbs.on_category_delay_changed = [](vlc_player_t *, es_format_category_e cat, vlc_tick_t, void *data) { if (cat == SPU_ES) static_cast<SubtitleCueTimeline *>(data)->invalidateNative(); };
            cbs.on_track_delay_changed = [](vlc_player_t *, vlc_es_id_t *id, vlc_tick_t, void *data) { if (vlc_es_id_GetCat(id) == SPU_ES) static_cast<SubtitleCueTimeline *>(data)->invalidateNative(); };
            cbs.on_associated_subs_fps_changed = [](vlc_player_t *, float, void *data) { static_cast<SubtitleCueTimeline *>(data)->invalidateNative(); };
            cbs.on_vout_changed = [](vlc_player_t *, vlc_player_vout_action, vout_thread_t *, vlc_vout_order, vlc_es_id_t *, void *data) { static_cast<SubtitleCueTimeline *>(data)->invalidateNative(); };
            return cbs;
        }();
        vlc_player_locker guard(m_player);
        m_listener = vlc_player_AddListener(m_player, &callbacks, this);
    }
}

SubtitleCueTimeline::~SubtitleCueTimeline()
{
    m_poll.stop();
    if (m_listener)
    {
        vlc_player_locker guard(m_player);
        vlc_player_RemoveListener(m_player, m_listener);
    }
    if (m_cancel) m_cancel->store(true);
    m_pool.waitForDone();
}

void SubtitleCueTimeline::invalidateNative()
{
    // Native callbacks run under the player lock. Invalidate immediately,
    // before queued GUI refresh, so stale worker data cannot be published.
    ++m_nativeEpoch;
    if (!m_refreshQueued.exchange(true))
        QMetaObject::invokeMethod(this, "refresh", Qt::QueuedConnection);
}

QByteArray SubtitleCueTimeline::key(const SubtitleCueSource &source)
{
    return QByteArray::number(static_cast<qulonglong>(source.mediaToken)) + '\0' + source.mediaUri + '\0' + source.trackId + '\0' + source.exactUri;
}

void SubtitleCueTimeline::clearCues()
{
    m_available = false;
    m_loadedKey.clear();
    m_cues.clear();
    ++m_revision;
    emit cuesChanged();
}

void SubtitleCueTimeline::setEnabled(bool enabled)
{
    if (m_enabled == enabled) return;
    m_enabled = enabled;
    ++m_nativeEpoch;
    if (enabled) m_poll.start();
    else m_poll.stop();
    refresh();
}

void SubtitleCueTimeline::reload()
{
    ++m_nativeEpoch;
    if (m_cancel) m_cancel->store(true);
    clearCues();
    refresh();
}

QVariantMap SubtitleCueTimeline::view(qint64 startTicks, qint64 endTicks) const
{
    QVariantList segments;
    bool dense = false;
    if (available() && endTicks > startTicks)
        for (qsizetype index = 0; index < m_cues.size(); ++index)
        {
            const auto &cue = m_cues[index];
            const auto start = cue.start + m_snapshot.delay;
            const auto end = cue.end + m_snapshot.delay;
            if (end <= startTicks || start >= endTicks) continue;
            if (segments.size() == 512) { segments.clear(); dense = true; break; }
            segments.append(QVariantMap{{"startTicks", start}, {"endTicks", end},
                {"sourceStartTicks", cue.start}, {"sourceEndTicks", cue.end}, {"text", cue.text},
                {"cueIndex", int(index)}, {"revision", m_revision},
                {"seekable", m_snapshot.canSeek && start >= 0 && m_snapshot.length > 0 && start < m_snapshot.length}});
        }
    return {{"segments", segments}, {"tooDense", dense}};
}

bool SubtitleCueTimeline::seekCue(int cueIndex, int requestedRevision)
{
    if (!m_enabled || !available() || m_reading || !m_player
        || requestedRevision != m_revision || cueIndex < 0 || cueIndex >= m_cues.size())
        return false;
    vlc_player_locker guard(m_player);
    const auto epoch = m_nativeEpoch.load();
    // The helper is explicitly already-locked: normal player locks must never
    // recurse while validating the same input and requesting its seek.
    const auto live = selectedExternalSrtSourceLocked(m_player);
    if (epoch != m_validatedEpoch || !live.available || !live.canSeek
        || live.length <= 0 || key(live) != m_loadedKey
        || live.delay != m_snapshot.delay || epoch != m_nativeEpoch.load())
        return false;
    const auto start = m_cues[cueIndex].start + live.delay;
    if (start < 0 || start >= live.length) return false;
    vlc_player_SetTime(m_player, start);
    return true; // guarded native request issued; completion remains asynchronous
}

bool SubtitleCueTimeline::setDelayTicks(qint64 requestedDelay, int requestedRevision)
{
    const auto limit = VLC_TICK_FROM_SEC(30 * 24 * 60 * 60);
    if (!m_enabled || !available() || m_reading || !m_player
        || requestedRevision != m_revision || requestedDelay < -limit || requestedDelay > limit)
        return false;
    vlc_player_locker guard(m_player);
    const auto epoch = m_nativeEpoch.load();
    const auto live = selectedExternalSrtSourceLocked(m_player);
    if (epoch != m_validatedEpoch || !live.available || !live.canSeek
        || live.length <= 0 || key(live) != m_loadedKey
        || live.delay != m_snapshot.delay || epoch != m_nativeEpoch.load())
        return false;
    const auto *track = vlc_player_GetSelectedTrack(m_player, SPU_ES);
    if (!track || QByteArray(vlc_es_id_GetStrId(track->es_id)) != live.trackId)
        return false;
    if (vlc_player_GetEsIdDelay(m_player, track->es_id) == INT64_MAX)
        vlc_player_SetSubtitleDelay(m_player, requestedDelay, VLC_PLAYER_WHENCE_ABSOLUTE);
    else if (vlc_player_SetEsIdDelay(m_player, track->es_id, requestedDelay,
                                   VLC_PLAYER_WHENCE_ABSOLUTE) != VLC_SUCCESS)
        return false;
    // Native callbacks invalidate the displayed revision immediately. Its
    // coalesced refresh publishes state after this nonrecursive lock is gone.
    return true;
}

void SubtitleCueTimeline::refresh()
{
    m_refreshQueued.store(false);
    if (!m_enabled)
    {
        if (m_cancel) m_cancel->store(true);
        m_available = false;
        m_snapshot = {};
        m_status.clear();
        if (!m_loadedKey.isEmpty() || !m_cues.isEmpty()) clearCues();
        emit stateChanged();
        emit positionChanged();
        return;
    }
    if (!m_player || !m_listener)
    {
        m_available = false;
        m_status = tr("Subtitle timeline is unavailable.");
        emit stateChanged();
        return;
    }
    SubtitleCueSource snapshot;
    quint64 epoch;
    {
        vlc_player_locker guard(m_player);
        snapshot = selectedExternalSrtSourceLocked(m_player);
        epoch = m_nativeEpoch.load();
    }
    const auto sourceKey = key(snapshot);
    const bool epochChanged = epoch != m_validatedEpoch;
    const bool seekability = snapshot.canSeek != m_snapshot.canSeek;
    const bool shifted = snapshot.delay != m_snapshot.delay;
    const bool position = snapshot.position != m_snapshot.position || snapshot.length != m_snapshot.length;
    const bool changed = snapshot.available != m_snapshot.available || snapshot.status != m_snapshot.status
        || snapshot.exactUri != m_snapshot.exactUri || snapshot.trackId != m_snapshot.trackId
        || snapshot.mediaToken != m_snapshot.mediaToken || shifted || epochChanged || seekability;
    m_snapshot = snapshot;
    m_validatedEpoch = epoch;
    if (position) emit positionChanged();
    if (shifted || epochChanged || seekability) { ++m_revision; emit cuesChanged(); }
    if (!snapshot.available)
    {
        if (m_cancel) m_cancel->store(true);
        m_available = false;
        m_status = snapshot.status;
        if (!m_loadedKey.isEmpty() || !m_cues.isEmpty()) clearCues();
        if (changed) emit stateChanged();
        return;
    }
    if (sourceKey == m_loadedKey)
    {
        if (changed) emit stateChanged();
        return;
    }
    m_available = false;
    if (!m_loadedKey.isEmpty() || !m_cues.isEmpty()) clearCues();
    m_status = tr("Reading the selected SRT file…");
    if (m_reading)
    {
        if (m_readKey != sourceKey && m_cancel) m_cancel->store(true);
        if (changed) emit stateChanged();
        return;
    }
    m_reading = true;
    m_readKey = sourceKey;
    m_cancel = std::make_shared<std::atomic_bool>(false);
    const auto cancel = m_cancel;
    const auto uri = snapshot.uri;
    emit stateChanged();
    // The worker owns only URI/data/token values; it never touches the native
    // player, current item, track, delay, playback state or native pointers.
    m_pool.start([this, uri, sourceKey, epoch, cancel] {
        auto result = readExternalSrtCues(uri, cancel);
        QMetaObject::invokeMethod(this, [this, result = std::move(result), sourceKey, epoch, cancel]() mutable {
            m_reading = false;
            if (!m_enabled || cancel->load() || result.canceled || epoch != m_nativeEpoch.load()) { refresh(); return; }
            // Duration-filter changes may not emit a player callback. Recheck
            // the actual selected source/timing state before publishing cues.
            bool accepted = false;
            {
                vlc_player_locker guard(m_player);
                const auto live = selectedExternalSrtSourceLocked(m_player);
                if (epoch == m_nativeEpoch.load() && live.available && key(live) == sourceKey)
                {
                    m_snapshot = live;
                    m_validatedEpoch = epoch;
                    m_loadedKey = sourceKey;
                    m_cues = std::move(result.cues);
                    m_available = result.error.isEmpty();
                    m_status = !result.error.isEmpty() ? result.error : m_cues.isEmpty()
                        ? tr("The selected SRT file contains no cues.")
                        : tr("Source cue snapshot with the current subtitle delay.");
                    accepted = true;
                }
            }
            if (!accepted) { refresh(); return; }
            ++m_revision;
            emit cuesChanged();
            emit stateChanged();
            emit positionChanged();
        }, Qt::QueuedConnection);
    });
}
