/* SPDX-License-Identifier: LGPL-2.1-or-later */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include <vlc_process_priority.h>
#ifdef _WIN32
# include <windows.h>

static DWORD native_class(int level, bool realtime)
{
    if (realtime) return REALTIME_PRIORITY_CLASS;
    switch (level) {
        case VLC_PROCESS_PRIORITY_IDLE: return IDLE_PRIORITY_CLASS;
        case VLC_PROCESS_PRIORITY_BELOW_NORMAL: return BELOW_NORMAL_PRIORITY_CLASS;
        case VLC_PROCESS_PRIORITY_NORMAL: return NORMAL_PRIORITY_CLASS;
        case VLC_PROCESS_PRIORITY_ABOVE_NORMAL: return ABOVE_NORMAL_PRIORITY_CLASS;
        case VLC_PROCESS_PRIORITY_HIGH: return HIGH_PRIORITY_CLASS;
        default: return 0;
    }
}

static int describe(DWORD native, vlc_process_priority_state *state)
{
    *state = (vlc_process_priority_state){.supported = true,
        .level = VLC_PROCESS_PRIORITY_INHERIT};
    switch (native) {
        case IDLE_PRIORITY_CLASS: state->level = VLC_PROCESS_PRIORITY_IDLE; break;
        case BELOW_NORMAL_PRIORITY_CLASS: state->level = VLC_PROCESS_PRIORITY_BELOW_NORMAL; break;
        case NORMAL_PRIORITY_CLASS: state->level = VLC_PROCESS_PRIORITY_NORMAL; break;
        case ABOVE_NORMAL_PRIORITY_CLASS: state->level = VLC_PROCESS_PRIORITY_ABOVE_NORMAL; break;
        case HIGH_PRIORITY_CLASS: state->level = VLC_PROCESS_PRIORITY_HIGH; break;
        case REALTIME_PRIORITY_CLASS:
            state->level = VLC_PROCESS_PRIORITY_NORMAL; state->realtime = true; break;
        default:
            state->system_error = native == 0 ? GetLastError() : ERROR_NOT_SUPPORTED;
            return VLC_EGENERIC;
    }
    return VLC_SUCCESS;
}
#endif

int vlc_process_priority_get(vlc_process_priority_state *state)
{
#ifdef _WIN32
    return describe(GetPriorityClass(GetCurrentProcess()), state);
#else
    *state = (vlc_process_priority_state){.level = VLC_PROCESS_PRIORITY_INHERIT};
    return VLC_EGENERIC;
#endif
}

int vlc_process_priority_apply(int level, bool realtime, vlc_process_priority_state *state)
{
    if (vlc_process_priority_get(state) != VLC_SUCCESS) return VLC_EGENERIC;
#ifdef _WIN32
    if (level < VLC_PROCESS_PRIORITY_IDLE || level > VLC_PROCESS_PRIORITY_INHERIT) {
        state->system_error = ERROR_INVALID_PARAMETER;
        return VLC_EGENERIC;
    }
    if (level == VLC_PROCESS_PRIORITY_INHERIT && !realtime) return VLC_SUCCESS;
    const DWORD before = native_class(state->level, state->realtime);
    const DWORD requested = native_class(level, realtime);
    if (before == requested) return VLC_SUCCESS;
    const bool set = SetPriorityClass(GetCurrentProcess(), requested) != 0;
    DWORD error = set ? ERROR_SUCCESS : GetLastError();
    DWORD after = GetPriorityClass(GetCurrentProcess());
    if (set && after == requested) return describe(after, state);
    if (error == ERROR_SUCCESS)
        error = after == 0 ? GetLastError() : ERROR_NOT_SUPPORTED;
    if (error == ERROR_SUCCESS) error = ERROR_GEN_FAILURE;
    bool rollback_failed = false;
    if (after != before) {
        rollback_failed = !SetPriorityClass(GetCurrentProcess(), before);
        after = GetPriorityClass(GetCurrentProcess());
        rollback_failed |= after != before;
    }
    const int read = describe(after, state);
    (void)read;
    state->system_error = error;
    state->rollback_failed = rollback_failed;
    return VLC_EGENERIC;
#else
    (void)level; (void)realtime;
    return VLC_EGENERIC;
#endif
}
