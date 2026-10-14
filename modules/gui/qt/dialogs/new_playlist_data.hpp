// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#pragma once
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include <vlc_media_library.h>
#include <QList>
#include <QString>
#include <QUrl>
#include <QVector>

struct NewPlaylistRequest
{
    QString name;
    QList<QUrl> files;
    QUrl thumbnail;
};

struct NewPlaylistResult
{
    qint64 id = 0;
    bool success = false;
    bool persisted = false;
    QString error;
};

// This boundary also lets failure tests exercise the production rollback path.
// Save never takes a playback queue or invokes playback APIs.
class NewPlaylistBackend
{
public:
    virtual ~NewPlaylistBackend() = default;
    virtual qint64 resolve(const QUrl &) = 0;
    virtual qint64 create(const QString &) = 0;
    virtual bool append(qint64, const QVector<int64_t> &) = 0;
    virtual bool remove(qint64) = 0;
};

QString newPlaylistArtworkDirectory();
bool isLocalPlaylistFile(const QUrl &);
QString newPlaylistArtwork(const QString &directory, qint64 id);
QString validateNewPlaylist(const NewPlaylistRequest &, bool libraryAvailable);
NewPlaylistResult saveNewPlaylist(NewPlaylistBackend &, const NewPlaylistRequest &, const QString &artworkDirectory);
NewPlaylistResult saveNewPlaylist(vlc_medialibrary_t *, const NewPlaylistRequest &, const QString &artworkDirectory);
