// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc/vlc.h>
#include <vlc_playlist.h>
#include "lib/libvlc_internal.h"
#include "playlist/playlist.h"

vlc_object_t *vlc_test_playback_object(libvlc_instance_t *);
bool vlc_test_playback_start_paused(vlc_player_t *);
int vlc_test_playback_stopped_action(vlc_playlist_t *);

vlc_object_t *vlc_test_playback_object(libvlc_instance_t *instance)
{ return VLC_OBJECT(instance->p_libvlc_int); }
bool vlc_test_playback_start_paused(vlc_player_t *player)
{
    vlc_player_Lock(player);
    const bool value = player->start_paused;
    vlc_player_Unlock(player);
    return value;
}
int vlc_test_playback_stopped_action(vlc_playlist_t *playlist)
{
    vlc_playlist_Lock(playlist);
    const int value = playlist->stopped_action;
    vlc_playlist_Unlock(playlist);
    return value;
}
