// SPDX-License-Identifier: LGPL-2.1-or-later
// Real loaded SoXR converter: timestamps follow emitted samples, not input calls.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#undef NDEBUG
#include <vlc_common.h>
#include <vlc_aout.h>
#include <vlc_filter.h>
#include <vlc_modules.h>
#include <math.h>
#include "../../libvlc/test.h"
#include "../../../lib/libvlc_internal.h"

struct clock_check {
    unsigned rate, channels, blocks, bad_pts, bad_lengths;
    uint64_t samples;
    vlc_tick_t base;
    double peak;
    bool unanchored;
};

static filter_t *new_filter(libvlc_instance_t *vlc, unsigned input_rate,
                            unsigned output_rate, unsigned channels, bool variable)
{
    filter_t *filter = vlc_object_create(vlc->p_libvlc_int, sizeof(*filter));
    assert(filter);
    es_format_Init(&filter->fmt_in, AUDIO_ES, VLC_CODEC_FL32);
    es_format_Init(&filter->fmt_out, AUDIO_ES, VLC_CODEC_FL32);
    filter->fmt_in.audio.i_format = filter->fmt_out.audio.i_format = VLC_CODEC_FL32;
    filter->fmt_in.audio.i_rate = input_rate;
    filter->fmt_out.audio.i_rate = output_rate;
    filter->fmt_in.audio.i_channels = filter->fmt_out.audio.i_channels = channels;
    filter->fmt_in.audio.i_physical_channels = filter->fmt_out.audio.i_physical_channels =
        channels == 1 ? AOUT_CHAN_CENTER : AOUT_CHANS_STEREO;
    aout_FormatPrepare(&filter->fmt_in.audio);
    aout_FormatPrepare(&filter->fmt_out.audio);
    filter->p_cfg = NULL;
    var_Create(filter, "soxr-resampler-quality", VLC_VAR_INTEGER);
    var_SetInteger(filter, "soxr-resampler-quality", 2);
    filter->p_module = module_need(filter, variable ? "audio resampler" : "audio converter",
                                   "soxr", true);
    assert(filter->p_module && !strcmp(module_get_object(filter->p_module), "soxr"));
    assert(filter->ops && filter->ops->drain_audio && filter->ops->flush);
    return filter;
}

static void delete_filter(filter_t *filter)
{
    module_unneed(filter, filter->p_module);
    es_format_Clean(&filter->fmt_in);
    es_format_Clean(&filter->fmt_out);
    vlc_object_delete(filter);
}

static block_t *input_block(filter_t *filter, unsigned frames, uint64_t offset,
                             vlc_tick_t base, bool missing_pts)
{
    const unsigned rate = filter->fmt_in.audio.i_rate;
    const unsigned channels = filter->fmt_in.audio.i_channels;
    block_t *input = block_Alloc(frames * channels * sizeof(float));
    assert(input);
    float *data = (float *)input->p_buffer;
    for (unsigned i = 0; i < frames; ++i)
        for (unsigned c = 0; c < channels; ++c)
            data[i * channels + c] = .25f * sin(2 * 3.14159265358979323846 *
                                               (220 + c * 110) * (offset + i) / rate);
    input->i_nb_samples = frames;
    input->i_pts = missing_pts ? VLC_TICK_INVALID : base + vlc_tick_from_samples(offset, rate);
    input->i_length = vlc_tick_from_samples(frames, rate);
    return input;
}

static void collect(block_t *output, struct clock_check *check)
{
    if (!output) return;
    assert(!output->p_next);
    assert(output->i_buffer == output->i_nb_samples * check->channels * sizeof(float));
    const vlc_tick_t start = vlc_tick_from_samples(check->samples, check->rate);
    const vlc_tick_t end = vlc_tick_from_samples(check->samples + output->i_nb_samples, check->rate);
    check->bad_pts += output->i_pts != (check->unanchored ? VLC_TICK_INVALID : check->base + start);
    check->bad_lengths += output->i_length != end - start;
    const float *data = (const float *)output->p_buffer;
    for (unsigned i = 0; i < output->i_nb_samples * check->channels; ++i) {
        assert(isfinite(data[i]) && fabs(data[i]) < .5);
        check->peak = __MAX(check->peak, fabs(data[i]));
    }
    check->samples += output->i_nb_samples;
    check->blocks++;
    block_Release(output);
}

