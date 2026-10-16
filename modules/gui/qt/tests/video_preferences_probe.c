/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include <vlc/vlc.h>
#include "lib/libvlc_internal.h"
vlc_object_t *vlc_test_video_preferences_object(libvlc_instance_t *);
vlc_object_t *vlc_test_video_preferences_object(libvlc_instance_t *instance)
{ return VLC_OBJECT(instance->p_libvlc_int); }
