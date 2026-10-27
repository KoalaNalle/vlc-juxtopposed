// SPDX-License-Identifier: GPL-2.0-or-later
// Independent native SDK oracle; the earlier failed acceptance stays frozen.
#define main soxr_frozen_acceptance_main
#include "soxr_lifecycle.c"
#undef main

struct pcm_vector { float *pcm; size_t samples; };
static void append_pcm(struct pcm_vector *vector, const float *pcm, size_t samples)
{
    assert(samples <= 512000 && vector->samples <= 512000 - samples);
    if (!samples) return;
    float *grown = realloc(vector->pcm, (vector->samples + samples) * sizeof(float));
    assert(grown);
    vector->pcm = grown;
    memcpy(vector->pcm + vector->samples, pcm, samples * sizeof(float));
    vector->samples += samples;
}
static soxr_t reference_engine(double output_ratio, bool variable)
{
    const soxr_io_spec_t io = soxr_io_spec(SOXR_FLOAT32_I, SOXR_FLOAT32_I);
    const soxr_quality_spec_t quality = soxr_quality_spec(variable ? SOXR_LQ : SOXR_MQ,
                                                         variable ? SOXR_VR : 0);
    soxr_error_t error;
    soxr_t engine = soxr_create(1, output_ratio, 1, &error, &io, &quality, NULL);
    assert(engine && !error);
    return engine;
}
// Direct SDK processing uses no production drain/filter function or delay query.
static void reference_feed(soxr_t engine, unsigned samples, unsigned rate,
                           unsigned frequency, struct pcm_vector *vector)
{
    unsigned offset = 0;
    while (offset < samples)
    {
        const unsigned remaining = samples - offset;
        const unsigned frames = remaining <= 17 ? remaining : __MIN(511u, remaining - 17);
        float input[511], output[2048];
        for (unsigned i = 0; i < frames; ++i)
            input[i] = .35f * sin(2 * 3.14159265358979323846 * frequency * (offset + i) / rate);
        size_t consumed, produced;
        assert(soxr_process(engine, input, frames, &consumed, output,
                            ARRAY_SIZE(output), &produced) == NULL);
        assert(consumed == frames && produced <= ARRAY_SIZE(output));
        append_pcm(vector, output, produced);
        offset += frames;
    }
}
static void reference_complete(soxr_t engine, struct pcm_vector *vector)
{
    for (unsigned attempt = 0; attempt < 32; ++attempt)
    {
        float output[2048];
        size_t consumed, produced;
        assert(soxr_process(engine, NULL, 0, &consumed, output,
                            ARRAY_SIZE(output), &produced) == NULL);
        assert(consumed == 0 && produced <= ARRAY_SIZE(output));
        append_pcm(vector, output, produced);
        if (produced == 0) return;
    }
    assert(!"Independent native reference did not finish its bounded drain");
}
static void reference_fixed(unsigned rate, unsigned samples, unsigned frequency,
                            struct pcm_vector *vector)
{
    soxr_t engine = reference_engine(48000.0 / rate, false);
    reference_feed(engine, samples, rate, frequency, vector);
    reference_complete(engine, vector);
    soxr_delete(engine);
}
static void collect_pcm(block_t *output, struct statistics *stats, struct pcm_vector *vector)
{
    if (output)
    {
        assert(!output->p_next && output->i_buffer == output->i_nb_samples * sizeof(float));
        append_pcm(vector, (const float *)output->p_buffer, output->i_nb_samples);
    }
    collect(output, stats);
}
static void native_feed_pcm(filter_t *filter, unsigned samples, vlc_tick_t base,
                            unsigned frequency, bool require_bypass,
                            struct statistics *stats, struct pcm_vector *vector)
{
    unsigned offset = 0;
    const unsigned rate = filter->fmt_in.audio.i_rate;
    while (offset < samples)
    {
        const unsigned remaining = samples - offset;
        const unsigned frames = remaining <= 17 ? remaining : __MIN(511u, remaining - 17);
        block_t *input = signal_block(frames, rate, offset, base, frequency);
        block_t *output = filter->ops->filter_audio(filter, input);
        filter_sys_t *sys = filter->p_sys;
        if (require_bypass)
        {
            assert(output == input);
            assert(sys->f_fixed_ratio == 1.0 && sys->last_soxr == NULL);
        }
        collect_pcm(output, stats, vector);
        offset += frames;
    }
}
static void assert_pcm_identity(const struct pcm_vector *actual, const struct pcm_vector *reference)
{
    printf("REFERENCE actual=%zu independentNative=%zu\n", actual->samples, reference->samples);
    fflush(stdout);
    assert(actual->samples == reference->samples);
    for (size_t i = 0; i < actual->samples; ++i)
        assert(isfinite(actual->pcm[i]) && isfinite(reference->pcm[i]) &&
               fabs(actual->pcm[i] - reference->pcm[i]) <= 0.00001);
}
static void fixed_variable_transitions_reference(libvlc_instance_t *instance)
{
    struct pcm_vector reference = {0}, actual = {0};
    reference_fixed(44100, 44100, 220, &reference);
    soxr_t variable = reference_engine(48000.0 / 44100.0, true);
    assert(soxr_set_io_ratio(variable, 1.0, 0) == NULL);
    reference_feed(variable, 48000, 48000, 330, &reference);
    assert(soxr_set_io_ratio(variable, 40000.0 / 48000.0, 0) == NULL);
    reference_feed(variable, 40000, 40000, 440, &reference);
    reference_complete(variable, &reference);
    soxr_delete(variable);
    reference_fixed(44100, 44100, 550, &reference);
    assert(reference.samples > 190000 && reference.samples < 194000);

    filter_t *filter = new_filter(instance, 44100, 48000, true);
    struct statistics stats = {0};
    native_feed_pcm(filter, 44100, VLC_TICK_0, 220, false, &stats, &actual);
    filter->fmt_in.audio.i_rate = 48000; // A VR 1:1 segment, not bypass.
    native_feed_pcm(filter, 48000, VLC_TICK_0 + VLC_TICK_FROM_SEC(1), 330, false, &stats, &actual);
    filter->fmt_in.audio.i_rate = 40000;
    native_feed_pcm(filter, 40000, VLC_TICK_0 + VLC_TICK_FROM_SEC(2), 440, false, &stats, &actual);
    filter->fmt_in.audio.i_rate = 44100;
    native_feed_pcm(filter, 44100, VLC_TICK_0 + VLC_TICK_FROM_SEC(3), 550, false, &stats, &actual);
    collect_pcm(filter->ops->drain_audio(filter), &stats, &actual);
    assert_pcm_identity(&actual, &reference);
    assert(stats.peak > .3 && stats.last_energy > .01);
    assert(filter->ops->drain_audio(filter) == NULL);
    delete_filter(filter);
    free(reference.pcm);
    free(actual.pcm);
    puts("PASS soxr_fixed_variable_transitions_independent_reference");
}
static void true_bypass_variable_transitions(libvlc_instance_t *instance)
{
    struct pcm_vector reference = {0}, actual = {0};
    // Ratio 1 is actual passthrough, so independently authored input is its PCM oracle.
    for (unsigned phase = 0; phase < 3; ++phase)
    {
        if (phase == 1)
        {
            soxr_t variable = reference_engine(1, true);
            assert(soxr_set_io_ratio(variable, 40000.0 / 48000.0, 0) == NULL);
            reference_feed(variable, 40000, 40000, 330, &reference);
            reference_complete(variable, &reference);
            soxr_delete(variable);
        }
        else
            for (unsigned offset = 0; offset < 48000;)
            {
                float pcm[511];
                const unsigned frames = __MIN(511u, 48000 - offset);
                for (unsigned i = 0; i < frames; ++i)
                    pcm[i] = .35f * sin(2 * 3.14159265358979323846 *
                                       (phase == 0 ? 220 : 440) * (offset + i) / 48000.0);
                append_pcm(&reference, pcm, frames);
                offset += frames;
            }
    }
    filter_t *filter = new_filter(instance, 48000, 48000, true);
    struct statistics stats = {0};
    native_feed_pcm(filter, 48000, VLC_TICK_0, 220, true, &stats, &actual);
    filter->fmt_in.audio.i_rate = 40000;
    native_feed_pcm(filter, 40000, VLC_TICK_0 + VLC_TICK_FROM_SEC(1), 330, false, &stats, &actual);
    filter->fmt_in.audio.i_rate = 48000;
    // First bypass block prepends real VR tail; later blocks must be exact aliases.
    block_t *first = signal_block(511, 48000, 0, VLC_TICK_0 + VLC_TICK_FROM_SEC(2), 440);
    collect_pcm(filter->ops->filter_audio(filter, first), &stats, &actual);
    filter_sys_t *sys = filter->p_sys;
    assert(sys->f_fixed_ratio == 1.0 && sys->last_soxr == NULL);
    for (unsigned offset = 511; offset < 48000;)
    {
        const unsigned remaining = 48000 - offset;
        const unsigned frames = remaining <= 17 ? remaining : __MIN(511u, remaining - 17);
        block_t *input = signal_block(frames, 48000, offset,
            VLC_TICK_0 + VLC_TICK_FROM_SEC(2), 440);
        block_t *output = filter->ops->filter_audio(filter, input);
        assert(output == input && sys->last_soxr == NULL);
        collect_pcm(output, &stats, &actual);
        offset += frames;
    }
    assert(filter->ops->drain_audio(filter) == NULL);
    assert_pcm_identity(&actual, &reference);
    assert(stats.peak > .3 && stats.last_energy > .01);
    delete_filter(filter);
    free(reference.pcm);
    free(actual.pcm);
    puts("PASS soxr_true_bypass_variable_transitions_independent_reference");
}
static void switch_initial_drain_allocation_failure_can_retry(libvlc_instance_t *instance)
{
    filter_t *filter = new_filter(instance, 44100, 48000, true);
    struct statistics stats = {0};
    feed(filter, 1000, VLC_TICK_0, 220, &stats);
    filter_sys_t *sys = filter->p_sys;
    assert(sys->last_soxr == sys->soxr && sys->i_last_olen > 0);
    filter->fmt_in.audio.i_rate = 48000;
    block_t *input = signal_block(17, 48000, 0, VLC_TICK_0 + VLC_TICK_FROM_SEC(1), 220);
    const unsigned before = injected_alloc;
    fail_alloc = true;
    assert(filter->ops->filter_audio(filter, input) == NULL);
    assert(injected_alloc == before + 1 && !fail_alloc && live_count == 0);
    assert(sys->last_soxr == sys->soxr && sys->i_last_olen > 0);
    collect(filter->ops->drain_audio(filter), &stats);
    assert_count(stats.samples, 1088, 1);
    assert(filter->ops->drain_audio(filter) == NULL);
    delete_filter(filter);
    puts("PASS soxr_switch_initial_drain_allocation_failure_can_retry");
}
int main(void)
{
    const char *args[] = {"--ignore-config", "--no-media-library", "--quiet"};
    libvlc_instance_t *instance = libvlc_new(ARRAY_SIZE(args), args);
    assert(instance);
    fixed_complete_and_repeat(instance);
    downsample_complete(instance);
    flush_then_new_signal(instance);
    fixed_variable_transitions_reference(instance);
    true_bypass_variable_transitions(instance);
    allocation_failure_consumes_input(instance);
    alias_process_failure_releases_once(instance);
    gather_failure_releases_preserved_chain(instance);
    drain_allocation_failure_can_retry(instance);
    switch_initial_drain_allocation_failure_can_retry(instance);
    for (size_t i = 0; i < tracked_count; ++i) assert(tracked[i].releases == 1 && !tracked[i].live);
    assert(live_count == 0);
    libvlc_release(instance);
    puts("PASS soxr_native_reference_lifecycle_cleanup");
    return 0;
}
