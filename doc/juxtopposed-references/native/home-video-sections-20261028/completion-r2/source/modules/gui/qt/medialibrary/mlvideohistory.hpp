// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#pragma once
#include "mlhelper.hpp"
#include <limits>

inline bool mlVideoHistoryMatches(const vlc_ml_media_t &media, bool completed)
{
    // A saved replay takes priority over an earlier completed watch. Do not
    // mistake a merely opened video (no position or play count) for completion.
    const bool unfinished = media.f_progress > 0. && media.f_progress < 1.;
    return completed ? media.i_playcount > 0 && media.f_progress <= 0. : unfinished;
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
