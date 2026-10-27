// SPDX-License-Identifier: GPL-2.0-or-later
// Access the actual internal player for a Qt/backend diagnostic. Keep the C
// private media structure in C: its atomic fields are not C++ declarations.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "../../../../lib/media_player_internal.h"

vlc_player_t *vlc_test_media_player_core(libvlc_media_player_t *player);
vlc_object_t *vlc_test_media_player_object(libvlc_media_player_t *player);

vlc_player_t *vlc_test_media_player_core(libvlc_media_player_t *player)
{
    return player->player;
}

vlc_object_t *vlc_test_media_player_object(libvlc_media_player_t *player)
{
    return VLC_OBJECT(player);
}
