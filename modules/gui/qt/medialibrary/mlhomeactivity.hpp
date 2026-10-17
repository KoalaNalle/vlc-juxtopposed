/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License, version 2 or later.
 *****************************************************************************/
#pragma once
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "mlhomeactivitydata.hpp"

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QSettings>
#include <QTimer>
#include <QVariantList>
#include <atomic>

class MediaLib;
struct qt_intf_t;

class MLHomeActivity : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList folders READ folders NOTIFY changed FINAL)
    Q_PROPERTY(QVariantList playlists READ playlists NOTIFY changed FINAL)
    Q_PROPERTY(QVariantList artists READ artists NOTIFY changed FINAL)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged FINAL)
    Q_PROPERTY(QString error READ error NOTIFY changed FINAL)
    Q_PROPERTY(int mediaCount READ mediaCount NOTIFY changed FINAL)
public:
    MLHomeActivity(MediaLib *library, QSettings *settings, qt_intf_t *intf = nullptr);
    ~MLHomeActivity() override;
    QVariantList folders() const { return m_folders; }
    QVariantList playlists() const { return m_playlists; }
    QVariantList artists() const { return m_artists; }
    bool loading() const { return m_loading; }
    QString error() const { return m_error; }
    int mediaCount() const { return m_mediaCount; }

    // Call after a native folder/playlist content route has actually entered.
    Q_INVOKABLE void recordVisit(MLItemId id);
    // Register before the accepted Add Folder job can emit native root events.
    quint64 beginFolderAddition(const QUrl &);
    void completeFolderAddition(quint64 serial, bool accepted, bool existingRoot);
    // Cancel pending additions/visits without erasing accepted saved visits.
    void invalidatePendingVisits();
    Q_INVOKABLE void clearVisits();
    Q_INVOKABLE void refresh();
signals:
    void changed();
    void loadingChanged();
private:
    static void libraryEvent(void *, const vlc_ml_event_t *);
    void beginRefresh();
    void setLoading(bool);
    void addedFolderEvent(const QUrl &, bool success, quint64 epoch);
    void retryAddedFolders();
    MediaLib *m_library;
    qt_intf_t *m_interface;
    MLHomeVisitStore m_store;
    vlc_ml_event_callback_t *m_listener = nullptr;
    QSet<quint64> m_tasks;
    QTimer m_refreshTimer, m_folderLookupTimer;
    MLHomeFolderAdditions m_addedFolders;
    bool m_folderLookupRunning = false, m_folderLookupAgain = false;
    QVariantList m_folders, m_playlists, m_artists;
    QString m_error;
    QString m_persistenceError;
    int m_mediaCount = 0;
    bool m_loading = false;
    bool m_refreshRunning = false;
    bool m_refreshAgain = false;
    quint64 m_generation = 0;
    std::atomic_uint64_t m_visitEpoch{0};
};
