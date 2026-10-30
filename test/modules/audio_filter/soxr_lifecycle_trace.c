// SPDX-License-Identifier: GPL-2.0-or-later
// Separate diagnostic trace: frozen acceptance assertions remain unchanged.
#define main soxr_frozen_acceptance_main
#include "soxr_lifecycle.c"
#undef main

static void trace_state(filter_t *filter, struct statistics *stats, const char *label)
{
    filter_sys_t *sys = filter->p_sys;
    printf("TRACE %s samples=%llu fixedDelay=%.12g variableDelay=%.12g last=%s lastCapacity=%zu nextPts=%lld\n",
           label, (unsigned long long)stats->samples, soxr_delay(sys->soxr),
           soxr_delay(sys->vr_soxr), sys->last_soxr == sys->soxr ? "fixed" :
           sys->last_soxr == sys->vr_soxr ? "variable" : "none", sys->i_last_olen,
           (long long)sys->i_next_pts);
    fflush(stdout);
}
static uint64_t trace_direct_feed(soxr_t engine, unsigned samples, unsigned rate)
{
    uint64_t total = 0;
    unsigned offset = 0;
    while (offset < samples)
    {
        const unsigned remaining = samples - offset;
        const unsigned frames = remaining <= 17 ? remaining : __MIN(511u, remaining - 17);
        float input[511], output[2048];
        for (unsigned i = 0; i < frames; ++i)
            input[i] = .35f * sin(2 * 3.14159265358979323846 * 220 * (offset + i) / rate);
        size_t consumed, produced;
        assert(soxr_process(engine, input, frames, &consumed, output,
                            ARRAY_SIZE(output), &produced) == NULL);
        assert(consumed == frames);
        total += produced;
        offset += frames;
    }
    return total;
}
static void trace_direct_variable(libvlc_instance_t *instance)
{
    filter_t *filter = new_filter(instance, 44100, 48000, true);
    filter_sys_t *sys = filter->p_sys;
    soxr_t engine = sys->vr_soxr;
    assert(soxr_set_io_ratio(engine, 1.0, 0) == NULL);
    uint64_t total = trace_direct_feed(engine, 48000, 48000);
    printf("TRACE directVR after48000=%llu delay=%.12g\n",
           (unsigned long long)total, soxr_delay(engine));
    assert(soxr_set_io_ratio(engine, 40000.0 / 48000.0, 0) == NULL);
    total += trace_direct_feed(engine, 40000, 40000);
    printf("TRACE directVR after40000=%llu delay=%.12g\n",
           (unsigned long long)total, soxr_delay(engine));
    float output[4096];
    for (unsigned iteration = 0; iteration < 32; ++iteration)
    {
        size_t consumed, produced;
        const size_t capacity = iteration == 0 ? 102 : ARRAY_SIZE(output);
        assert(soxr_process(engine, NULL, 0, &consumed, output, capacity, &produced) == NULL);
        assert(consumed == 0 && produced <= capacity);
        total += produced;
        printf("TRACE directVR drain=%u capacity=%zu produced=%zu cumulative=%llu\n",
               iteration, capacity, produced, (unsigned long long)total);
        if (produced == 0) break;
        assert(iteration < 31);
    }
    printf("TRACE directVR ideal=96000 delta=%lld combinedOracle=%llu\n",
           (long long)total - 96000, (unsigned long long)(total + 96000));
    delete_filter(filter);
}
int main(void)
{
    const char *args[] = {"--ignore-config", "--no-media-library", "--quiet"};
    libvlc_instance_t *instance = libvlc_new(ARRAY_SIZE(args), args);
    assert(instance);
    filter_t *filter = new_filter(instance, 44100, 48000, true);
    struct statistics stats = {0};
    feed(filter, 44100, VLC_TICK_0, 220, &stats);
    trace_state(filter, &stats, "after44100");
    filter->fmt_in.audio.i_rate = 48000;
    feed(filter, 48000, VLC_TICK_0 + VLC_TICK_FROM_SEC(1), 330, &stats);
    trace_state(filter, &stats, "after48000");
    filter->fmt_in.audio.i_rate = 40000;
    feed(filter, 40000, VLC_TICK_0 + VLC_TICK_FROM_SEC(2), 440, &stats);
    trace_state(filter, &stats, "after40000");
    filter->fmt_in.audio.i_rate = 44100;
    feed(filter, 44100, VLC_TICK_0 + VLC_TICK_FROM_SEC(3), 550, &stats);
    trace_state(filter, &stats, "after44100again");
    collect(filter->ops->drain_audio(filter), &stats);
    trace_state(filter, &stats, "afterDrain");
    printf("TRACE expected=192000 tolerance=10 delta=%lld peak=%.12g lastEnergy=%.12g\n",
           (long long)stats.samples - 192000, stats.peak, stats.last_energy);
    assert(filter->ops->drain_audio(filter) == NULL);
    delete_filter(filter);
    trace_direct_variable(instance);
    for (size_t i = 0; i < tracked_count; ++i)
        assert(tracked[i].releases == 1 && !tracked[i].live);
    assert(live_count == 0);
    libvlc_release(instance);
    return 0;
}
