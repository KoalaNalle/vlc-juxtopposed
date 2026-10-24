// SPDX-License-Identifier: GPL-2.0-or-later
// Source-compiled production SoXR regression with real native blocks/libSoXR.
// Allocation/Gather/process failures are explicit injected API outcomes.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <vlc_common.h>
#include <vlc_aout.h>
#include <vlc_filter.h>
#include <vlc_plugin.h>
#include <vlc/vlc.h>
#include <soxr.h>
#include "../../../lib/libvlc_internal.h"

struct tracked_block
{
    block_t *block;
    const struct vlc_frame_callbacks *original;
    unsigned releases;
    bool live;
};
static struct tracked_block tracked[2048];
static size_t tracked_count, live_count;
static bool fail_alloc, fail_gather, fail_process;
static unsigned injected_alloc, injected_gather, injected_process;
static unsigned injected_alias_process;

static void tracked_free(block_t *block)
{
    for (size_t i = tracked_count; i-- > 0;)
        if (tracked[i].block == block && tracked[i].live)
        {
            assert(tracked[i].releases == 0 && live_count > 0);
            tracked[i].releases++;
            tracked[i].live = false;
            live_count--;
            block->cbs = tracked[i].original;
            tracked[i].original->free(block);
            return;
        }
    assert(!"Untracked or double-released diagnostic native block");
}
static const struct vlc_frame_callbacks tracked_callbacks = {.free = tracked_free};

static block_t *tracked_alloc(size_t bytes)
{
    if (fail_alloc) { fail_alloc = false; injected_alloc++; return NULL; }
    assert(tracked_count < ARRAY_SIZE(tracked));
    block_t *block = block_Alloc(bytes);
    if (block)
    {
        assert(block->cbs && block->cbs->free);
        tracked[tracked_count++] = (struct tracked_block){block, block->cbs, 0, true};
        block->cbs = &tracked_callbacks;
        live_count++;
    }
    return block;
}
static block_t *injected_chain_gather(block_t *chain)
{
    if (fail_gather)
    {
        fail_gather = false;
        injected_gather++;
        return NULL; // Real failed Gather contract retains its complete chain.
    }
    return block_ChainGather(chain);
}
static soxr_error_t injected_soxr_process(soxr_t engine, soxr_in_t input,
    size_t input_length, size_t *input_done, soxr_out_t output,
    size_t output_length, size_t *output_done)
{
    if (fail_process)
    {
        fail_process = false;
        injected_process++;
        if (input == output) injected_alias_process++;
        return "Explicit diagnostic process failure";
    }
    return soxr_process(engine, input, input_length, input_done,
                        output, output_length, output_done);
}

// Include the current production implementation without changing its logic.
// Headers are already loaded, so instrumentation does not rewrite declarations.
#undef MODULE_NAME
#undef MODULE_STRING
#define MODULE_NAME soxr_lifecycle_diagnostic
#define MODULE_STRING "soxr_lifecycle_diagnostic"
// Standalone built-in descriptors omit the dynamic plugin's logging symbol.
// vlc_messages.h requires it for the unchanged production msg_* calls.
const char vlc_module_name[] = MODULE_STRING;
VLC_DECL_MODULE_ENTRY(soxr_lifecycle_diagnostic);
#undef block_Alloc
#undef block_ChainGather
#define block_Alloc tracked_alloc
#define block_ChainGather injected_chain_gather
#define soxr_process injected_soxr_process
#include "../../../modules/audio_filter/resampler/soxr.c"
#undef block_Alloc
#undef block_ChainGather
#undef soxr_process

struct statistics { uint64_t samples; double peak, last_energy; vlc_tick_t last_pts; };

