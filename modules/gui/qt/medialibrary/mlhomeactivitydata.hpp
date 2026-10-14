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
