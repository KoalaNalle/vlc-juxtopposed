// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifndef VLC_QT_PLAYER_TIMER_POINT_HPP
#define VLC_QT_PLAYER_TIMER_POINT_HPP

#include <vlc_common.h>
#include <vlc_player.h>
#include <cmath>
#include <limits>

namespace vlc::qt {

inline vlc_player_timer_point invalidPlayerTimerPoint()
{
    return {0.0, 1.0, VLC_TICK_INVALID, VLC_TICK_INVALID,
            false, VLC_TICK_INVALID};
}

inline int interpolatePlayerTimerPoint(const vlc_player_timer_point &point,
                                      vlc_tick_t systemNow,
                                      vlc_tick_t *time, double *position)
{
    // Initial paused audio can report a pause before any timer point exists.
    // The native interpolation API requires a point received by on_update.
    if (point.system_date == VLC_TICK_INVALID || point.ts == VLC_TICK_INVALID)
        return VLC_EGENERIC;
    return vlc_player_timer_point_Interpolate(&point, systemNow, time, position);
}

inline vlc_tick_t playerTimelineTime(const vlc_player_timer_point &point,
                                    vlc_tick_t time, double position,
                                    bool seekable)
{
    if (!point.live)
        return time;

    // Live timestamps can be absolute stream clocks. Only the normalized
    // position and length describe the current seek window (vlc_player.h).
    // Preserve the engine timestamp elsewhere for seeking and subtitle cues.
    if (!seekable || point.length <= 0 || !std::isfinite(position)
        || position < 0.0 || position > 1.0)
        return VLC_TICK_INVALID;

    const long double offset = position * static_cast<long double>(point.length);
    if (offset >= static_cast<long double>(std::numeric_limits<vlc_tick_t>::max() - VLC_TICK_0))
        return VLC_TICK_INVALID;
    return VLC_TICK_0 + static_cast<vlc_tick_t>(offset);
}

inline vlc_tick_t playerSeekTime(const vlc_player_timer_point &point,
                                 vlc_tick_t previousTime)
{
    // on_seek carries the requested zero-based time, unlike on_update's ts.
    // A live position request has no absolute stream timestamp. Keep it unknown
    // until a real point arrives; a relative offset is not a subtitle/cue clock.
    if (point.live && point.position >= 0.0)
        return VLC_TICK_INVALID;
    if (!point.live && point.position == 0.0 && point.length > 0)
        return VLC_TICK_0;
    if (point.ts > VLC_TICK_INVALID
        && point.ts < std::numeric_limits<vlc_tick_t>::max())
        return point.ts + VLC_TICK_0;
    return previousTime;
}

} // namespace vlc::qt

#endif
