// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include "player/player.h"
uint64_t vlc_test_privacy_epoch(vlc_player_t *);
bool vlc_test_privacy_remember(vlc_player_t *);
char *vlc_test_privacy_pattern(vlc_player_t *);
uint64_t vlc_test_privacy_epoch(vlc_player_t *p) { return p->history_epoch; }
bool vlc_test_privacy_remember(vlc_player_t *p) { return p->history_policy.remember_position; }
char *vlc_test_privacy_pattern(vlc_player_t *p) { return strdup(p->history_policy.exclusion_pattern); }
