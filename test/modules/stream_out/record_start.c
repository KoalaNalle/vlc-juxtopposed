// SPDX-License-Identifier: LGPL-2.1-or-later
// Exercise the actual recording start calculation with owned buffered streams.
#define MODULE_STRING "record_start_test"
const char vlc_module_name[] = MODULE_STRING;
#include "../../../modules/stream_out/record.c"
#include <stdio.h>

struct fixture
{
    sout_stream_sys_t sys;
    sout_stream_id_sys_t streams[4];
    sout_stream_id_sys_t *ids[4];
    block_t blocks[4][3];
};

static void add(struct fixture *f, int cat, vlc_tick_t head, vlc_tick_t key)
{
    int n = f->sys.i_id++;
    sout_stream_id_sys_t *id = &f->streams[n];
    f->sys.id = f->ids;
    f->ids[n] = id;
    id->fmt.i_cat = cat;
    // The native calculation only checks this token for non-nullness.
    id->id = id;
    id->p_first = &f->blocks[n][0];
    id->p_first->i_dts = head;
    id->p_first->i_pts = VLC_TICK_INVALID;
    if (key != VLC_TICK_INVALID)
    {
        id->p_first->p_next = &f->blocks[n][1];
        f->blocks[n][1].i_dts = key;
        f->blocks[n][1].i_flags = BLOCK_FLAG_TYPE_I;
    }
}

static unsigned failures, checks;
static void check(const char *name, struct fixture *f, vlc_tick_t expected)
{
    vlc_tick_t actual = OutputStartDate(&f->sys);
    bool passed = actual == expected;
    ++checks;
    failures += !passed;
    printf("{\"name\":\"%s\",\"actual\":%lld,\"expected\":%lld,\"passed\":%s}\n",
           name, (long long)actual, (long long)expected, passed ? "true" : "false");
}

static void check_block(const char *name, int cat, vlc_tick_t dts, vlc_tick_t pts,
                        vlc_tick_t length, vlc_tick_t start, bool keep,
                        vlc_tick_t expected_dts, vlc_tick_t expected_pts,
                        vlc_tick_t expected_length)
{
    uint8_t payload[] = {1, 2, 3};
    sout_stream_id_sys_t id = {0};
    id.fmt.i_cat = cat;
    block_t block = {.i_dts=dts, .i_pts=pts, .i_length=length,
        .p_buffer=payload, .i_buffer=sizeof(payload), .i_flags=BLOCK_FLAG_DISCONTINUITY};
    const bool actual = BlockAtStart(&id, &block, start);
    const bool passed = actual == keep && block.i_dts == expected_dts
        && block.i_pts == expected_pts && block.i_length == expected_length
        && block.p_buffer == payload && block.i_buffer == sizeof(payload)
        && block.i_flags == BLOCK_FLAG_DISCONTINUITY && payload[0] == 1
        && payload[1] == 2 && payload[2] == 3;
    ++checks; failures += !passed;
    printf("{\"name\":\"%s\",\"kept\":%s,\"dts\":%lld,\"pts\":%lld,\"length\":%lld,\"passed\":%s}\n",
        name, actual ? "true" : "false", (long long)block.i_dts,
        (long long)block.i_pts, (long long)block.i_length, passed ? "true" : "false");
}

