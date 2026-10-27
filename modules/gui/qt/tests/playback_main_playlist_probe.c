// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc/vlc.h>
#include <vlc_interface.h>
#include <vlc_playlist.h>
#include "lib/libvlc_internal.h"

vlc_playlist_t *vlc_test_playback_main_playlist(libvlc_instance_t *);
void vlc_test_playback_exit_handler(libvlc_instance_t *, void (*)(void *), void *);

void vlc_test_playback_exit_handler(libvlc_instance_t *instance,
                                  void (*handler)(void *), void *opaque)
{
    // Observe only this isolated instance's exit notification. This does not
    // terminate the console runner or register an operating-system action.
    libvlc_SetExitHandler(instance->p_libvlc_int, handler, opaque);
}

vlc_playlist_t *vlc_test_playback_main_playlist(libvlc_instance_t *instance)
{
    // Use the application's real startup path. It applies the saved main-player
    // policies; a bare vlc_playlist_New deliberately does not do that work.
    // This object loads no interface module and creates no window or dialog.
    intf_thread_t *intf = vlc_object_create(instance->p_libvlc_int, sizeof(*intf));
    if (!intf)
        return NULL;
    vlc_playlist_t *playlist = vlc_intf_GetMainPlaylist(intf);
    vlc_object_delete(intf);
    return playlist; // Owned by the isolated LibVLC instance.
}
