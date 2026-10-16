// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifndef VLC_QT_NETWORK_STREAM_MODEL_HPP
#define VLC_QT_NETWORK_STREAM_MODEL_HPP
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include <QObject>
#include <QThreadPool>
#include <QUrl>
#include <vlc_playlist.h>
struct vlc_medialibrary_t;

class NetworkStreamModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString urlText READ urlText WRITE setUrlText NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString playlistName READ playlistName WRITE setPlaylistName NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool playlistAvailable READ playlistAvailable CONSTANT FINAL)
    Q_PROPERTY(bool canSubmit READ canSubmit NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged FINAL)
public:
    NetworkStreamModel(vlc_playlist_t *, vlc_medialibrary_t *, QObject *parent = nullptr);
    ~NetworkStreamModel();
    QString urlText() const { return m_text; }
    QString playlistName() const { return m_name; }
    bool playlistAvailable() const { return m_library != nullptr; }
    bool canSubmit() const;
    bool busy() const { return m_busy; }
    QString error() const { return m_error.isEmpty() ? m_validationError : m_error; }
    Q_INVOKABLE void reset();
    Q_INVOKABLE void submit(bool startPlaying = true);
public slots:
    void setUrlText(const QString &);
    void setPlaylistName(const QString &);
signals:
    void stateChanged();
    void finished(bool success);
private:
    void validate();
    void openSources(const QList<QUrl> &, bool, qint64 savedPlaylist = 0);
    void complete(bool, const QString & = {});
    vlc_playlist_t *m_playlist;
    vlc_medialibrary_t *m_library;
    QThreadPool m_pool;
    QString m_text, m_name, m_error, m_validationError;
    QList<QUrl> m_urls;
    bool m_busy = false;
    bool m_committed = false;
};
#endif
