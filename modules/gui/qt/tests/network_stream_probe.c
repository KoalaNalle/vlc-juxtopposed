// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include <vlc/vlc.h>
#include <vlc_media_library.h>
#include <vlc_playlist.h>
#include "../../../../lib/libvlc_internal.h"
#include "../../../../src/player/player.h"

vlc_playlist_t *vlc_test_stream_playlist(libvlc_instance_t *instance);
vlc_medialibrary_t *vlc_test_stream_library(libvlc_instance_t *instance);
void vlc_test_stream_audio(vlc_playlist_t *playlist,
    void (*play)(void *, const void *, unsigned, int64_t), void *opaque);

vlc_playlist_t *vlc_test_stream_playlist(libvlc_instance_t *instance)
{
    return vlc_playlist_New(VLC_OBJECT(instance->p_libvlc_int),
        VLC_PLAYLIST_PREPARSING_DISABLED, 1, VLC_TICK_FROM_SEC(5));
}

vlc_medialibrary_t *vlc_test_stream_library(libvlc_instance_t *instance)
{
    return vlc_ml_instance_get(instance->p_libvlc_int);
}

void vlc_test_stream_audio(vlc_playlist_t *playlist,
    void (*play)(void *, const void *, unsigned, int64_t), void *opaque)
{
    vlc_player_t *player = vlc_playlist_GetPlayer(playlist);
    var_Create(player, "amem-play", VLC_VAR_ADDRESS);
    var_Create(player, "amem-data", VLC_VAR_ADDRESS);
    var_Create(player, "amem-format", VLC_VAR_STRING);
    var_Create(player, "amem-rate", VLC_VAR_INTEGER);
    var_Create(player, "amem-channels", VLC_VAR_INTEGER);
    var_SetAddress(player, "amem-play", play);
    var_SetAddress(player, "amem-data", opaque);
    var_SetString(player, "amem-format", "FL32");
    var_SetInteger(player, "amem-rate", 44100);
    var_SetInteger(player, "amem-channels", 1);
    var_SetString(player, "aout", "amem,none");
    vlc_player_aout_Reset(player);
}
