/* SPDX-License-Identifier: LGPL-2.1-or-later */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include <vlc/vlc.h>
#include "../../../lib/libvlc_internal.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

/* A quiet owned child. It never opens media or alters another process. Restore
 * the initial class immediately after observing initialization, before I/O. */
int main(int argc, char **argv)
{
    const char *initial = getenv("JUXTO_PRIORITY_INITIAL");
    if (initial && !SetPriorityClass(GetCurrentProcess(), strtoul(initial, NULL, 0)))
        return 2;
    const DWORD before = GetPriorityClass(GetCurrentProcess());
    libvlc_instance_t *instance = libvlc_new(argc - 1, (const char **)(argv + 1));
    const DWORD after = GetPriorityClass(GetCurrentProcess());
    const bool restored = SetPriorityClass(GetCurrentProcess(), before)
        && GetPriorityClass(GetCurrentProcess()) == before;
    int64_t startup_error = -1;
    if (instance) {
        vlc_object_t *root = VLC_OBJECT(instance->p_libvlc_int);
        if (var_Type(root, "win32-priority-request-error"))
            startup_error = var_GetInteger(root, "win32-priority-request-error");
        libvlc_release(instance);
    }
    printf("{\"before\":%lu,\"after\":%lu,\"restored\":%s,"
           "\"initialized\":%s,\"startupError\":%"PRId64"}\n",
           (unsigned long)before, (unsigned long)after,
           restored ? "true" : "false", instance ? "true" : "false", startup_error);
    return instance && restored ? 0 : 1;
}
