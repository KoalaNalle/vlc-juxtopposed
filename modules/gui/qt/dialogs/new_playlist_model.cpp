// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "new_playlist_model.hpp"
#include <QDir>
#include <QFileInfo>

NewPlaylistModel::NewPlaylistModel(vlc_medialibrary_t *library, QString artworkDirectory, QObject *parent)
    : QObject(parent), m_library(library), m_artworkDirectory(std::move(artworkDirectory))
{
    m_pool.setMaxThreadCount(1);
    validate();
}

NewPlaylistModel::~NewPlaylistModel()
{
    // DialogsProvider is destroyed before VLC's native library. A pending save
    // finishes in the worker; it owns all request/image data and borrows no UI.
    m_pool.waitForDone();
}

QVariantList NewPlaylistModel::files() const
{
    QVariantList result;
    for (const auto &url : m_request.files)
        result.append(QVariantMap{{"url", url}, {"title", url.fileName()}, {"path", QDir::toNativeSeparators(url.toLocalFile())}});
    return result;
}

bool NewPlaylistModel::canSave() const
{
    return editable() && available() && m_validation.isEmpty();
}

void NewPlaylistModel::validate()
{
    m_validation = validateNewPlaylist(m_request, available());
}

void NewPlaylistModel::setName(const QString &name)
{
    if (!editable() || m_request.name == name) return;
    m_request.name = name;
    m_error.clear();
    validate();
    emit changed();
}

void NewPlaylistModel::setThumbnail(const QUrl &thumbnail)
{
    if (!editable() || m_request.thumbnail == thumbnail) return;
    // The dialog binds this property directly to Image.source. Reject unsafe
    // paths before publishing them, rather than only rejecting a later save.
    if (!thumbnail.isEmpty() && !isLocalPlaylistFile(thumbnail))
    {
        m_error = tr("Choose a readable local image for the thumbnail.");
        emit changed();
        return;
    }
    m_request.thumbnail = thumbnail;
    m_error.clear();
    validate();
    emit changed();
}

bool NewPlaylistModel::appendFiles(const QVariantList &urls)
{
    if (!editable()) return false;
    NewPlaylistRequest next = m_request;
    for (const auto &value : urls)
    {
        const auto url = value.toUrl();
        const QFileInfo file(url.toLocalFile());
        if (!isLocalPlaylistFile(url) || !file.isFile() || !file.isReadable())
        {
            m_error = tr("Select readable local files. Folders and network addresses cannot be added here.");
            emit changed();
            return false;
        }
        next.files.append(url);
    }
    if (next.files.size() > 512)
    {
        m_error = tr("Select at most 512 files.");
        emit changed();
        return false;
    }
    m_request = std::move(next);
    m_error.clear();
    validate();
    emit changed();
    return true;
}

void NewPlaylistModel::removeFile(int index)
{
    if (!editable() || index < 0 || index >= m_request.files.size()) return;
    m_request.files.removeAt(index);
    m_error.clear();
    validate();
    emit changed();
}

void NewPlaylistModel::moveFile(int index, int destination)
{
    if (!editable() || index < 0 || destination < 0 || index >= m_request.files.size() || destination >= m_request.files.size()) return;
    m_request.files.move(index, destination);
    m_error.clear();
    validate();
    emit changed();
}

void NewPlaylistModel::reset()
{
    if (m_busy) return;
    m_request = {};
    m_error.clear();
    m_committed = m_success = false;
    m_savedId = 0;
    validate();
    emit changed();
}

void NewPlaylistModel::save()
{
    if (!canSave()) return;
    m_busy = true;
    m_error.clear();
    emit changed();
    const auto request = m_request;
    const auto library = m_library;
    const auto directory = m_artworkDirectory;
    m_pool.start([this, library, request, directory] {
        const auto result = saveNewPlaylist(library, request, directory);
        QMetaObject::invokeMethod(this, [this, result] {
            m_busy = false;
            m_savedId = result.id;
            m_committed = result.persisted;
            m_success = result.success;
            m_error = result.error;
            emit changed();
            emit finished(result.success);
        }, Qt::QueuedConnection);
    });
}
