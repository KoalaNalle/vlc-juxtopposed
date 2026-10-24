// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef VLC_QT_PLAYBACK_SEEK_POLICY_HPP
#define VLC_QT_PLAYBACK_SEEK_POLICY_HPP
#include <vlc_common.h>
#include <vlc_player.h>
// Existing core flag is otherwise only created by input configuration; the
// actual Qt PlayerController caller chooses the speed for every explicit seek.
inline vlc_player_seek_speed qtPlaybackSeekSpeed(vlc_object_t *context)
{
    return var_InheritBool(context, "input-fast-seek")
        ? VLC_PLAYER_SEEK_FAST : VLC_PLAYER_SEEK_PRECISE;
}
#endif
