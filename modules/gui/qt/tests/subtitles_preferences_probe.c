/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include <vlc/vlc.h>
#include <vlc_variables.h>
#include "lib/libvlc_internal.h"
/* Configured Qt AM flags use the plugin log macros in component sources. */
const char vlc_module_name[]="subtitles_preferences_probe";
vlc_object_t *vlc_test_subtitles_object(libvlc_instance_t *);
void vlc_test_subtitles_vmem(vlc_object_t *,libvlc_video_lock_cb,libvlc_video_unlock_cb,libvlc_video_display_cb,void *);
vlc_object_t *vlc_test_subtitles_object(libvlc_instance_t *instance)
{return VLC_OBJECT(instance->p_libvlc_int);}
void vlc_test_subtitles_vmem(vlc_object_t *root,libvlc_video_lock_cb lock,
                            libvlc_video_unlock_cb unlock,libvlc_video_display_cb display,void *opaque)
{
    var_Create(root,"vmem-lock",VLC_VAR_ADDRESS);var_SetAddress(root,"vmem-lock",lock);
    var_Create(root,"vmem-unlock",VLC_VAR_ADDRESS);var_SetAddress(root,"vmem-unlock",unlock);
    var_Create(root,"vmem-display",VLC_VAR_ADDRESS);var_SetAddress(root,"vmem-display",display);
    var_Create(root,"vmem-data",VLC_VAR_ADDRESS);var_SetAddress(root,"vmem-data",opaque);
    var_Create(root,"vmem-chroma",VLC_VAR_STRING);var_SetString(root,"vmem-chroma","BGRA");
    var_Create(root,"vmem-width",VLC_VAR_INTEGER);var_SetInteger(root,"vmem-width",320);
    var_Create(root,"vmem-height",VLC_VAR_INTEGER);var_SetInteger(root,"vmem-height",180);
    var_Create(root,"vmem-pitch",VLC_VAR_INTEGER);var_SetInteger(root,"vmem-pitch",1280);
}
