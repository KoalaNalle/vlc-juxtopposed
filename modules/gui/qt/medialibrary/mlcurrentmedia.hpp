// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <QObject>
#include <QSet>
#include <QTimer>
#include <vlc_common.h>
#include <vlc_media_library.h>

class MediaLib;
class PlayerController;
namespace vlc { namespace playlist { class PlaylistController; } }

// Current-input library identity and checked favorite writes. No indexing or
// library enablement is performed here; unknown media stays unavailable.
class MLCurrentMedia : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY changed FINAL)
    Q_PROPERTY(bool favorite READ favorite NOTIFY changed FINAL)
    Q_PROPERTY(bool busy READ busy NOTIFY changed FINAL)
    Q_PROPERTY(QString error READ error NOTIFY changed FINAL)
    Q_PROPERTY(QString playlistName READ playlistName NOTIFY changed FINAL)
public:
    MLCurrentMedia(MediaLib *, PlayerController *, vlc::playlist::PlaylistController *, QObject *parent = nullptr);
    ~MLCurrentMedia() override;
    bool available() const { return m_id > 0; }
    bool favorite() const { return m_favorite; }
    bool busy() const { return m_busy; }
    QString error() const { return m_error; }
    QString playlistName() const { return m_playlistName; }
    Q_INVOKABLE bool setFavorite(bool);
    Q_INVOKABLE void refresh();
signals:
    void changed();
private:
    static void libraryEvent(void *, const vlc_ml_event_t *);
    void sourceChanged();
    void lookup();
    void completed();
    MediaLib *m_library;
    PlayerController *m_player;
    vlc::playlist::PlaylistController *m_playlist;
    vlc_ml_event_callback_t *m_listener = nullptr;
    QSet<quint64> m_tasks;
    QTimer m_refreshTimer;
    QString m_mrl, m_error, m_playlistName;
    qint64 m_sourcePlaylistId = 0;
    quint64 m_generation = 0;
    int64_t m_id = 0;
    bool m_favorite = false, m_busy = false, m_refreshAgain = false;
};