static void feed(filter_t *filter, unsigned frames, vlc_tick_t base,
                 struct clock_check *check, unsigned missing_pts, bool discontinuity, int jump)
{
    const unsigned sizes[] = {1, 13, 511, 17, 2205, 127};
    bool jumped = false;
    for (unsigned offset = 0, call = 0; offset < frames; ++call) {
        const unsigned count = __MIN(sizes[call % ARRAY_SIZE(sizes)], frames - offset);
        if (jump && !jumped && offset >= frames / 2) {
            const vlc_tick_t delta = VLC_TICK_FROM_SEC(2) * jump;
            base += delta;
            check->base += delta;
            jumped = true;
        }
        block_t *input = input_block(filter, count, offset, base, missing_pts == 2 || (missing_pts == 1 && offset > 0));
        if (discontinuity && offset == 0) input->i_flags |= BLOCK_FLAG_DISCONTINUITY;
        collect(filter->ops->filter_audio(filter, input), check);
        offset += count;
    }
}

static bool finish(filter_t *filter, struct clock_check *check, const char *name,
                   uint64_t expected)
{
    collect(filter->ops->drain_audio(filter), check);
    bool repeated_empty = filter->ops->drain_audio(filter) == NULL;
    bool passed = repeated_empty && check->samples == expected && check->blocks > 0
               && check->bad_pts == 0 && check->bad_lengths == 0 && check->peak > .2;
    printf("%s: %s samples=%"PRIu64" expected=%"PRIu64" blocks=%u bad-pts=%u bad-lengths=%u\n",
           passed ? "PASS" : "FAIL", name, check->samples, expected, check->blocks,
           check->bad_pts, check->bad_lengths);
    return passed;
}

static bool run(libvlc_instance_t *vlc, const char *name, unsigned input_rate,
                unsigned output_rate, unsigned channels, unsigned frames, unsigned mode)
{
    filter_t *filter = new_filter(vlc, input_rate, output_rate, channels, false);
    struct clock_check check = {.rate=output_rate, .channels=channels,
                                .base=VLC_TICK_0 + VLC_TICK_FROM_SEC(5)};
    const vlc_tick_t input_base = check.base;
    if (mode == 7) {
        // Preserve the unknown head; the next PTS locates its retained audio.
        check.base -= vlc_tick_from_samples(17, input_rate);
        check.unanchored = true;
        collect(filter->ops->filter_audio(filter,
            input_block(filter, 17, 0, VLC_TICK_0, true)), &check);
        check.unanchored = false;
    }
    if (mode == 8) check.unanchored = true;
    if (mode == 1 || mode == 2) {
        struct clock_check discarded = {.rate=output_rate, .channels=channels, .base=VLC_TICK_0};
        feed(filter, 1000, VLC_TICK_0, &discarded, false, false, 0);
        if (mode == 1) {
            filter->ops->flush(filter);
            assert(filter->ops->drain_audio(filter) == NULL);
        }
    }
    feed(filter, frames, input_base, &check, mode == 8 ? 2 : mode == 3 ? 1 : 0, mode == 2, mode == 5 ? 1 : mode == 6 ? -1 : 0);
    const uint64_t expected = ((uint64_t)(frames + (mode == 7 ? 17 : 0)) * output_rate + input_rate / 2) / input_rate;
    bool passed = finish(filter, &check, name, expected);
    if (mode == 4) {
        // A completed drain starts another independent stream at its own PTS.
        struct clock_check repeated = {.rate=output_rate, .channels=channels,
                                       .base=VLC_TICK_0 + VLC_TICK_FROM_SEC(10)};
        feed(filter, frames, repeated.base, &repeated, false, false, 0);
        passed = finish(filter, &repeated, "fresh stream after completed drain", expected) && passed;
    }
    delete_filter(filter);
    return passed;
}

