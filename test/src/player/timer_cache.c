// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
// Compile the actual timer implementation to exercise its private video cache.
// This component test does not create a player, decoder or native ES track.
#include "../../../src/player/timer.c"
#include <stdio.h>

struct observed_timecode
{
    unsigned updates;
    unsigned frames;
};

static void on_timecode(const struct vlc_player_timer_smpte_timecode *tc, void *data)
{
    struct observed_timecode *observed = data;
    ++observed->updates;
    observed->frames = tc->frames;
}

static bool check_cache(double rate, bool paused, bool listener)
{
    vlc_player_t player = {0};
    vlc_player_InitTimer(&player);
    player.timer.input_length = VLC_TICK_FROM_SEC(60);
    struct vlc_player_timer_source *source = &player.timer.smpte_source;
    // An already selected source is compared by identity only in this path.
    // The local token is never dereferenced or passed to a native player API.
    int video_token;
    source->es = (vlc_es_id_t *)&video_token;
    struct observed_timecode observed = {0};
    const struct vlc_player_timer_smpte_cbs callbacks = {.on_update = on_timecode};
    struct vlc_player_timer_id timer = {.smpte_cbs = &callbacks, .data = &observed};
    if (listener) vlc_list_append(&timer.node, &source->listeners);
    struct vlc_player_timer_point point = {
        .rate = rate, .ts = VLC_TICK_0 + VLC_TICK_FROM_MS(80),
        .system_date = paused ? VLC_TICK_MAX : VLC_TICK_FROM_SEC(10),
    };
    vlc_mutex_lock(&player.timer.lock);
    vlc_player_UpdateTimerSmpteSource(&player, source->es, &point,
                                     point.system_date, 0, 0);
    bool first = source->point.ts == point.ts
              && source->point.system_date != VLC_TICK_INVALID
              && source->point.rate == rate;
    point.ts += VLC_TICK_FROM_MS(40);
    vlc_player_UpdateTimerSmpteSource(&player, source->es, &point,
                                     point.system_date, 0, 0);
    bool second = source->point.ts == point.ts
               && source->point.system_date == point.system_date;
    bool quiet = observed.updates == 0 && source->smpte.frame_rate == 0;
    // Frame-number notifications start only when the rate becomes known.
    vlc_player_UpdateTimerSmpteSource(&player, source->es, &point,
                                     point.system_date, 25, 1);
    bool known = observed.updates == (listener ? 1u : 0u)
              && (!listener || observed.frames == 3)
              && source->point.ts == point.ts;
    vlc_mutex_unlock(&player.timer.lock);
    if (listener) vlc_list_remove(&timer.node);
    vlc_player_DestroyTimer(&player);
    const bool passed = first && second && quiet && known;
    printf("{\"rate\":%.1f,\"paused\":%s,\"listener\":%s,"
           "\"firstCached\":%s,\"secondCached\":%s,\"quietBeforeFps\":%s,"
           "\"knownRateCorrect\":%s,\"passed\":%s}\n", rate,
           paused ? "true" : "false", listener ? "true" : "false",
           first ? "true" : "false", second ? "true" : "false",
           quiet ? "true" : "false", known ? "true" : "false",
           passed ? "true" : "false");
    return passed;
}

int main(void)
{
    unsigned failures = 0;
    const double rates[] = {.5, 1., 1.5, 2.};
    for (unsigned i = 0; i < ARRAY_SIZE(rates); ++i)
        for (unsigned paused = 0; paused < 2; ++paused)
            for (unsigned listener = 0; listener < 2; ++listener)
                failures += !check_cache(rates[i], paused, listener);
    return failures ? 1 : 0;
}
