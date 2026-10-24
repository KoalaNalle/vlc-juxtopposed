/* SPDX-License-Identifier: LGPL-2.1-or-later */
#ifndef VLC_OSD_FEEDBACK_DURATION_H
#define VLC_OSD_FEEDBACK_DURATION_H
#include <vlc_common.h>
#include <vlc_variables.h>
/* Read for each message. Legacy defaults remain distinct until explicitly set.
 * Do not apply this to vout_OSDText callers that already supply a duration. */
static inline vlc_tick_t vlc_osd_feedback_duration(vlc_object_t *source,
                                                 vlc_tick_t legacy_duration)
{
    const int64_t milliseconds = var_InheritInteger(source, "osd-feedback-duration");
    return milliseconds < 0 ? legacy_duration
         : VLC_TICK_FROM_MS(VLC_CLIP(milliseconds, 500, 5000));
}
#endif
