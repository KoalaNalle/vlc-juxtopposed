// SPDX-License-Identifier: LGPL-2.1-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include <vlc_codec.h>
#include <vlc_modules.h>
#include <vlc/vlc.h>
#include "../../libvlc/test.h"
#include "../../../lib/libvlc_internal.h"

struct owner {
    decoder_t decoder;
    es_format_t input;
};

static block_t *packet(unsigned char marker, unsigned start, unsigned duration)
{
    block_t *block = block_Alloc(4);
    assert(block);
    memset(block->p_buffer, marker, block->i_buffer);
    block->i_pts = block->i_dts = VLC_TICK_0 + VLC_TICK_FROM_MS(start);
    block->i_length = VLC_TICK_FROM_MS(duration);
    return block;
}

int main(void)
{
    /* Cross-built Windows diagnostics may run against an explicit package;
     * test_init() otherwise replaces its valid path with the build-host path. */
    if (getenv("VLC_PLUGIN_PATH") == NULL)
        test_init();
    const char *args[] = {"--ignore-config", "--no-media-library", "--quiet"};
    libvlc_instance_t *vlc = libvlc_new(ARRAY_SIZE(args), args);
    assert(vlc);
    struct owner *owner = vlc_object_create(vlc->p_libvlc_int, sizeof(*owner));
    assert(owner);
    decoder_t *decoder = &owner->decoder;
    es_format_Init(&owner->input, AUDIO_ES, VLC_CODEC_S16L);
    owner->input.audio.i_rate = 44100;
    owner->input.audio.i_channels = 1;
    owner->input.audio.i_bitspersample = 16;
    decoder->fmt_in = &owner->input;
    es_format_Init(&decoder->fmt_out, AUDIO_ES, 0);
    module_t *module = module_need(decoder, "audio packetizer", "any", false);
    assert(module);
    assert(strcmp(module_get_object(module), "copy") == 0);
    assert(decoder->pf_packetize(decoder, NULL) == NULL);

    block_t *input = packet(1, 100, 17);
    assert(decoder->pf_packetize(decoder, &input) == NULL);
    assert(input == NULL);
    // Exhausting this input's loop must retain lookahead for the next PTS.
    assert(decoder->pf_packetize(decoder, &input) == NULL);
    input = packet(2, 160, 19);
    block_t *out = decoder->pf_packetize(decoder, &input);
    assert(out && input == NULL);
    assert(out->p_buffer[0] == 1 && out->i_length == VLC_TICK_FROM_MS(60));
    block_Release(out);
    assert(decoder->pf_packetize(decoder, &input) == NULL);

    // Explicit draining returns the final packet once, with its own duration.
    out = decoder->pf_packetize(decoder, NULL);
    assert(out && out->p_buffer[0] == 2);
    assert(out->i_length == VLC_TICK_FROM_MS(19));
    block_Release(out);
    assert(decoder->pf_packetize(decoder, NULL) == NULL);

    // A one-packet input is preserved at EOF; an explicit flush discards it.
    input = packet(3, 250, 10);
    assert(decoder->pf_packetize(decoder, &input) == NULL && input == NULL);
    out = decoder->pf_packetize(decoder, NULL);
    assert(out && out->p_buffer[0] == 3);
    block_Release(out);
    input = packet(4, 300, 10);
    assert(decoder->pf_packetize(decoder, &input) == NULL && input == NULL);
    decoder->pf_flush(decoder);
    assert(decoder->pf_packetize(decoder, NULL) == NULL);

    module_unneed(decoder, module);
    es_format_Clean(&decoder->fmt_out);
    es_format_Clean(&owner->input);

    // Draining the generic video path must still classify its final frame.
    es_format_Init(&owner->input, VIDEO_ES, VLC_CODEC_WMV3);
    owner->input.i_extra = 4;
    owner->input.p_extra = calloc(1, owner->input.i_extra);
    assert(owner->input.p_extra);
    es_format_Init(&decoder->fmt_out, VIDEO_ES, 0);
    module = module_need(decoder, "video packetizer", "copy", true);
    assert(module);
    assert(strcmp(module_get_object(module), "copy") == 0);
    input = packet(0x20, 100, 40); // No optional fields; third bit is P-frame.
    input->i_flags |= BLOCK_FLAG_TYPE_I;
    assert(decoder->pf_packetize(decoder, &input) == NULL && input == NULL);
    out = decoder->pf_packetize(decoder, NULL);
    assert(out && (out->i_flags & BLOCK_FLAG_TYPE_MASK) == BLOCK_FLAG_TYPE_P);
    assert(out->i_length == VLC_TICK_FROM_MS(40));
    block_Release(out);
    assert(decoder->pf_packetize(decoder, NULL) == NULL);
    module_unneed(decoder, module);
    es_format_Clean(&decoder->fmt_out);
    es_format_Clean(&owner->input);
    vlc_object_delete(decoder);
    libvlc_release(vlc);
    puts("Passed: actual copy packetizer lookahead, final/single-packet drain, repeated drain, flush ownership and final WMV3 frame classification.");
    return 0;
}
