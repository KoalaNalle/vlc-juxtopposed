/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License, version 2 or later.
 *****************************************************************************/
#include "mlhomeactivitydata.hpp"
#include "mlhelper.hpp"
#include "qt.hpp"
#include <algorithm>
#include <limits>

namespace {
const QString visitKey = QStringLiteral("Juxtopposed/home-visits-v1");
bool visitType(MLItemId id)
{
    return id.id > 0 && (id.type == VLC_ML_PARENT_FOLDER || id.type == VLC_ML_PARENT_PLAYLIST);
}
void rank(QVariantList &rows)
{
    std::sort(rows.begin(), rows.end(), [](const QVariant &left, const QVariant &right) {
        const auto a = left.toMap(), b = right.toMap();
        if (a.value("activityCount").toULongLong() != b.value("activityCount").toULongLong())
            return a.value("activityCount").toULongLong() > b.value("activityCount").toULongLong();
        if (a.value("lastActivity").toLongLong() != b.value("lastActivity").toLongLong())
            return a.value("lastActivity").toLongLong() > b.value("lastActivity").toLongLong();
        const int titleOrder = QString::compare(a.value("title").toString(), b.value("title").toString(), Qt::CaseInsensitive);
        if (titleOrder)
            return titleOrder < 0;
        return a.value("id").value<MLItemId>().id < b.value("id").value<MLItemId>().id;
    });
}
}

MLHomeVisitStore::MLHomeVisitStore(QSettings *settings) : m_settings(settings)
{
    if (!m_settings)
        return;
    for (const auto &value : m_settings->value(visitKey).toList())
    {
        const auto row = value.toMap();
        const auto id = MLItemId::fromString(row.value("id").toString());
        bool countOk = false, dateOk = false;
        const auto count = row.value("count").toULongLong(&countOk);
        const auto date = row.value("lastVisit").toLongLong(&dateOk);
        if (visitType(id) && countOk && count > 0 && dateOk && date >= 0)
            m_visits.insert(id, {id, count, date});
    }
}

bool MLHomeVisitStore::record(MLItemId id, qint64 timestamp)
{
    if (!m_settings || !visitType(id) || timestamp < 0)
        return false;
    auto &visit = m_visits[id];
    visit.id = id;
    if (visit.count != std::numeric_limits<quint64>::max())
        ++visit.count;
    visit.lastVisit = std::max(visit.lastVisit, timestamp);
    QVariantList rows;
    const auto ordered = visits();
    for (const auto &record : ordered)
        rows.push_back(QVariantMap{{"id", record.id.toString()}, {"count", record.count}, {"lastVisit", record.lastVisit}});
    m_settings->setValue(visitKey, rows);
    m_settings->sync();
    return m_settings->status() == QSettings::NoError;
}

QList<MLHomeVisitStore::Visit> MLHomeVisitStore::visits() const
{
    auto result = m_visits.values();
    std::sort(result.begin(), result.end(), [](const Visit &a, const Visit &b) {
        if (a.id.type != b.id.type)
            return a.id.type < b.id.type;
        return a.id.id < b.id.id;
    });
    return result;
}

bool MLHomeVisitStore::clear()
{
    if (!m_settings)
        return false;
    m_settings->remove(visitKey);
    m_settings->sync();
    if (m_settings->status() != QSettings::NoError)
        return false;
    m_visits.clear();
    return true;
}


MLHomeActivitySnapshot loadMLHomeActivity(vlc_medialibrary_t *ml, const QList<MLHomeVisitStore::Visit> &visits)
{
    MLHomeActivitySnapshot ctx;
    if (!ml)
    {
        ctx.error = QStringLiteral("The media library is unavailable.");
        return ctx;
    }
    auto params = vlc_ml_query_params_create();
    const auto count = vlc_ml_count_media(ml, &params);
    ctx.mediaCount = static_cast<int>(std::min(count, static_cast<size_t>(std::numeric_limits<int>::max())));
    for (const auto &visit : visits)
    {
        if (!visitType(visit.id) || visit.count == 0)
            continue;
        QVariantMap row{{"id", QVariant::fromValue(visit.id)}, {"activityCount", visit.count}, {"lastActivity", visit.lastVisit}};
        if (visit.id.type == VLC_ML_PARENT_FOLDER)
        {
            ml_unique_ptr<vlc_ml_folder_t> folder(vlc_ml_get_folder(ml, visit.id.id));
            if (!folder || folder->b_banned)
                continue;
            row.insert("title", qfu(folder->psz_name));
            row.insert("available", folder->b_present && folder->psz_mrl);
            row.insert("mrl", folder->psz_mrl ? QUrl::fromEncoded(folder->psz_mrl) : QUrl());
            row.insert("count", folder->i_nb_media);
            ctx.folders.push_back(row);
        }
        else
        {
            ml_unique_ptr<vlc_ml_playlist_t> playlist(vlc_ml_get_playlist(ml, visit.id.id));
            if (!playlist)
                continue;
            row.insert("title", qfu(playlist->psz_name));
            row.insert("count", playlist->i_nb_media);
            ctx.playlists.push_back(row);
        }
    }
    struct ArtistActivity { quint64 count = 0; qint64 lastPlayed = 0; };
    QHash<int64_t, ArtistActivity> artists;
    // Page through the complete native audio set. Filtering/limiting
    // before aggregation would produce incorrect artist rankings.
    constexpr uint32_t pageSize = 512;
    params.i_nbResults = pageSize;
    for (uint32_t offset = 0;;)
    {
        params.i_offset = offset;
        ml_unique_ptr<vlc_ml_media_list_t> audio(vlc_ml_list_audio_media(ml, &params));
        if (!audio)
        {
            ctx.error = QStringLiteral("Could not load listening history.");
            return ctx;
        }
        for (const auto &media : ml_range_iterate<vlc_ml_media_t>(audio))
        {
            if (media.i_subtype != VLC_ML_MEDIA_SUBTYPE_ALBUMTRACK || media.album_track.i_artist_id <= 0 ||
                (media.i_playcount == 0 && media.i_last_played_date <= 0))
                continue;
            auto &artist = artists[media.album_track.i_artist_id];
            artist.count += media.i_playcount;
            artist.lastPlayed = std::max(artist.lastPlayed, static_cast<qint64>(media.i_last_played_date));
        }
        if (audio->i_nb_items < pageSize)
            break;
        if (offset > std::numeric_limits<uint32_t>::max() - pageSize)
        {
            ctx.error = QStringLiteral("Listening history is too large to load.");
            return ctx;
        }
        offset += pageSize;
    }
    for (auto it = artists.cbegin(); it != artists.cend(); ++it)
    {
        ml_unique_ptr<vlc_ml_artist_t> artist(vlc_ml_get_artist(ml, it.key()));
        if (!artist)
            continue;
        ctx.artists.push_back(QVariantMap{
            {"id", QVariant::fromValue(MLItemId(artist->i_id, VLC_ML_PARENT_ARTIST))},
            {"title", qfu(artist->psz_name)},
            {"artwork", qfu(artist->thumbnails[VLC_ML_THUMBNAIL_SMALL].psz_mrl)},
            {"activityCount", it->count}, {"lastActivity", it->lastPlayed},
            {"count", artist->i_nb_tracks}});
    }
    rank(ctx.folders);
    rank(ctx.playlists);
    rank(ctx.artists);
    return ctx;
}