static filter_t *new_filter(libvlc_instance_t *instance, unsigned input_rate,
                            unsigned output_rate, bool variable)
{
    filter_t *filter = vlc_object_create(instance->p_libvlc_int, sizeof(*filter));
    assert(filter);
    es_format_Init(&filter->fmt_in, AUDIO_ES, VLC_CODEC_FL32);
    es_format_Init(&filter->fmt_out, AUDIO_ES, VLC_CODEC_FL32);
    filter->fmt_in.audio.i_format = filter->fmt_out.audio.i_format = VLC_CODEC_FL32;
    filter->fmt_in.audio.i_rate = input_rate;
    filter->fmt_out.audio.i_rate = output_rate;
    filter->fmt_in.audio.i_channels = filter->fmt_out.audio.i_channels = 1;
    filter->fmt_in.audio.i_physical_channels = filter->fmt_out.audio.i_physical_channels = AOUT_CHAN_CENTER;
    aout_FormatPrepare(&filter->fmt_in.audio);
    aout_FormatPrepare(&filter->fmt_out.audio);
    assert(filter->fmt_in.audio.i_bytes_per_frame == sizeof(float));
    assert(filter->fmt_out.audio.i_bytes_per_frame == sizeof(float));
    filter->p_module = NULL;
    filter->p_cfg = NULL;
    var_Create(filter, "soxr-resampler-quality", VLC_VAR_INTEGER);
    assert(var_SetInteger(filter, "soxr-resampler-quality", 2) == VLC_SUCCESS);
    assert((variable ? OpenResampler(VLC_OBJECT(filter)) : OpenConverter(VLC_OBJECT(filter))) == VLC_SUCCESS);
    return filter;
}
static void delete_filter(filter_t *filter)
{
    filter->ops->close(filter);
    es_format_Clean(&filter->fmt_in);
    es_format_Clean(&filter->fmt_out);
    vlc_object_delete(filter);
    assert(live_count == 0);
}
static block_t *signal_block(unsigned frames, unsigned rate, uint64_t offset,
                             vlc_tick_t base, unsigned frequency)
{
    assert(frames && frames <= 1024);
    block_t *block = tracked_alloc(frames * sizeof(float));
    assert(block);
    float *pcm = (float *)block->p_buffer;
    for (unsigned i = 0; i < frames; ++i)
        pcm[i] = .35f * sin(2 * 3.14159265358979323846 * frequency * (offset + i) / rate);
    block->i_nb_samples = frames;
    block->i_pts = base + vlc_tick_from_samples(offset, rate);
    block->i_length = vlc_tick_from_samples(frames, rate);
    return block;
}
static void collect(block_t *output, struct statistics *stats)
{
    if (!output) return;
    assert(!output->p_next);
    assert(output->i_buffer == output->i_nb_samples * sizeof(float));
    assert(output->i_nb_samples < 1000000);
    assert(output->i_pts != VLC_TICK_INVALID);
    if (stats->samples) assert(output->i_pts >= stats->last_pts);
    stats->last_pts = output->i_pts;
    const float *pcm = (const float *)output->p_buffer;
    for (unsigned i = 0; i < output->i_nb_samples; ++i)
    {
        assert(isfinite(pcm[i]) && fabs(pcm[i]) < .5);
        stats->peak = __MAX(stats->peak, fabs(pcm[i]));
    }
    if (output->i_nb_samples)
    {
        stats->last_energy = 0;
        const unsigned start = output->i_nb_samples > 16 ? output->i_nb_samples - 16 : 0;
        for (unsigned i = start; i < output->i_nb_samples; ++i) stats->last_energy += fabs(pcm[i]);
    }
    stats->samples += output->i_nb_samples;
    block_Release(output);
}
static void feed(filter_t *filter, unsigned samples, vlc_tick_t base,
                 unsigned frequency, struct statistics *stats)
{
    const unsigned rate = filter->fmt_in.audio.i_rate;
    unsigned offset = 0;
    while (offset < samples)
    {
        // Leave a very small 17-frame final chunk behind each actual signal.
        const unsigned remaining = samples - offset;
        const unsigned frames = remaining <= 17 ? remaining : __MIN(511u, remaining - 17);
        block_t *input = signal_block(frames, rate, offset, base, frequency);
        collect(filter->ops->filter_audio(filter, input), stats);
        offset += frames;
    }
}
static void assert_count(uint64_t actual, uint64_t expected, unsigned tolerance)
{
    assert(actual + tolerance >= expected && actual <= expected + tolerance);
}
static void fixed_complete_and_repeat(libvlc_instance_t *instance)
{
    filter_t *filter = new_filter(instance, 8000, 44100, false);
    struct statistics stats = {0};
    feed(filter, 16000, VLC_TICK_0, 220, &stats);
    collect(filter->ops->drain_audio(filter), &stats);
    assert_count(stats.samples, 88200, 1);
    assert(stats.peak > .3 && stats.last_energy > .01);
    assert(filter->ops->drain_audio(filter) == NULL);
    assert(filter->ops->drain_audio(filter) == NULL);
    delete_filter(filter);
    puts("PASS soxr_fixed_complete_and_repeat");
}
static void downsample_complete(libvlc_instance_t *instance)
{
    filter_t *filter = new_filter(instance, 44100, 8000, false);
    struct statistics stats = {0};
    feed(filter, 88200, VLC_TICK_0, 220, &stats);
    collect(filter->ops->drain_audio(filter), &stats);
    assert_count(stats.samples, 16000, 1);
    assert(stats.peak > .3 && stats.last_energy > .01);
    delete_filter(filter);
    puts("PASS soxr_downsample_complete");
}
static void flush_then_new_signal(libvlc_instance_t *instance)
{
    filter_t *filter = new_filter(instance, 8000, 44100, false);
    struct statistics discarded = {0}, clean = {0};
    feed(filter, 2000, VLC_TICK_0, 440, &discarded);
    filter->ops->flush(filter);
    assert(filter->ops->drain_audio(filter) == NULL);
    feed(filter, 16000, VLC_TICK_0 + VLC_TICK_FROM_SEC(5), 220, &clean);
    collect(filter->ops->drain_audio(filter), &clean);
    assert_count(clean.samples, 88200, 1);
    assert(clean.peak > .3 && clean.last_energy > .01);
    delete_filter(filter);
    puts("PASS soxr_flush_then_new_signal");
}
static void allocation_failure_consumes_input(libvlc_instance_t *instance)
{
    filter_t *filter = new_filter(instance, 8000, 44100, false);
    block_t *input = signal_block(17, 8000, 0, VLC_TICK_0, 220);
    const unsigned before = injected_alloc;
    fail_alloc = true;
    assert(filter->ops->filter_audio(filter, input) == NULL);
    assert(injected_alloc == before + 1 && !fail_alloc && live_count == 0);
    delete_filter(filter);
    puts("PASS soxr_injected_allocation_failure_consumes_input");
}
static void alias_process_failure_releases_once(libvlc_instance_t *instance)
{
    filter_t *filter = new_filter(instance, 44100, 8000, false);
    block_t *input = signal_block(511, 44100, 0, VLC_TICK_0, 220);
    const unsigned before = injected_process;
    const unsigned aliases = injected_alias_process;
    fail_process = true; // This real source path uses its native input as output.
    assert(filter->ops->filter_audio(filter, input) == NULL);
    assert(injected_process == before + 1 && !fail_process && live_count == 0);
    assert(injected_alias_process == aliases + 1);
    delete_filter(filter);
    puts("PASS soxr_injected_alias_process_failure_releases_once");
}
static void gather_failure_releases_preserved_chain(libvlc_instance_t *instance)
{
    filter_t *filter = new_filter(instance, 44100, 48000, true);
    struct statistics stats = {0};
    feed(filter, 1000, VLC_TICK_0, 220, &stats);
    filter->fmt_in.audio.i_rate = 48000;
    block_t *input = signal_block(17, 48000, 0, VLC_TICK_0 + VLC_TICK_FROM_SEC(1), 220);
    const unsigned before = injected_gather;
    fail_gather = true;
    assert(filter->ops->filter_audio(filter, input) == NULL);
    assert(injected_gather == before + 1 && !fail_gather && live_count == 0);
    delete_filter(filter);
    puts("PASS soxr_injected_gather_failure_releases_preserved_chain");
}
static void drain_allocation_failure_can_retry(libvlc_instance_t *instance)
{
    filter_t *filter = new_filter(instance, 8000, 44100, false);
    struct statistics stats = {0};
    feed(filter, 16000, VLC_TICK_0, 220, &stats);
    const unsigned before = injected_alloc;
    fail_alloc = true;
    assert(filter->ops->drain_audio(filter) == NULL);
    assert(injected_alloc == before + 1 && !fail_alloc && live_count == 0);
    collect(filter->ops->drain_audio(filter), &stats);
    assert_count(stats.samples, 88200, 1);
    assert(stats.last_energy > .01);
    assert(filter->ops->drain_audio(filter) == NULL);
    delete_filter(filter);
    puts("PASS soxr_injected_drain_allocation_failure_can_retry");
}
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
    // SDK example5 configures the initial VR ratio after creation. Its first
    // ratio call initializes stream phase before any later instant change.
    if (variable) assert(soxr_set_io_ratio(engine, 1.0 / output_ratio, 0) == NULL);
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
    puts("PASS soxr_native_lifecycle_cleanup");
    return 0;
}
