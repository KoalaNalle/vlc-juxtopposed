/* SPDX-License-Identifier: LGPL-2.1-or-later */
#ifndef VLC_CONFIG_TRANSACTION_H
#define VLC_CONFIG_TRANSACTION_H 1

#include <vlc_configuration.h>

#ifdef __cplusplus
extern "C" {
#endif

/** One exact, persistent configuration change. Strings are copied. */
typedef struct vlc_config_change {
    const char *name;
    int type; /**< VLC_VAR_BOOL, VLC_VAR_INTEGER, VLC_VAR_FLOAT or VLC_VAR_STRING */
    union {
        int64_t i;
        float f;
        const char *str;
    } value;
} vlc_config_change;

/**
 * Atomically apply and save a group of exact values.
 *
 * Existing unrelated saved options are preserved across cooperating processes.
 * The latest successful serialized edit wins when the same key is edited.
 * On failure, previous in-memory values and pending-save state are restored.
 * Out-of-range, unsaveable, duplicate or mismatched changes are rejected.
 * Values saved by other processes are not imported into this running instance.
 *
 * Returns 0 after successful atomic replacement, or -1 on failure. No readback
 * outside this transaction is needed and could race with a later valid writer.
 */
VLC_API int config_SaveChanges(libvlc_int_t *, const vlc_config_change *, size_t);
#define config_SaveChanges(a, c, n) config_SaveChanges(vlc_object_instance(a), c, n)

#ifdef __cplusplus
}
#endif

#endif
