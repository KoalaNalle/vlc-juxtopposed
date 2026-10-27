/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License, version 2 or later.
 *****************************************************************************/
#pragma once
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "mlqmltypes.hpp"
#include <QHash>
#include <QPointer>
#include <QSettings>
#include <QUrl>
#include <QDeadlineTimer>
#include <QVariantList>

// Visits are interface activity, distinct from native media completion counts.
class MLHomeVisitStore
{
public:
    struct Visit { MLItemId id; quint64 count = 0; qint64 lastVisit = 0; };
    explicit MLHomeVisitStore(QSettings *settings);
    bool record(MLItemId id, qint64 timestamp);
    bool clear();
    QList<Visit> visits() const;
private:
    QPointer<QSettings> m_settings;
    QHash<MLItemId, Visit> m_visits;
};

struct MLHomeActivitySnapshot
{
    QVariantList folders, playlists, artists;
    QString error;
    int mediaCount = 0;
};

MLHomeActivitySnapshot loadMLHomeActivity(vlc_medialibrary_t *, const QList<MLHomeVisitStore::Visit> &);

// Native roots are resolved through the existing entry-point list: the C API
// has no folder-by-MRL getter. No request stores an invented database ID.
QString mlHomeFolderKey(const QUrl &);
struct MLHomeFolderLookup
{
    enum Status { Missing, Ready, Unavailable, Error } status = Missing;
    MLItemId id;
};
MLHomeFolderLookup lookupMLHomeAddedFolder(vlc_medialibrary_t *, const QUrl &);

class MLHomeFolderAdditions
{
public:
    struct Request {
        QUrl folder;
        QString key;
        quint64 serial = 0, epoch = 0;
        bool accepted = false, confirmed = false;
        QDeadlineTimer deadline;
    };
    quint64 begin(const QUrl &);
    void accepted(quint64 serial, bool success, bool existingRoot);
    void rootAdded(const QUrl &, bool success);
    void cancel(const QUrl &);
    bool current(const Request &) const;
    bool consume(const Request &);
    QList<Request> ready() const;
    bool empty() const { return m_requests.isEmpty(); }
    void clear();
private:
    QHash<QString, Request> m_requests;
    quint64 m_serial = 0, m_epoch = 0;
};
