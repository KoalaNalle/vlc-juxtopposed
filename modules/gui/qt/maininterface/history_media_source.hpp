// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QObject>
#include <QString>
#include <QThreadPool>
#include <memory>
struct qt_intf_t;
class MediaLib;
Q_MOC_INCLUDE("medialibrary/medialib.hpp")
namespace vlc { namespace playlist { class PlaylistController; } }

// The capability probe never opens storage. Only explicit open() initializes
// the shared history backend, independently of Library browsing/indexing.
class HistoryMediaSource : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available CONSTANT FINAL)
    Q_PROPERTY(MediaLib *source READ source NOTIFY changed FINAL)
    Q_PROPERTY(bool busy READ busy NOTIFY changed FINAL)
    Q_PROPERTY(QString error READ error NOTIFY changed FINAL)
public:
    HistoryMediaSource(qt_intf_t *, vlc::playlist::PlaylistController *, MediaLib *browsing, QObject *parent = nullptr);
    ~HistoryMediaSource() override;
    bool available() const { return m_available; }
    MediaLib *source() const { return m_source; }
    bool busy() const { return m_busy; }
    QString error() const { return m_error; }
    Q_INVOKABLE void open();
signals:
    void changed();
    void historyClearRequested();
private:
    qt_intf_t *m_intf;
    vlc::playlist::PlaylistController *m_playlistController;
    const bool m_available;
    MediaLib *m_source;
    bool m_busy = false;
    QString m_error;
    QThreadPool m_openingPool;
    std::unique_ptr<MediaLib> m_ownedSource;
};
