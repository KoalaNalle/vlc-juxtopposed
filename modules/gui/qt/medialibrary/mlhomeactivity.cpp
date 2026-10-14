/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License, version 2 or later.
 *****************************************************************************/
#include "mlhomeactivity.hpp"
#include "medialib.hpp"
#include "mlhelper.hpp"
#include <QDateTime>
#include <utility>

namespace {
bool visitType(MLItemId id)
{
    return id.id > 0 && (id.type == VLC_ML_PARENT_FOLDER || id.type == VLC_ML_PARENT_PLAYLIST);
}
bool availableFolder(vlc_medialibrary_t *ml, MLItemId id)
{
    ml_unique_ptr<vlc_ml_folder_t> folder(vlc_ml_get_folder(ml, id.id));
    return folder && folder->b_present && !folder->b_banned && folder->psz_mrl;
}
bool availablePlaylist(vlc_medialibrary_t *ml, MLItemId id)
{
    return static_cast<bool>(ml_unique_ptr<vlc_ml_playlist_t>(vlc_ml_get_playlist(ml, id.id)));
}
bool available(vlc_medialibrary_t *ml, MLItemId id)
{
    return id.type == VLC_ML_PARENT_FOLDER ? availableFolder(ml, id) : availablePlaylist(ml, id);
}
}

MLHomeActivity::MLHomeActivity(MediaLib *library, QSettings *settings, qt_intf_t *intf)
    : m_library(library), m_interface(intf), m_store(settings)
{
    m_refreshTimer.setSingleShot(true);
    m_refreshTimer.setInterval(100);
    connect(&m_refreshTimer, &QTimer::timeout, this, &MLHomeActivity::beginRefresh);
    m_listener = m_library->registerEventListener(libraryEvent, this);
    refresh();
}

MLHomeActivity::~MLHomeActivity()
{
    m_refreshTimer.stop();
    if (m_listener)
        m_library->unregisterEventListener(m_listener);
    for (const auto task : std::as_const(m_tasks))
        m_library->cancelMLTask(this, task);
    // Settings are written on accepted visits, never during late shutdown.
}

void MLHomeActivity::setLoading(bool value)
{
    if (m_loading == value)
        return;
    m_loading = value;
    emit loadingChanged();
}

void MLHomeActivity::libraryEvent(void *data, const vlc_ml_event_t *event)
{
    switch (event->i_type)
    {
    case VLC_ML_EVENT_HISTORY_CHANGED:
    case VLC_ML_EVENT_MEDIA_ADDED:
    case VLC_ML_EVENT_MEDIA_UPDATED:
    case VLC_ML_EVENT_MEDIA_DELETED:
    case VLC_ML_EVENT_FOLDER_ADDED:
    case VLC_ML_EVENT_FOLDER_UPDATED:
    case VLC_ML_EVENT_FOLDER_DELETED:
    case VLC_ML_EVENT_PLAYLIST_ADDED:
    case VLC_ML_EVENT_PLAYLIST_UPDATED:
    case VLC_ML_EVENT_PLAYLIST_DELETED:
    case VLC_ML_EVENT_ARTIST_UPDATED:
    case VLC_ML_EVENT_ARTIST_DELETED:
        QMetaObject::invokeMethod(static_cast<MLHomeActivity *>(data), &MLHomeActivity::refresh, Qt::QueuedConnection);
        break;
    default:
        break;
    }
}

void MLHomeActivity::recordVisit(MLItemId id)
{
    if (!visitType(id) || (m_interface && !var_InheritBool(m_interface, "save-recentplay")))
        return;
    struct Context { bool valid = false; };
    const auto epoch = m_visitEpoch;
    const auto task = m_library->runOnMLThread<Context>(this,
        [id](vlc_medialibrary_t *ml, Context &ctx) { ctx.valid = available(ml, id); },
        [this, id, epoch](quint64 taskId, Context &ctx) {
            m_tasks.remove(taskId);
            if (epoch != m_visitEpoch || !ctx.valid || (m_interface && !var_InheritBool(m_interface, "save-recentplay")))
                return;
            if (!m_store.record(id, QDateTime::currentMSecsSinceEpoch()))
            {
                m_persistenceError = tr("Could not save folder or playlist activity.");
                m_error = m_persistenceError;
                emit changed();
            }
            else
                m_persistenceError.clear();
            refresh();
        }, "ML_HOME_VISIT_QUEUE");
    m_tasks.insert(task);
}

void MLHomeActivity::clearVisits()
{
    ++m_visitEpoch;
    if (m_store.clear())
    {
        m_folders.clear();
        m_playlists.clear();
        m_persistenceError.clear();
        m_error.clear();
    }
    else
    {
        m_persistenceError = tr("Could not clear folder or playlist activity.");
        m_error = m_persistenceError;
    }
    emit changed();
    refresh();
}

void MLHomeActivity::refresh()
{
    setLoading(true);
    ++m_generation;
    if (m_refreshRunning)
    {
        m_refreshAgain = true;
        return;
    }
    m_refreshTimer.start();
}

void MLHomeActivity::beginRefresh()
{
    if (m_refreshRunning)
        return;
    m_refreshRunning = true;
    m_refreshAgain = false;
    setLoading(true);
    const auto generation = m_generation;
    const auto visits = m_store.visits();
    using Context = MLHomeActivitySnapshot;
    const auto task = m_library->runOnMLThread<Context>(this,
        [visits](vlc_medialibrary_t *ml, Context &ctx) {
            ctx = loadMLHomeActivity(ml, visits);
        },
        [this, generation](quint64 taskId, Context &ctx) {
            m_tasks.remove(taskId);
            m_refreshRunning = false;
            if (generation == m_generation)
            {
                if (ctx.error.isEmpty())
                {
                    m_folders = std::move(ctx.folders);
                    m_playlists = std::move(ctx.playlists);
                    m_artists = std::move(ctx.artists);
                    m_mediaCount = ctx.mediaCount;
                }
                m_error = ctx.error.isEmpty() ? m_persistenceError : ctx.error;
                emit changed();
            }
            if (m_refreshAgain || generation != m_generation)
                m_refreshTimer.start();
            else
                setLoading(false);
        }, "ML_HOME_REFRESH_QUEUE");
    m_tasks.insert(task);
}
