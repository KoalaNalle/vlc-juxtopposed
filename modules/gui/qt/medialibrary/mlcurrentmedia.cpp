// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "mlcurrentmedia.hpp"
#include "medialib.hpp"
#include "mlhelper.hpp"
#include "player/player_controller.hpp"
#include "playlist/playlist_controller.hpp"
#include <utility>

namespace {
constexpr char favoriteQueue[] = "current-media-favorite";
struct Result {
    int64_t id = 0;
    bool favorite = false, saved = false;
    QString playlistName;
};
void readMedia(vlc_medialibrary_t *library, const QString &mrl, Result &result)
{
    ml_unique_ptr<vlc_ml_media_t> media{vlc_ml_get_media_by_mrl(library, qtu(mrl))};
    if (media) {
        result.id = media->i_id;
        result.favorite = media->b_is_favorite;
    }
}
}

MLCurrentMedia::MLCurrentMedia(MediaLib *library, PlayerController *player,
                               vlc::playlist::PlaylistController *playlist, QObject *parent)
    : QObject(parent), m_library(library), m_player(player), m_playlist(playlist)
{
    m_refreshTimer.setSingleShot(true);
    m_refreshTimer.setInterval(30);
    connect(&m_refreshTimer, &QTimer::timeout, this, &MLCurrentMedia::lookup);
    connect(player, &PlayerController::inputChanged, this, &MLCurrentMedia::sourceChanged);
    connect(playlist, &vlc::playlist::PlaylistController::currentSourcePlaylistChanged,
            this, &MLCurrentMedia::sourceChanged);
    if (library)
        m_listener = library->registerEventListener(libraryEvent, this);
    sourceChanged();
}

MLCurrentMedia::~MLCurrentMedia()
{
    if (m_listener) m_library->unregisterEventListener(m_listener);
    for (auto task : std::as_const(m_tasks)) m_library->cancelMLTask(this, task);
}

void MLCurrentMedia::sourceChanged()
{
    ++m_generation;
    m_refreshTimer.stop();
    // The player retains its last URL after releasing the input.
    const auto input = m_player->getInput();
    const auto url = m_player->getUrl();
    const auto selected = m_playlist->getCurrentItem();
    m_mrl = input ? url.toString(QUrl::FullyEncoded) : QString{};
    // Player and queue notifications are independent. Do not label the new
    // player input with the old queue selection while those callbacks settle.
    m_sourcePlaylistId = input && selected.inputItem() == input && selected.getUrl() == url
        ? m_playlist->currentSourcePlaylistId() : 0;
    m_playlistName.clear();
    m_id = 0;
    m_favorite = m_busy = m_refreshAgain = false;
    m_error.clear();
    emit changed();
    refresh();
}

void MLCurrentMedia::refresh()
{
    if (!m_library || m_mrl.isEmpty()) return;
    if (m_busy) m_refreshAgain = true;
    else m_refreshTimer.start();
}

void MLCurrentMedia::completed()
{
    m_busy = false;
    emit changed();
    if (std::exchange(m_refreshAgain, false)) refresh();
}

void MLCurrentMedia::lookup()
{
    if (!m_library || m_mrl.isEmpty()) return;
    if (m_busy) { m_refreshAgain = true; return; }
    m_busy = true;
    emit changed();
    const auto generation = m_generation;
    const auto mrl = m_mrl;
    const auto sourcePlaylist = m_sourcePlaylistId;
    const auto task = m_library->runOnMLThread<Result>(this,
        [mrl, sourcePlaylist](vlc_medialibrary_t *library, Result &result) {
            readMedia(library, mrl, result);
            if (sourcePlaylist > 0) {
                ml_unique_ptr<vlc_ml_playlist_t> playlist{vlc_ml_get_playlist(library, sourcePlaylist)};
                if (playlist) result.playlistName = QString::fromUtf8(playlist->psz_name);
            }
        },
        [this, generation](quint64 task, Result &result) {
            m_tasks.remove(task);
            if (generation != m_generation) return;
            m_id = result.id;
            m_favorite = result.favorite;
            m_playlistName = result.playlistName;
            completed();
        }, favoriteQueue);
    m_tasks.insert(task);
}

bool MLCurrentMedia::setFavorite(bool desired)
{
    if (!available() || m_busy || !m_library) return false;
    if (desired == m_favorite) return true;
    m_refreshTimer.stop();
    m_busy = true;
    m_error.clear();
    emit changed();
    const auto generation = m_generation;
    const auto mrl = m_mrl;
    const auto id = m_id;
    const auto task = m_library->runOnMLThread<Result>(this,
        [mrl, id, desired](vlc_medialibrary_t *library, Result &result) {
            // Resolve the URL again before the mutation, rather than trusting a
            // stale numeric ID after a library reset or a source change.
            readMedia(library, mrl, result);
            if (result.id != id) return;
            const auto status = vlc_ml_media_set_favorite(library, id, desired);
            result.id = 0;
            readMedia(library, mrl, result);
            result.saved = status == VLC_SUCCESS && result.id == id && result.favorite == desired;
        },
        [this, generation](quint64 task, Result &result) {
            m_tasks.remove(task);
            if (generation != m_generation) return;
            m_id = result.id;
            m_favorite = result.id > 0 && result.favorite;
            if (!result.saved) m_error = tr("Could not save the favorite. Try again.");
            completed();
        }, favoriteQueue);
    m_tasks.insert(task);
    return true;
}

void MLCurrentMedia::libraryEvent(void *data, const vlc_ml_event_t *event)
{
    auto *self = static_cast<MLCurrentMedia *>(data);
    if (event->i_type == VLC_ML_EVENT_PLAYLIST_DELETED) {
        const auto id = event->deletion.i_entity_id;
        QMetaObject::invokeMethod(self, [self, id] { self->m_playlist->forgetSourcePlaylist(id); }, Qt::QueuedConnection);
        return;
    }
    if (event->i_type == VLC_ML_EVENT_PLAYLIST_UPDATED) {
        const auto id = event->modification.i_entity_id;
        QMetaObject::invokeMethod(self, [self, id] {
            if (self->m_sourcePlaylistId == id) self->refresh();
        }, Qt::QueuedConnection);
        return;
    }
    int64_t id = 0;
    switch (event->i_type) {
    case VLC_ML_EVENT_MEDIA_ADDED: break;
    case VLC_ML_EVENT_MEDIA_UPDATED: id = event->modification.i_entity_id; break;
    case VLC_ML_EVENT_MEDIA_DELETED: id = event->deletion.i_entity_id; break;
    case VLC_ML_EVENT_FAVORITES_CHANGED:
        if (event->favorites_changed.i_entity_type != VLC_ML_PARENT_UNKNOWN) return;
        id = event->favorites_changed.i_entity_id;
        break;
    default: return;
    }
    QMetaObject::invokeMethod(self, [self, id] {
        if (!id || !self->m_id || self->m_id == id) self->refresh();
    }, Qt::QueuedConnection);
}
