// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifndef VLC_QT_PLAYER_TIMER_POINT_HPP
#define VLC_QT_PLAYER_TIMER_POINT_HPP

#include <vlc_common.h>
#include <vlc_player.h>

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

} // namespace vlc::qt

#endif
