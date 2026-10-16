// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "network_stream_model.hpp"
#include "util/shared_input_item.hpp"
#include <vlc_media_library.h>
#include <QRegularExpression>
#include <QVector>

namespace {
struct SavedPlaylist { qint64 id = 0; QString error; bool persisted = false; };
SavedPlaylist savePlaylist(vlc_medialibrary_t *library, const QString &name,
                           const QList<QUrl> &urls)
{
    QVector<int64_t> ids;
    for (const auto &url : urls)
    {
        const auto uri = url.toEncoded();
        auto *media = vlc_ml_get_media_by_mrl(library, uri.constData());
        if (!media) media = vlc_ml_new_external_media(library, uri.constData());
        if (!media) return {0, NetworkStreamModel::tr("Could not save a stream in the media library.")};
        ids.append(media->i_id);
        vlc_ml_media_release(media);
    }
    const auto title = name.toUtf8();
    auto *playlist = vlc_ml_playlist_create(library, title.constData());
    if (!playlist) return {0, NetworkStreamModel::tr("Could not create the playlist.")};
    const auto id = playlist->i_id;
    vlc_ml_playlist_release(playlist);
    if (vlc_ml_playlist_append(library, id, ids.constData(), ids.size()) != VLC_SUCCESS)
    {
        const bool removed = vlc_ml_playlist_delete(library, id) == VLC_SUCCESS;
        return {0, removed ? NetworkStreamModel::tr("Could not save the playlist's streams.")
                           : NetworkStreamModel::tr("Could not save the streams. Check Playlists before retrying."), !removed};
    }
    return {id, {}, true};
}
}

NetworkStreamModel::NetworkStreamModel(vlc_playlist_t *playlist,
                                     vlc_medialibrary_t *library, QObject *parent)
    : QObject(parent), m_playlist(playlist), m_library(library)
{
    m_pool.setMaxThreadCount(1);
}

NetworkStreamModel::~NetworkStreamModel()
{
    // A submitted save completes before the native library/playlist are released.
    m_pool.waitForDone();
}

bool NetworkStreamModel::canSubmit() const
{
    return m_playlist && !m_busy && !m_committed && !m_urls.isEmpty()
        && m_validationError.isEmpty() && (m_name.trimmed().isEmpty() || m_library);
}

void NetworkStreamModel::validate()
{
    m_urls.clear();
    m_validationError.clear();
    if (m_text.size() > 65536)
    {
        m_validationError = tr("The stream address list is too long.");
        return;
    }
    const auto lines = m_text.split(QRegularExpression("\\r\\n|\\n|\\r"));
    int lineNumber = 0;
    for (const auto &line : lines)
    {
        ++lineNumber;
        const auto value = line.trimmed();
        if (value.isEmpty()) continue;
        const QUrl url(value, QUrl::StrictMode);
        const auto scheme = url.scheme().toLower();
        const bool hostRequired = QStringList{"http", "https", "rtsp", "rtsps", "rtmp",
            "rtmps", "ftp", "ftps", "sftp", "smb", "nfs"}.contains(scheme);
        if (!url.isValid() || url.isRelative() || url.isLocalFile()
            || !value.contains("://") || (hostRequired && url.host().isEmpty())
            || (url.host().isEmpty() && url.path().isEmpty() && url.port() < 0))
        {
            m_validationError = tr("Enter a complete network URL on line %1.").arg(lineNumber);
            m_urls.clear();
            return;
        }
        m_urls.append(url);
        if (m_urls.size() > 512)
        {
            m_urls.clear();
            m_validationError = tr("Enter at most 512 stream addresses.");
            return;
        }
    }
    if (!m_name.trimmed().isEmpty() && !m_library)
        m_validationError = tr("The media library must be enabled to save a named playlist.");
}

void NetworkStreamModel::setUrlText(const QString &text)
{
    if (m_busy || m_text == text) return;
    m_text = text;
    m_committed = false;
    m_error.clear();
    validate();
    emit stateChanged();
}

void NetworkStreamModel::setPlaylistName(const QString &name)
{
    if (m_busy || m_name == name) return;
    m_name = name;
    m_committed = false;
    m_error.clear();
    validate();
    emit stateChanged();
}

void NetworkStreamModel::reset()
{
    if (m_busy) return;
    m_text.clear();
    m_name.clear();
    m_urls.clear();
    m_error.clear();
    m_validationError.clear();
    m_committed = false;
    emit stateChanged();
}

void NetworkStreamModel::complete(bool success, const QString &error)
{
    m_busy = false;
    m_error = error;
    emit stateChanged();
    emit finished(success);
}

void NetworkStreamModel::submit(bool startPlaying)
{
    if (!canSubmit()) return;
    m_busy = true;
    m_error.clear();
    emit stateChanged();
    const auto urls = m_urls;
    const auto name = m_name.trimmed();
    if (name.isEmpty()) { openSources(urls, startPlaying); return; }
    const auto library = m_library;
    m_pool.start([this, library, urls, name, startPlaying] {
        const auto saved = savePlaylist(library, name, urls);
        QMetaObject::invokeMethod(this, [this, saved, urls, startPlaying] {
            if (!saved.id)
            {
                m_committed = saved.persisted;
                complete(false, saved.error);
            }
            else openSources(urls, startPlaying, saved.id);
        }, Qt::QueuedConnection);
    });
}

void NetworkStreamModel::openSources(const QList<QUrl> &urls, bool startPlaying,
                                    qint64 savedPlaylist)
{
    QVector<SharedInputItem> held;
    QVector<input_item_t *> items;
    bool added = false;
    for (const auto &url : urls)
    {
        const auto uri = url.toEncoded();
        auto *item = input_item_New(uri.constData(), nullptr);
        if (!item) break;
        held.append(SharedInputItem(item, false));
        items.append(item);
    }
    int playback = VLC_SUCCESS;
    if (items.size() == urls.size())
    {
        vlc_playlist_Lock(m_playlist);
        const auto first = vlc_playlist_Count(m_playlist);
        added = vlc_playlist_Append(m_playlist, items.constData(), items.size()) == VLC_SUCCESS;
        if (added && startPlaying)
        {
            playback = vlc_playlist_GoTo(m_playlist, first);
            if (playback == VLC_SUCCESS) playback = vlc_playlist_Start(m_playlist);
        }
        vlc_playlist_Unlock(m_playlist);
    }
    if (!added)
    {
        if (!savedPlaylist) { complete(false, tr("Could not add the streams to the queue.")); return; }
        const auto library = m_library;
        m_pool.start([this, library, savedPlaylist] {
            const bool removed = vlc_ml_playlist_delete(library, savedPlaylist) == VLC_SUCCESS;
            QMetaObject::invokeMethod(this, [this, removed] {
                m_committed = !removed;
                complete(false, removed ? tr("Could not add the streams to the queue.")
                    : tr("The playlist was saved, but the streams could not be queued. Open it from Playlists."));
            }, Qt::QueuedConnection);
        });
        return;
    }
    m_committed = true; // A repeated activation must not add the sources twice.
    complete(playback == VLC_SUCCESS, playback == VLC_SUCCESS ? QString{}
        : tr("Streams were added, but playback could not start. Open them from the queue."));
}