int main(void)
{
    struct fixture f = {0};
    add(&f, AUDIO_ES, 1173167, VLC_TICK_INVALID);
    add(&f, VIDEO_ES, 1200001, 9920001);
    add(&f, SPU_ES, 19999001, VLC_TICK_INVALID);
    check("futureSubtitleDoesNotDelayAV", &f, 9920001);

    f = (struct fixture){0};
    add(&f, AUDIO_ES, 7000, VLC_TICK_INVALID);
    add(&f, SPU_ES, 9000000, VLC_TICK_INVALID);
    check("futureSubtitleDoesNotDelayAudioOnly", &f, 7000);

    f = (struct fixture){0};
    add(&f, AUDIO_ES, 7000, VLC_TICK_INVALID);
    add(&f, VIDEO_ES, 1000, 4000);
    add(&f, SPU_ES, 2000, VLC_TICK_INVALID);
    check("laterAudioKeepsAVAlignment", &f, 7000);

    f = (struct fixture){0};
    add(&f, VIDEO_ES, 1000, 4000);
    add(&f, VIDEO_ES, 2000, 8000);
    add(&f, SPU_ES, 9000, VLC_TICK_INVALID);
    check("multipleVideoStreamsAlignAtLatestKey", &f, 8000);

    f = (struct fixture){0};
    add(&f, SPU_ES, 3000, VLC_TICK_INVALID);
    add(&f, SPU_ES, 8000, VLC_TICK_INVALID);
    check("subtitleOnlyRetainsLegacyStart", &f, 8000);

    f = (struct fixture){0};
    add(&f, AUDIO_ES, 5000, VLC_TICK_INVALID);
    check("audioOnlyKeepsFirstPacket", &f, 5000);

    f = (struct fixture){0};
    add(&f, VIDEO_ES, VLC_TICK_INVALID, VLC_TICK_INVALID);
    f.blocks[0][0].i_pts = 6000;
    check("missingDtsUsesPts", &f, 6000);

    f = (struct fixture){0};
    add(&f, VIDEO_ES, 3000, VLC_TICK_INVALID);
    check("noKeyKeepsHeadFallback", &f, 3000);

    f = (struct fixture){0};
    add(&f, VIDEO_ES, 3000, 5000);
    add(&f, SPU_ES, 9000, VLC_TICK_INVALID);
    f.streams[1].id = NULL;
    check("rejectedStreamCannotSetStart", &f, 5000);

    f = (struct fixture){0};
    add(&f, VIDEO_ES, 3000, 5000);
    add(&f, SPU_ES, 9000, VLC_TICK_INVALID);
    f.streams[1].p_first = NULL;
    check("emptyStreamCannotSetStart", &f, 5000);

    f = (struct fixture){0};
    check("emptyRecordingKeepsZero", &f, 0);
    check_block("overlappingSubtitleKeepsItsRemainingInterval", SPU_ES,
                5000, 5000, 11000, 10000, true, 10000, 10000, 6000);
    check_block("futureSubtitleIsUnchanged", SPU_ES,
                12000, 12000, 2000, 10000, true, 12000, 12000, 2000);
    check_block("subtitleEndingAtStartIsExpired", SPU_ES,
                5000, 5000, 5000, 10000, false, 5000, 5000, 5000);
    check_block("unknownSubtitleDurationIsNotInvented", SPU_ES,
                5000, 5000, 0, 10000, false, 5000, 5000, 0);
    check_block("audioBeforeStartIsNotRetimed", AUDIO_ES,
                5000, 5000, 11000, 10000, false, 5000, 5000, 11000);
    check_block("videoBeforeStartIsNotRetimed", VIDEO_ES,
                5000, 5000, 11000, 10000, false, 5000, 5000, 11000);
    check_block("overlapCanUsePresentationTimestamp", SPU_ES,
                VLC_TICK_INVALID, 5000, 11000, 10000, true, 10000, 10000, 6000);
    check_block("overlapPreservesCompositionOffset", SPU_ES,
                5000, 5500, 11000, 10000, true, 10000, 10500, 6000);
    check_block("invalidSubtitleTimeIsNotInvented", SPU_ES,
                VLC_TICK_INVALID, VLC_TICK_INVALID, 11000, 10000, false,
                VLC_TICK_INVALID, VLC_TICK_INVALID, 11000);
    check_block("overflowingPresentationTimeIsRejected", SPU_ES,
                1, INT64_MAX - 4, 11000, 10000, false, 1, INT64_MAX - 4, 11000);
    check_block("unusableTimestampSpanIsRejected", SPU_ES,
                INT64_MIN + 1, INT64_MIN + 1, 11000, 10000, false,
                INT64_MIN + 1, INT64_MIN + 1, 11000);
    printf("{\"checks\":%u,\"failures\":%u}\n", checks, failures);
    return failures ? 1 : 0;
}
