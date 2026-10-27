// SPDX-License-Identifier: GPL-2.0-or-later
// Isolated timer state only; this never mutates an active playback player.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include "../../../../src/player/player.h"

static struct vlc_player_timer_point *captured_point;
static unsigned callback_count;
static void vlc_player_SendTimerSeek(vlc_player_t *player,
    struct vlc_player_timer_source *source,
    const struct vlc_player_timer_point *point, bool smpte)
{
    (void)player; (void)source;
    ++callback_count;
    if (!smpte) *captured_point = *point;
    else assert(point->ts == captured_point->ts
        && point->position == captured_point->position
        && point->length == captured_point->length && point->live == captured_point->live);
}
static void owned_UpdateTimerSeekState(vlc_player_t *, vlc_tick_t, double);
#define vlc_player_UpdateTimerSeekState owned_UpdateTimerSeekState
#include "player_seek_timer_production.inc"
#undef vlc_player_UpdateTimerSeekState

unsigned vlc_test_pending_seek_point(vlc_tick_t input_length, bool input_live,
    vlc_tick_t output_length, bool output_live, vlc_tick_t time, double position,
    double previous_position, struct vlc_player_timer_point *point);
unsigned vlc_test_pending_seek_point(vlc_tick_t input_length, bool input_live,
    vlc_tick_t output_length, bool output_live, vlc_tick_t time, double position,
    double previous_position, struct vlc_player_timer_point *point)
{
    vlc_player_t player = {0};
    vlc_mutex_init(&player.timer.lock);
    player.timer.input_length = input_length;
    player.timer.input_live = input_live;
    player.timer.best_source.point.length = output_length;
    player.timer.best_source.point.live = output_live;
    player.timer.best_source.point.rate = 1.;
    player.timer.seek_position = previous_position;
    captured_point = point; callback_count = 0;
    owned_UpdateTimerSeekState(&player, time, position);
    return callback_count;
}
