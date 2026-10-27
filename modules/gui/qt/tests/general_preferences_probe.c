// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc/vlc.h>
#include "lib/libvlc_internal.h"

vlc_object_t *vlc_test_general_instance_object(libvlc_instance_t *instance);
vlc_object_t *vlc_test_general_instance_object(libvlc_instance_t *instance)
{ return VLC_OBJECT(instance->p_libvlc_int); }
