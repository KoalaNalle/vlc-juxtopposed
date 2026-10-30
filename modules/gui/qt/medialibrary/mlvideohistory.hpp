// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#pragma once
#include "mlhelper.hpp"
#include <vlc_player.h>
#include <limits>
#include <mutex>
#include <optional>
#include <unordered_set>
#include <vector>

inline bool mlVideoHistoryMatches(const vlc_ml_media_t &media, bool completed)
{
    // A saved replay takes priority over an earlier completed watch. Do not
    // mistake a merely opened video (no position or play count) for completion.
    const bool unfinished = media.f_progress > 0. && media.f_progress < 1.;
    return completed ? media.i_playcount > 0 && media.f_progress <= 0. : unfinished;
}

enum class MLVideoHistoryScope { All, Unfinished, Completed };

inline bool mlVideoHistoryMatches(const vlc_ml_media_t &media, MLVideoHistoryScope scope)
{
    return media.i_type == VLC_ML_MEDIA_TYPE_VIDEO && media.i_last_played_date > 0
        && (scope == MLVideoHistoryScope::All
            || mlVideoHistoryMatches(media, scope == MLVideoHistoryScope::Completed));
}

struct MLVideoHistoryEditResult
{
    bool enumerated = false;
    size_t removed = 0;
    size_t failed = 0;
    bool success() const { return enumerated && failed == 0; }
};

inline std::mutex &mlVideoHistoryEditMutex()
{
    static std::mutex mutex;
    return mutex;
}

// Serialize only the final scope check and removal with this player's Stop.
// VLC_ENOENT means the item changed section meanwhile: leave its newer history
// alone. Other failures must be reported, including a missing indexed item.
inline int removeMLVideoHistoryItem(vlc_medialibrary_t *ml, vlc_player_t *player,
                                   MLVideoHistoryScope scope, int64_t id)
{
    if (!ml || !player)
        return VLC_EGENERIC;
    vlc_player_Lock(player);
    ml_unique_ptr<vlc_ml_media_t> media(vlc_ml_get_media(ml, id));
    const int result = !media ? VLC_EGENERIC
        : !mlVideoHistoryMatches(*media, scope) ? VLC_ENOENT
        : vlc_player_RemoveMediaFromHistory(player, id);
    vlc_player_Unlock(player);
    return result;
}

// Snapshot IDs first: deleting while advancing native offsets would skip later
// entries. Do not hold the player lock during the snapshot or the entire batch.
// No view query is accepted, so search, dashboard limits and paging cannot turn
// "clear all" into an accidental partial clear. A stale item is a safe no-op.
template<typename Remove>
MLVideoHistoryEditResult editMLVideoHistory(vlc_medialibrary_t *ml,
    MLVideoHistoryScope scope, std::optional<int64_t> itemId, Remove remove)
{
    MLVideoHistoryEditResult result;
    if (!ml || (itemId && *itemId <= 0))
        return result;
    // Separate dashboard/full-list models can issue edits together. Keep their
    // snapshots and mutations ordered without holding the player or UI lock.
    const std::lock_guard<std::mutex> editLock(mlVideoHistoryEditMutex());
    std::vector<int64_t> ids;
    if (itemId)
        ids.push_back(*itemId);
    else
    {
        auto query = vlc_ml_query_params_create();
        constexpr uint32_t pageSize = 256;
        // The native default sorts by last playback date. Stop updates that
        // date and could move an existing row across the next page boundary.
        query.i_sort = VLC_ML_SORTING_INSERTIONDATE;
        query.b_desc = false;
        query.i_nbResults = pageSize;
        std::unordered_set<int64_t> seen;
        for (;;)
        {
            ml_unique_ptr<vlc_ml_media_list_t> page(vlc_ml_list_video_history(ml, &query));
            if (!page)
                return result; // Nothing has been changed yet.
            for (const auto &media : ml_range_iterate<vlc_ml_media_t>(page))
                if (mlVideoHistoryMatches(media, scope) && seen.insert(media.i_id).second)
                    ids.push_back(media.i_id);
            if (page->i_nb_items < pageSize)
                break;
            if (query.i_offset > std::numeric_limits<uint32_t>::max() - pageSize)
                return result;
            query.i_offset += pageSize;
        }
    }
    result.enumerated = true;
    for (const auto id : ids)
    {
        ml_unique_ptr<vlc_ml_media_t> media(vlc_ml_get_media(ml, id));
        if (!media)
        {
            ++result.failed;
            continue;
        }
        // The card or the snapshot may have become stale since it was read.
        if (!mlVideoHistoryMatches(*media, scope))
            continue;
        const auto status = remove(id);
        if (status == VLC_SUCCESS)
            ++result.removed;
        else if (status != VLC_ENOENT)
            ++result.failed;
    }
    return result;
}

// Filter before applying the view's offset/limit. Filtering an already limited
// history page would leave gaps and produce incorrect See All counts.
template<typename Visitor>
size_t visitMLVideoHistory(vlc_medialibrary_t *ml, const vlc_ml_query_params_t *params,
                          bool completed, Visitor visitor)
{
    auto query = params ? *params : vlc_ml_query_params_create();
    constexpr uint32_t pageSize = 256;
    query.i_nbResults = pageSize;
    query.i_offset = 0;
    size_t matched = 0;
    for (;;)
    {
        ml_unique_ptr<vlc_ml_media_list_t> page(vlc_ml_list_video_history(ml, &query));
        if (!page)
            return 0;
        for (const auto &media : ml_range_iterate<vlc_ml_media_t>(page))
            if (mlVideoHistoryMatches(media, completed) && !visitor(media, matched++))
                return matched;
        if (page->i_nb_items < pageSize || query.i_offset > std::numeric_limits<uint32_t>::max() - pageSize)
            return matched;
        query.i_offset += pageSize;
    }
}
