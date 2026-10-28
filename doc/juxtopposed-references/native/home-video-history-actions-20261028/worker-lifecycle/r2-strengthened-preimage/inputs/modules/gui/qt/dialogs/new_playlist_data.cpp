// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "new_playlist_data.hpp"
#include <vlc_configuration.h>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QSaveFile>

namespace {
QString message(const char *text)
{
    return QCoreApplication::translate("NewPlaylistModel", text);
}

class NativeBackend final : public NewPlaylistBackend
{
public:
    explicit NativeBackend(vlc_medialibrary_t *library) : m_library(library) {}
    qint64 resolve(const QUrl &url) override
    {
        const auto uri = url.toEncoded();
        auto *media = vlc_ml_get_media_by_mrl(m_library, uri.constData());
        if (!media) media = vlc_ml_new_external_media(m_library, uri.constData());
        if (!media) return 0;
        const auto id = media->i_id;
        vlc_ml_media_release(media);
        return id;
    }
    qint64 create(const QString &name) override
    {
        const auto title = name.toUtf8();
        auto *playlist = vlc_ml_playlist_create(m_library, title.constData());
        if (!playlist) return 0;
        const auto id = playlist->i_id;
        vlc_ml_playlist_release(playlist);
        return id;
    }
    bool append(qint64 id, const QVector<int64_t> &media) override
    {
        return media.isEmpty() || vlc_ml_playlist_append(m_library, id, media.constData(), media.size()) == VLC_SUCCESS;
    }
    bool remove(qint64 id) override { return vlc_ml_playlist_delete(m_library, id) == VLC_SUCCESS; }
private:
    vlc_medialibrary_t *m_library;
};
}

QString newPlaylistArtworkDirectory()
{
    char *path = config_GetUserDir(VLC_USERDATA_DIR);
    const auto result = path ? QDir(QString::fromUtf8(path)).filePath("juxtopposed-playlist-art") : QString{};
    free(path);
    return result;
}

QString newPlaylistArtwork(const QString &directory, qint64 id)
{
    if (id <= 0 || directory.isEmpty()) return {};
    const auto path = QDir(directory).filePath(QString::number(id) + ".png");
    return QFileInfo(path).isFile() ? QUrl::fromLocalFile(path).toString() : QString{};
}

bool isLocalPlaylistFile(const QUrl &url)
{
    const auto path = url.toLocalFile();
    // QUrl can decode an authority-free file URL to a UNC/device path. Check
    // the original decoded prefix before Qt strips a Windows device prefix
    // while normalizing separators, and reject relative paths before stat IO.
    if (!url.isValid() || !url.isLocalFile() || !url.host().isEmpty()
        || path.isEmpty() || path.startsWith("//") || path.startsWith("\\\\"))
        return false;
    const auto normalized = QDir::fromNativeSeparators(path);
    // Windows Qt considers C:relative absolute, although Win32 resolves it
    // against that drive's current directory. Require the drive's root slash.
    if (normalized.size() >= 2 && normalized.at(0).isLetter() && normalized.at(1) == ':'
        && (normalized.size() < 3 || normalized.at(2) != '/'))
        return false;
    return !normalized.startsWith("//") && QDir::isAbsolutePath(normalized);
}

QString validateNewPlaylist(const NewPlaylistRequest &request, bool libraryAvailable)
{
    if (!libraryAvailable) return message("Enable the media library to save playlists.");
    if (request.name.trimmed().isEmpty()) return message("Enter a playlist name.");
    if (request.name.size() > 255 || request.name.contains(QChar::Null))
        return message("Use a playlist name of at most 255 characters.");
    if (request.files.size() > 512) return message("Select at most 512 files.");
    for (const auto &url : request.files)
    {
        const QFileInfo file(url.toLocalFile());
        if (!isLocalPlaylistFile(url) || !file.isFile() || !file.isReadable())
            return message("A selected file is missing or unreadable: %1").arg(url.fileName());
    }
    if (!request.thumbnail.isEmpty())
    {
        const QFileInfo file(request.thumbnail.toLocalFile());
        if (!isLocalPlaylistFile(request.thumbnail) || !file.isFile() || !file.isReadable())
            return message("Choose a readable local image for the thumbnail.");
        if (file.size() > 20 * 1024 * 1024) return message("Choose a thumbnail smaller than 20 MB.");
    }
    return {};
}

NewPlaylistResult saveNewPlaylist(NewPlaylistBackend &backend, const NewPlaylistRequest &request, const QString &artworkDirectory)
{
    if (const auto error = validateNewPlaylist(request, true); !error.isEmpty()) return {0, false, false, error};
    QImage artwork;
    if (!request.thumbnail.isEmpty())
    {
        if (artworkDirectory.isEmpty()) return {0, false, false, message("Could not find a folder to save the thumbnail.")};
        QImageReader reader(request.thumbnail.toLocalFile());
        reader.setAutoTransform(true);
        const auto size = reader.size();
        if (!size.isValid() || size.width() > 8192 || size.height() > 8192
            || qint64(size.width()) * size.height() > 16 * 1024 * 1024)
            return {0, false, false, message("Choose a supported image no larger than 16 megapixels.")};
        reader.setScaledSize(size.scaled(1024, 1024, Qt::KeepAspectRatio));
        artwork = reader.read();
        if (artwork.isNull()) return {0, false, false, message("Could not read the chosen thumbnail.")};
    }

    QVector<int64_t> ids;
    for (const auto &url : request.files)
    {
        const auto id = backend.resolve(url);
        if (!id) return {0, false, false, message("Could not add %1 to the media library. No playlist was saved; earlier files may remain in the library.").arg(url.fileName())};
        ids.append(id); // Preserve selection order and intentional repetitions.
    }
    const auto id = backend.create(request.name.trimmed());
    if (!id) return {0, false, false, message("Could not create the playlist. Selected files may remain in the media library.")};
    auto rollback = [&](const QString &error) -> NewPlaylistResult {
        if (backend.remove(id)) return {0, false, false, error};
        return {id, false, true, message("A playlist was saved incompletely. Open it from Playlists before making another copy.")};
    };
    if (!backend.append(id, ids)) return rollback(message("Could not save the files. The incomplete playlist was removed; selected files may remain in the library."));
    const auto targetPath = QDir(artworkDirectory).filePath(QString::number(id) + ".png");
    // ML IDs can be reused after a user resets the database. A new playlist
    // without artwork must not inherit an obsolete copied cover with that ID.
    if (artwork.isNull() && !artworkDirectory.isEmpty() && QFileInfo(targetPath).isFile() && !QFile::remove(targetPath))
        return rollback(message("Could not reset the playlist thumbnail. The playlist was removed."));
    if (!artwork.isNull())
    {
        if (!QDir().mkpath(artworkDirectory)) return rollback(message("Could not save the thumbnail. The playlist was removed."));
        QSaveFile file(targetPath);
        if (!file.open(QIODevice::WriteOnly) || !artwork.save(&file, "PNG") || !file.commit())
            return rollback(message("Could not save the thumbnail. The playlist was removed."));
    }
    return {id, true, true, {}};
}

NewPlaylistResult saveNewPlaylist(vlc_medialibrary_t *library, const NewPlaylistRequest &request, const QString &artworkDirectory)
{
    if (!library) return {0, false, false, message("Enable the media library to save playlists.")};
    NativeBackend backend(library);
    return saveNewPlaylist(backend, request, artworkDirectory);
}
