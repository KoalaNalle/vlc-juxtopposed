/* SPDX-License-Identifier: LGPL-2.1-or-later */
#ifndef VLC_PROCESS_PRIORITY_H
#define VLC_PROCESS_PRIORITY_H
#include <vlc_common.h>

enum {
    VLC_PROCESS_PRIORITY_IDLE = -2,
    VLC_PROCESS_PRIORITY_BELOW_NORMAL = -1,
    VLC_PROCESS_PRIORITY_NORMAL = 0,
    VLC_PROCESS_PRIORITY_ABOVE_NORMAL = 1,
    VLC_PROCESS_PRIORITY_HIGH = 2,
    VLC_PROCESS_PRIORITY_INHERIT = 3,
};
typedef struct vlc_process_priority_state {
    bool supported;
    int level; /* Base class, or Normal when the actual class is real-time. */
    bool realtime;
    uint32_t system_error;
    bool rollback_failed;
} vlc_process_priority_state;

/* These APIs address only the calling process. No token privilege is enabled.
 * Inherit/false leaves its class unchanged. Other requests require exact native
 * readback; a rejected/downgraded request restores the prior class if possible. */
VLC_API int vlc_process_priority_get(vlc_process_priority_state *) VLC_USED;
VLC_API int vlc_process_priority_apply(int level, bool realtime,
                                     vlc_process_priority_state *) VLC_USED;
#endif
