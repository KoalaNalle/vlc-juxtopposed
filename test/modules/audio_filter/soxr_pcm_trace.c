// SPDX-License-Identifier: GPL-2.0-or-later
// Bounded independent diagnostic; registered acceptance/tolerance are unchanged.
#define main soxr_current_acceptance_main
#include "soxr_lifecycle.c"
#undef main

static void trace_compare(const char *label, const struct pcm_vector *actual,
                           const struct pcm_vector *reference)
{
    size_t first = SIZE_MAX, worst = 0, failures = 0;
    double maximum = 0;
    for (size_t i = 0; i < __MIN(actual->samples, reference->samples); ++i)
    {
        const double difference = fabs(actual->pcm[i] - reference->pcm[i]);
        assert(isfinite(difference));
        if (difference > 0.00001)
        { if (first == SIZE_MAX) first = i; failures++; }
        if (difference > maximum) { maximum = difference; worst = i; }
    }
    printf("TRACE %s actual=%zu reference=%zu differences=%zu max=%.12g worst=%zu first=%llu\n",
           label, actual->samples, reference->samples, failures, maximum, worst,
           (unsigned long long)first);
    if (first != SIZE_MAX)
        for (size_t i = first > 2 ? first - 2 : 0;
             i < __MIN(first + 5, __MIN(actual->samples, reference->samples)); ++i)
            printf("TRACE sample=%zu actual=%.12g reference=%.12g abs=%.12g\n",
                   i, actual->pcm[i], reference->pcm[i],
                   fabs(actual->pcm[i] - reference->pcm[i]));
    fflush(stdout);
}
static struct pcm_vector trace_reference(bool initialize_initial_vr_ratio)
{
    struct pcm_vector reference = {0};
    reference_fixed(44100, 44100, 220, &reference);
    const double configured_ratio = 48000.0 / 44100.0;
    soxr_t variable = reference_engine(configured_ratio, true);
    if (initialize_initial_vr_ratio)
        assert(soxr_set_io_ratio(variable, 1.0 / configured_ratio, 0) == NULL);
    assert(soxr_set_io_ratio(variable, 1.0, 0) == NULL);
    reference_feed(variable, 48000, 48000, 330, &reference);
    assert(soxr_set_io_ratio(variable, 40000.0 / 48000.0, 0) == NULL);
    reference_feed(variable, 40000, 40000, 440, &reference);
    reference_complete(variable, &reference);
    soxr_delete(variable);
    reference_fixed(44100, 44100, 550, &reference);
    return reference;
}
int main(void)
{
    const char *args[] = {"--ignore-config", "--no-media-library", "--quiet"};
    libvlc_instance_t *instance = libvlc_new(ARRAY_SIZE(args), args);
    assert(instance);
    struct pcm_vector actual = {0};
    filter_t *filter = new_filter(instance, 44100, 48000, true);
    struct statistics stats = {0};
    native_feed_pcm(filter, 44100, VLC_TICK_0, 220, false, &stats, &actual);
    filter->fmt_in.audio.i_rate = 48000;
    native_feed_pcm(filter, 48000, VLC_TICK_0 + VLC_TICK_FROM_SEC(1), 330, false, &stats, &actual);
    filter->fmt_in.audio.i_rate = 40000;
    native_feed_pcm(filter, 40000, VLC_TICK_0 + VLC_TICK_FROM_SEC(2), 440, false, &stats, &actual);
    filter->fmt_in.audio.i_rate = 44100;
    native_feed_pcm(filter, 44100, VLC_TICK_0 + VLC_TICK_FROM_SEC(3), 550, false, &stats, &actual);
    collect_pcm(filter->ops->drain_audio(filter), &stats, &actual);
    delete_filter(filter);
    struct pcm_vector original_reference = trace_reference(false);
    struct pcm_vector initialized_reference = trace_reference(true);
    trace_compare("create-only-VR-reference", &actual, &original_reference);
    trace_compare("SDK-initial-ratio-reference", &actual, &initialized_reference);
    free(actual.pcm);
    free(original_reference.pcm);
    free(initialized_reference.pcm);
    assert(live_count == 0);
    libvlc_release(instance);
    return 0;
}
