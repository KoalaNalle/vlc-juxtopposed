// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include <vlc_demux.h>
#include <vlc_block.h>
#include "../../libvlc/test.h"
#include "../../../lib/libvlc_internal.h"

struct capture {
    es_out_t out;
    const uint8_t *data;
    size_t data_size;
    unsigned rate, frame_size, packets, bad_lengths, bad_pts, bad_data;
    uint64_t next_sample;
    vlc_tick_t pcr, duration;
};

static es_out_id_t *add(es_out_t *out, input_source_t *source, const es_format_t *fmt)
{
    VLC_UNUSED(source);
    assert(fmt->i_cat == AUDIO_ES);
    return (es_out_id_t *)out;
}

static int send(es_out_t *out, es_out_id_t *id, block_t *block)
{
    struct capture *capture = (struct capture *)out;
    assert(id == (es_out_id_t *)out);
    const uint64_t samples = block->i_buffer / capture->frame_size;
    const vlc_tick_t start = vlc_tick_from_samples(capture->next_sample, capture->rate);
    const vlc_tick_t end = vlc_tick_from_samples(capture->next_sample + samples, capture->rate);
    capture->bad_lengths += block->i_length != end - start;
    capture->bad_pts += block->i_pts != VLC_TICK_0 + start
                     || block->i_dts != block->i_pts || capture->pcr != block->i_pts;
    const size_t offset = capture->next_sample * capture->frame_size;
    capture->bad_data += block->i_buffer % capture->frame_size != 0
                     || offset > capture->data_size
                     || block->i_buffer > capture->data_size - offset
                     || memcmp(block->p_buffer, capture->data + offset, block->i_buffer);
    capture->next_sample += samples;
    capture->duration += block->i_length;
    capture->packets++;
    block_Release(block);
    return VLC_SUCCESS;
}

static void del(es_out_t *out, es_out_id_t *id) { VLC_UNUSED(out);VLC_UNUSED(id); }
static void destroy(es_out_t *out) { VLC_UNUSED(out); }
static int control(es_out_t *out, input_source_t *source, int query, va_list args)
{
    VLC_UNUSED(source);
    struct capture *capture = (struct capture *)out;
    if (query == ES_OUT_SET_PCR) {
        capture->pcr = va_arg(args, vlc_tick_t);
        return VLC_SUCCESS;
    }
    if (query == ES_OUT_RESET_PCR) {
        capture->pcr = VLC_TICK_INVALID;
        return VLC_SUCCESS;
    }
    return VLC_EGENERIC;
}
static const struct es_out_callbacks callbacks = {
    .add=add, .send=send, .del=del, .control=control, .destroy=destroy,
};

static bool run(libvlc_instance_t *vlc, const char *name, unsigned rate,
                unsigned channels, unsigned bits, unsigned format,
                unsigned samples, unsigned seek_samples)
{
    const unsigned frame_size = channels * (bits / 8);
    const size_t data_size = (size_t)samples * frame_size;
    const size_t size = 44 + data_size + (data_size & 1);
    uint8_t *wav = calloc(1, size);
    assert(wav);
    memcpy(wav, "RIFF", 4);SetDWLE(wav+4, size-8);
    memcpy(wav+8, "WAVEfmt ", 8);SetDWLE(wav+16, 16);
    SetWLE(wav+20, format);SetWLE(wav+22, channels);
    SetDWLE(wav+24, rate);SetDWLE(wav+28, rate*frame_size);
    SetWLE(wav+32, frame_size);SetWLE(wav+34, bits);
    memcpy(wav+36, "data", 4);SetDWLE(wav+40, data_size);
    for (size_t i=0; i<data_size; i++) wav[44+i] = (i*17+3)%251;
    struct capture capture = {.out={.cbs=&callbacks}, .data=wav+44,
        .data_size=data_size, .rate=rate, .frame_size=frame_size,
        .next_sample=seek_samples, .pcr=VLC_TICK_INVALID};
    stream_t *stream = vlc_stream_MemoryNew(vlc->p_libvlc_int, wav, size, true);
    assert(stream);
    demux_t *demux = demux_New(VLC_OBJECT(vlc->p_libvlc_int), "wav",
                                "memory://owned.wav", stream, &capture.out);
    if (!demux) {
        vlc_stream_Delete(stream);free(wav);
        printf("FAIL: %s (WAV module could not open fixture)\n", name);
        return false;
    }
    bool seek_ok = !seek_samples || demux_Control(demux, DEMUX_SET_TIME,
        vlc_tick_from_samples(seek_samples, rate), true) == VLC_SUCCESS;
    int result = VLC_DEMUXER_SUCCESS;
    unsigned calls=0;
    while (seek_ok && result == VLC_DEMUXER_SUCCESS && calls++<256)
        result = demux_Demux(demux);
    const vlc_tick_t expected = vlc_tick_from_samples(samples, rate)
                            - vlc_tick_from_samples(seek_samples, rate);
    bool passed = seek_ok && result == VLC_DEMUXER_EOF && capture.packets>0
               && capture.next_sample == samples && capture.duration == expected
               && !capture.bad_lengths && !capture.bad_pts && !capture.bad_data;
    printf("%s: %s packets=%u bad-lengths=%u bad-pts=%u bad-data=%u duration=%"PRId64" expected=%"PRId64"\n",
        passed?"PASS":"FAIL", name, capture.packets, capture.bad_lengths,
        capture.bad_pts, capture.bad_data, capture.duration, expected);
    demux_Delete(demux);
    free(wav);
    return passed;
}

int main(void)
{
    if (getenv("VLC_PLUGIN_PATH") == NULL) test_init();
    const char *args[] = {"--ignore-config", "--no-media-library", "--quiet"};
    libvlc_instance_t *vlc = libvlc_new(ARRAY_SIZE(args), args);
    assert(vlc);
    unsigned failures=0;
    failures += !run(vlc,"exact stereo",44100,2,16,1,88200,0);
    failures += !run(vlc,"partial stereo tail",44100,2,16,1,88641,0);
    failures += !run(vlc,"short mono",44100,1,16,1,441,0);
    failures += !run(vlc,"fractional packet duration",11025,1,16,1,6000,0);
    failures += !run(vlc,"48k partial tail",48000,2,16,1,4901,0);
    failures += !run(vlc,"single sample frame",44100,2,16,1,1,0);
    failures += !run(vlc,"float stereo",48000,2,32,3,3199,0);
    failures += !run(vlc,"mu-law short odd payload",8000,1,8,7,399,0);
    failures += !run(vlc,"seek then final packet",44100,2,16,1,88200,44100);
    libvlc_release(vlc);
    printf("RESULT: 9 cases, %u failures\n",failures);
    return failures ? 1 : 0;
}
