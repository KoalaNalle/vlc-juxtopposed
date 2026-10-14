// SPDX-License-Identifier: LGPL-2.1-or-later
// Real loaded FLAC packetizer: encoded fixture identity, bounded malformed IO.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#undef NDEBUG
#include <assert.h>
#include <vlc_common.h>
#include <vlc_codec.h>
#include <vlc_modules.h>
#include <vlc_fs.h>
#include <vlc/vlc.h>
#include "../../../lib/libvlc_internal.h"
#include "../../../modules/packetizer/flac.h"
#ifdef _WIN32
# include <windows.h>
#else
# include <sys/mman.h>
# include <unistd.h>
#endif

struct fixture
{
    uint8_t *file;
    size_t bytes;
    const uint8_t *frames;
    size_t frame_bytes;
    struct flac_stream_info info;
};
struct owner { decoder_t decoder; es_format_t input; };
struct collected { uint8_t *bytes; size_t count; unsigned packets, samples; vlc_tick_t first_pts; };

static struct fixture read_fixture(const char *directory, const char *name)
{
    char *path;
    assert(asprintf(&path, "%s/%s", directory, name) >= 0);
    FILE *file = vlc_fopen(path, "rb");
    free(path);
    assert(file);
    assert(fseek(file, 0, SEEK_END) == 0);
    const long length = ftell(file);
    assert(length >= 42 && length <= 512 * 1024);
    assert(fseek(file, 0, SEEK_SET) == 0);
    struct fixture result = {0};
    result.bytes = length;
    result.file = malloc(result.bytes);
    assert(result.file);
    assert(fread(result.file, 1, result.bytes, file) == result.bytes);
    assert(fclose(file) == 0);
    // These are the real forced encoder's preserved streaming-header files.
    assert(!memcmp(result.file, "fLaC\x80\0\0\x22", 8));
    FLAC_ParseStreamInfo(result.file + 8, &result.info);
    assert(result.info.sample_rate == 44100 && result.info.channels == 1);
    assert(result.info.bits_per_sample == 16);
    result.frames = result.file + 42;
    result.frame_bytes = result.bytes - 42;
    return result;
}
static decoder_t *new_packetizer(libvlc_instance_t *instance, const struct fixture *fixture)
{
    struct owner *owner = vlc_object_create(instance->p_libvlc_int, sizeof(*owner));
    assert(owner);
    decoder_t *decoder = &owner->decoder;
    es_format_Init(&owner->input, AUDIO_ES, VLC_CODEC_FLAC);
    owner->input.audio.i_rate = fixture->info.sample_rate;
    owner->input.audio.i_channels = fixture->info.channels;
    owner->input.audio.i_bitspersample = fixture->info.bits_per_sample;
    owner->input.i_extra = FLAC_STREAMINFO_SIZE;
    owner->input.p_extra = malloc(FLAC_STREAMINFO_SIZE);
    assert(owner->input.p_extra);
    memcpy(owner->input.p_extra, fixture->file + 8, FLAC_STREAMINFO_SIZE);
    decoder->fmt_in = &owner->input;
    es_format_Init(&decoder->fmt_out, AUDIO_ES, 0);
    decoder->p_module = module_need(decoder, "audio packetizer", "flac", true);
    assert(decoder->p_module && decoder->pf_packetize && decoder->pf_flush);
    assert(strcmp(module_get_object(decoder->p_module), "flac") == 0);
    return decoder;
}
static void delete_packetizer(decoder_t *decoder)
{
    struct owner *owner = container_of(decoder, struct owner, decoder);
    module_unneed(decoder, decoder->p_module);
    es_format_Clean(&decoder->fmt_out);
    es_format_Clean(&owner->input);
    vlc_object_delete(decoder);
}
static void collect(struct collected *result, block_t *output, const struct fixture *fixture)
{
    while (output)
    {
        block_t *next = output->p_next;
        assert(result->packets < 64 && result->count + output->i_buffer <= 512 * 1024);
        struct flac_header_info frame;
        assert(FLAC_ParseSyncInfo(output->p_buffer, output->i_buffer, &fixture->info, NULL, &frame) != 0);
        uint8_t *bytes = realloc(result->bytes, result->count + output->i_buffer);
        assert(bytes);
        result->bytes = bytes;
        memcpy(bytes + result->count, output->p_buffer, output->i_buffer);
        result->count += output->i_buffer;
        result->samples += frame.i_frame_length;
        if (result->packets++ == 0) result->first_pts = output->i_pts;
        assert(output->i_pts != VLC_TICK_INVALID);
        assert(output->i_length > 0);
        output->p_next = NULL;
        block_Release(output);
        output = next;
    }
}
static void push(decoder_t *decoder, const struct fixture *fixture,
                  const uint8_t *data, size_t bytes, size_t chunk,
                  vlc_tick_t first_pts, struct collected *result)
{
    for (size_t offset = 0; offset < bytes;)
    {
        const size_t length = __MIN(chunk, bytes - offset);
        block_t *input = block_Alloc(length);
        assert(input);
        memcpy(input->p_buffer, data + offset, length);
        input->i_pts = input->i_dts = offset == 0 ? first_pts : VLC_TICK_INVALID;
        unsigned calls = 0;
        block_t *output;
        while ((output = decoder->pf_packetize(decoder, &input)))
        {
            assert(++calls < 64);
            collect(result, output, fixture);
        }
        assert(input == NULL); // Actual module consumes every offered block.
        offset += length;
    }
}
static void drain(decoder_t *decoder, const struct fixture *fixture, struct collected *result)
{
    block_t *output;
    unsigned calls = 0;
    while ((output = decoder->pf_packetize(decoder, NULL)))
    {
        assert(++calls < 64);
        collect(result, output, fixture);
    }
    assert(decoder->pf_packetize(decoder, NULL) == NULL);
}
static void one_short_final_frame(libvlc_instance_t *instance, const struct fixture *one)
{
    assert(one->frame_bytes < FLAC_HEADER_SIZE_MAX && one->frame_bytes >= FLAC_HEADER_SIZE_MIN);
    decoder_t *decoder = new_packetizer(instance, one);
    struct collected result = {0};
    const vlc_tick_t pts = VLC_TICK_0 + VLC_TICK_FROM_SEC(7);
    push(decoder, one, one->frames, one->frame_bytes, one->frame_bytes, pts, &result);
    drain(decoder, one, &result);
    assert(result.packets == 1 && result.samples == 1 && result.first_pts == pts);
    assert(result.count == one->frame_bytes && !memcmp(result.bytes, one->frames, result.count));
    free(result.bytes);
    delete_packetizer(decoder);
    puts("PASS flac_short_final_frame_and_repeated_drain");
}
static void split_frame_drain(libvlc_instance_t *instance, const struct fixture *mixed)
{
    const size_t chunks[] = {1, 2, 3, 7, 17};
    for (size_t i = 0; i < ARRAY_SIZE(chunks); ++i)
    {
        decoder_t *decoder = new_packetizer(instance, mixed);
        struct collected result = {0};
        push(decoder, mixed, mixed->frames, mixed->frame_bytes, chunks[i], VLC_TICK_0, &result);
        drain(decoder, mixed, &result);
        assert(result.packets == 2 && result.samples == mixed->info.max_blocksize + 1);
        assert(result.count == mixed->frame_bytes && !memcmp(result.bytes, mixed->frames, result.count));
        free(result.bytes);
        delete_packetizer(decoder);
    }
    puts("PASS flac_split_native_frames_and_complete_drain");
}
static void every_truncated_short_frame(libvlc_instance_t *instance, const struct fixture *one)
{
    for (size_t length = 0; length < one->frame_bytes; ++length)
    {
        decoder_t *decoder = new_packetizer(instance, one);
        struct collected result = {0};
        push(decoder, one, one->frames, length, 1, VLC_TICK_0, &result);
        drain(decoder, one, &result);
        assert(result.packets == 0 && result.count == 0);
        free(result.bytes);
        delete_packetizer(decoder);
    }
    puts("PASS flac_every_truncated_short_frame_rejected");
}
static void bad_crc_then_flush_and_valid_signal(libvlc_instance_t *instance, const struct fixture *one)
{
    decoder_t *decoder = new_packetizer(instance, one);
    uint8_t bytes[FLAC_HEADER_SIZE_MAX];
    assert(one->frame_bytes < sizeof(bytes));
    memcpy(bytes, one->frames, one->frame_bytes);
    bytes[one->frame_bytes - 1] ^= 1; // Real frame, deliberately wrong payload CRC.
    struct collected bad = {0}, valid = {0};
    push(decoder, one, bytes, one->frame_bytes, 3, VLC_TICK_0, &bad);
    drain(decoder, one, &bad);
    assert(bad.packets == 0);
    decoder->pf_flush(decoder);
    push(decoder, one, one->frames, one->frame_bytes, 3, VLC_TICK_0 + VLC_TICK_FROM_SEC(11), &valid);
    drain(decoder, one, &valid);
    assert(valid.packets == 1 && valid.samples == 1);
    assert(valid.first_pts == VLC_TICK_0 + VLC_TICK_FROM_SEC(11));
    assert(valid.count == one->frame_bytes && !memcmp(valid.bytes, one->frames, valid.count));
    free(bad.bytes); free(valid.bytes);
    delete_packetizer(decoder);
    puts("PASS flac_bad_crc_flush_then_valid_frame");
}
static void flush_discards_partial_lookahead(libvlc_instance_t *instance, const struct fixture *one)
{
    decoder_t *decoder = new_packetizer(instance, one);
    struct collected result = {0};
    push(decoder, one, one->frames, one->frame_bytes - 1, 1, VLC_TICK_0, &result);
    decoder->pf_flush(decoder);
    drain(decoder, one, &result);
    assert(result.packets == 0);
    push(decoder, one, one->frames, one->frame_bytes, 1, VLC_TICK_0 + VLC_TICK_FROM_SEC(13), &result);
    drain(decoder, one, &result);
    assert(result.samples == 1 && result.first_pts == VLC_TICK_0 + VLC_TICK_FROM_SEC(13));
    free(result.bytes);
    delete_packetizer(decoder);
    puts("PASS flac_flush_discards_partial_lookahead");
}
static void guarded_truncated_two_byte_blocksize(const struct fixture *one)
{
    // Nine-byte minimum header, four-byte frame number, only one of the two
    // required blocksize-hint bytes. An old parser reads into the guard page.
    const uint8_t malformed[] = {0xff, 0xf8, 0x70, 0x08, 0xf0, 0x80, 0x80, 0x80, 0};
    uint8_t *allocation;
    size_t page;
#ifdef _WIN32
    SYSTEM_INFO info; GetSystemInfo(&info); page = info.dwPageSize;
    allocation = VirtualAlloc(NULL, page * 2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    assert(allocation);
    DWORD old;
    assert(VirtualProtect(allocation + page, page, PAGE_NOACCESS, &old));
#else
    page = sysconf(_SC_PAGESIZE);
    allocation = mmap(NULL, page * 2, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(allocation != MAP_FAILED);
    assert(mprotect(allocation + page, page, PROT_NONE) == 0);
#endif
    uint8_t *header = allocation + page - sizeof(malformed);
    memcpy(header, malformed, sizeof(malformed));
    struct flac_header_info parsed;
    assert(FLAC_ParseSyncInfo(header, sizeof(malformed), &one->info, NULL, &parsed) == 0);
#ifdef _WIN32
    assert(VirtualFree(allocation, 0, MEM_RELEASE));
#else
    assert(munmap(allocation, page * 2) == 0);
#endif
    puts("PASS flac_truncated_blocksize_guarded_shared_header");
}
int main(void)
{
    const char *directory = getenv("VLC_FLAC_BOUNDARY_DIR");
    assert(directory && *directory);
    const char *args[] = {"--ignore-config", "--no-media-library", "--verbose=2",
        "--file-logging", "--logfile=packetizer-flac-extra.log"};
    libvlc_instance_t *instance = libvlc_new(ARRAY_SIZE(args), args);
    assert(instance);
    const struct fixture one = read_fixture(directory, "boundary-one.flac");
    const struct fixture mixed = read_fixture(directory, "boundary-B-plus-one.flac");
    one_short_final_frame(instance, &one);
    split_frame_drain(instance, &mixed);
    every_truncated_short_frame(instance, &one);
    bad_crc_then_flush_and_valid_signal(instance, &one);
    flush_discards_partial_lookahead(instance, &one);
    guarded_truncated_two_byte_blocksize(&one);
    free(one.file); free(mixed.file);
    libvlc_release(instance);
    puts("PASS flac_packetizer_native_extra_cleanup");
    return 0;
}
