/*****************************************************************************
 * Copyright (C) 2026 VLC authors and VideoLAN
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License, version 2 or later.
 *****************************************************************************/
#include "mlhomeactivitydata.hpp"
#include "dialogs/new_playlist_data.hpp"
#include "mlhelper.hpp"
#include "qt.hpp"
#include <algorithm>
#include <limits>
#include <utility>

namespace {
const QString visitKey = QStringLiteral("Juxtopposed/home-visits-v1");
QList<MLHomeVisitStore::Visit> orderedVisits(const QHash<MLItemId, MLHomeVisitStore::Visit> &records)
{
    auto result = records.values();
    std::sort(result.begin(), result.end(), [](const MLHomeVisitStore::Visit &a, const MLHomeVisitStore::Visit &b) {
        if (a.id.type != b.id.type) return a.id.type < b.id.type;
        return a.id.id < b.id.id;
    });
    return result;
}
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
    if (!m_settings || m_settings->status() != QSettings::NoError || !visitType(id) || timestamp < 0)
        return false;
    auto candidate = m_visits;
    auto &visit = candidate[id];
    visit.id = id;
    if (visit.count != std::numeric_limits<quint64>::max())
        ++visit.count;
    visit.lastVisit = std::max(visit.lastVisit, timestamp);
    QVariantList rows;
    const auto ordered = orderedVisits(candidate);
    for (const auto &record : ordered)
        rows.push_back(QVariantMap{{"id", record.id.toString()}, {"count", record.count}, {"lastVisit", record.lastVisit}});
    const bool hadKey = m_settings->contains(visitKey);
    const auto previous = m_settings->value(visitKey);
    m_settings->setValue(visitKey, rows);
    m_settings->sync();
    if (m_settings->status() != QSettings::NoError)
    {
        // Keep both the accepted cache and QSettings' pending document intact:
        // its later destructor/retry must not save a failed candidate visit.
        if (hadKey) m_settings->setValue(visitKey, previous);
        else m_settings->remove(visitKey);
        return false;
    }
    m_visits = std::move(candidate);
    return true;
}

QList<MLHomeVisitStore::Visit> MLHomeVisitStore::visits() const
{
    return orderedVisits(m_visits);
}

bool MLHomeVisitStore::clear()
{
    if (!m_settings || m_settings->status() != QSettings::NoError)
        return false;
    const bool hadKey = m_settings->contains(visitKey);
    const auto previous = m_settings->value(visitKey);
    m_settings->remove(visitKey);
    m_settings->sync();
    if (m_settings->status() != QSettings::NoError)
    {
        if (hadKey) m_settings->setValue(visitKey, previous);
        else m_settings->remove(visitKey);
        return false;
    }
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
            auto artwork = newPlaylistArtwork(newPlaylistArtworkDirectory(), playlist->i_id);
            if (artwork.isEmpty()) artwork = qfu(playlist->psz_artwork_mrl);
            row.insert("artwork", artwork);
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

QString mlHomeFolderKey(const QUrl &folder)
{
    if (!folder.isValid() || !folder.isLocalFile() || folder.hasQuery() || folder.hasFragment())
        return {};
    return folder.adjusted(QUrl::NormalizePathSegments | QUrl::StripTrailingSlash)
                 .toString(QUrl::FullyEncoded);
}

MLHomeFolderLookup lookupMLHomeAddedFolder(vlc_medialibrary_t *ml, const QUrl &requested)
{
    MLHomeFolderLookup result;
    const auto key = mlHomeFolderKey(requested);
    if (!ml) { result.status = MLHomeFolderLookup::Error; return result; }
    if (key.isEmpty()) { result.status = MLHomeFolderLookup::Unavailable; return result; }
    ml_unique_ptr<vlc_ml_folder_list_t> roots(vlc_ml_list_entry_points(ml, nullptr));
    if (!roots) { result.status = MLHomeFolderLookup::Error; return result; }
    for (const auto &folder : ml_range_iterate<vlc_ml_folder_t>(roots))
    {
        if (!folder.psz_mrl || mlHomeFolderKey(QUrl::fromEncoded(folder.psz_mrl)) != key)
            continue;
        if (folder.i_id <= 0 || !folder.b_present || folder.b_banned)
            result.status = MLHomeFolderLookup::Unavailable;
        else
        {
            result.status = MLHomeFolderLookup::Ready;
            result.id = MLItemId(folder.i_id, VLC_ML_PARENT_FOLDER);
        }
        return result;
    }
    ml_unique_ptr<vlc_ml_folder_list_t> banned(vlc_ml_list_banned_entry_points(ml, nullptr));
    if (!banned) { result.status = MLHomeFolderLookup::Error; return result; }
    for (const auto &folder : ml_range_iterate<vlc_ml_folder_t>(banned))
        if (folder.psz_mrl && mlHomeFolderKey(QUrl::fromEncoded(folder.psz_mrl)) == key)
        { result.status = MLHomeFolderLookup::Unavailable; break; }
    return result;
}

quint64 MLHomeFolderAdditions::begin(const QUrl &folder)
{
    const auto key = mlHomeFolderKey(folder);
    if (key.isEmpty()) return 0;
    if (m_requests.contains(key)) return m_requests[key].serial;
    Request request; request.folder = folder; request.key = key;
    request.serial = ++m_serial; request.epoch = m_epoch;
    m_requests.insert(key, request);
    return request.serial;
}
void MLHomeFolderAdditions::accepted(quint64 serial, bool success, bool existingRoot)
{
    for (auto it = m_requests.begin(); it != m_requests.end(); ++it)
        if (it->serial == serial)
        {
            if (!success) { m_requests.erase(it); return; }
            it->accepted = true;
            if (existingRoot && !it->confirmed)
            {
                it->confirmed = true;
                it->deadline = QDeadlineTimer(30000);
            }
            return;
        }
}
void MLHomeFolderAdditions::rootAdded(const QUrl &folder, bool success)
{
    const auto key = mlHomeFolderKey(folder);
    auto it = m_requests.find(key);
    if (it == m_requests.end()) return;
    if (!success) { m_requests.erase(it); return; }
    if (!it->confirmed)
    {
        it->confirmed = true;
        it->deadline = QDeadlineTimer(30000);
    }
}
void MLHomeFolderAdditions::cancel(const QUrl &folder)
{
    m_requests.remove(mlHomeFolderKey(folder));
}
bool MLHomeFolderAdditions::current(const Request &request) const
{
    const auto it = m_requests.find(request.key);
    return it != m_requests.end() && it->serial == request.serial && it->epoch == request.epoch
        && it->accepted && it->confirmed;
}
bool MLHomeFolderAdditions::consume(const Request &request)
{
    if (!current(request)) return false;
    m_requests.remove(request.key); return true;
}
QList<MLHomeFolderAdditions::Request> MLHomeFolderAdditions::ready() const
{
    QList<Request> result;
    for (const auto &request : m_requests)
        if (request.accepted && request.confirmed) result.append(request);
    return result;
}
void MLHomeFolderAdditions::clear()
{
    ++m_epoch; m_requests.clear();
}