static bool adaptive_contract(libvlc_instance_t *vlc)
{
    filter_t *filter = new_filter(vlc, 44100, 48000, 1, true);
    unsigned failed = 0;
    const unsigned rates[] = {44100, 48000, 40000, 44100};
    for (unsigned phase = 0; phase < ARRAY_SIZE(rates); ++phase) {
        filter->fmt_in.audio.i_rate = rates[phase];
        for (unsigned offset = 0; offset < rates[phase]; offset += 511) {
            const unsigned count = __MIN(511u, rates[phase] - offset);
            block_t *input = input_block(filter, count, offset,
                                         VLC_TICK_0 + VLC_TICK_FROM_SEC(phase), false);
            const vlc_tick_t pts = input->i_pts;
            block_t *output = filter->ops->filter_audio(filter, input);
            if (output) {
                // Adaptive playback intentionally follows the input media clock.
                failed += output->i_pts != pts;
                block_Release(output);
            }
        }
    }
    block_t *tail = filter->ops->drain_audio(filter);
    if (tail) block_Release(tail);
    failed += filter->ops->drain_audio(filter) != NULL;
    delete_filter(filter);
    printf("%s: adaptive input-clock/ratio-switch contract failures=%u\n", failed ? "FAIL" : "PASS", failed);
    return failed == 0;
}

static bool generic_chain(libvlc_instance_t *vlc, unsigned input_rate,
                           unsigned output_rate, const char *name)
{
    filter_t *parent = new_filter(vlc, input_rate, output_rate, 1, false);
    var_Create(parent, "audio-time-stretch", VLC_VAR_BOOL);
    var_SetBool(parent, "audio-time-stretch", false);
    aout_filters_t *chain = aout_FiltersNew(parent, &parent->fmt_in.audio,
                                            &parent->fmt_out.audio, NULL);
    assert(chain);
    struct clock_check check = {.rate=output_rate, .channels=1, .base=VLC_TICK_0};
    const unsigned frames = 2 * input_rate;
    for (unsigned offset = 0; offset < frames;) {
        const unsigned count = __MIN(511u, frames - offset);
        collect(aout_FiltersPlay(chain, input_block(parent, count, offset,
                              check.base, false), 1.f), &check);
        offset += count;
    }
    collect(aout_FiltersDrain(chain), &check);
    bool passed = check.samples == 2 * output_rate && !check.bad_pts
               && !check.bad_lengths && check.peak > .2
               && aout_FiltersDrain(chain) == NULL;
    printf("%s: %s samples=%"PRIu64" expected=%u bad-pts=%u bad-lengths=%u\n",
           passed ? "PASS" : "FAIL", name, check.samples, 2 * output_rate,
           check.bad_pts, check.bad_lengths);
    aout_FiltersDelete(parent, chain);
    delete_filter(parent);
    return passed;
}

int main(void)
{
    if (getenv("VLC_PLUGIN_PATH") == NULL) test_init();
    const char *args[] = {"--ignore-config", "--no-media-library", "--quiet"};
    libvlc_instance_t *vlc = libvlc_new(ARRAY_SIZE(args), args);
    assert(vlc);
    unsigned failures = 0;
    failures += !run(vlc,"stereo downsample",44100,8000,2,88200,0);
    failures += !run(vlc,"mono upsample",8000,44100,1,16000,0);
    failures += !run(vlc,"fractional output clock",48000,11025,1,96000,0);
    failures += !run(vlc,"partial tail",44100,48000,1,88641,0);
    failures += !run(vlc,"short buffered stream",8000,44100,1,17,0);
    failures += !run(vlc,"flush reanchors and drops old tail",8000,44100,1,16000,1);
    failures += !run(vlc,"discontinuity reanchors and drops old tail",8000,44100,1,16000,2);
    failures += !run(vlc,"missing later timestamps use sample clock",44100,8000,1,88200,3);
    failures += !run(vlc,"completed drain reanchors",8000,44100,1,16000,4);
    failures += !run(vlc,"forward timestamp jump preserves buffered PCM",44100,48000,1,88200,5);
    failures += !run(vlc,"backward timestamp jump preserves buffered PCM",44100,48000,1,88200,6);
    failures += !run(vlc,"unknown head preserved until timestamp arrives",8000,44100,1,16000,7);
    failures += !run(vlc,"all unknown timestamps retain complete PCM",8000,44100,1,16000,8);
    failures += !adaptive_contract(vlc);
    failures += !generic_chain(vlc, 44100, 8000, "generic no-clock downsample chain");
    failures += !generic_chain(vlc, 8000, 44100, "generic no-clock upsample chain");
    libvlc_release(vlc);
    printf("RESULT: 16 cases, %u failures\n", failures);
    return failures ? 1 : 0;
}
