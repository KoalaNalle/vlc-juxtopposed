// SPDX-License-Identifier: GPL-2.0-or-later
// Private native instance bridge; this does not install stub modules.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "../../../../lib/libvlc_internal.h"

vlc_object_t *vlc_test_native_instance_object(libvlc_instance_t *instance);
vlc_object_t *vlc_test_native_instance_object(libvlc_instance_t *instance)
{
    return VLC_OBJECT(instance->p_libvlc_int);
}
