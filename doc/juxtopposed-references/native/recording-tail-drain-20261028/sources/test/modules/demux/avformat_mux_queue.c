// SPDX-License-Identifier: LGPL-2.1-or-later
// Exercise the production queue with real Core FIFO/frame ownership, no FFmpeg.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include <vlc_block.h>
#include <vlc_es.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../../modules/demux/avformat/avformat_mux_queue.h"

enum { MAX_STREAMS = 4, MAX_BLOCKS = 32, MAX_BYTES = 640 };
struct fixture;
struct tracked_block
{
    block_t block; /* First: the custom release callback can recover its owner. */
    struct fixture *owner;
    unsigned tag, index, releases, writes;
    int category;
    size_t bytes;
    vlc_tick_t dts, pts, length;
    uint32_t flags;
    uint8_t payload[MAX_BYTES];
};

struct fixture
{
    struct avformat_mux_queue queue;
    struct avformat_mux_stream *streams[MAX_STREAMS];
    block_fifo_t *fifos[MAX_STREAMS];
    unsigned indices[MAX_STREAMS];
    int categories[MAX_STREAMS];
    struct tracked_block blocks[MAX_BLOCKS];
    unsigned block_count, header_calls, write_calls, trailer_calls;
    unsigned written[MAX_BLOCKS], fail_write_at;
    bool fail_header, fail_trailer;
};

static unsigned checks, failures, cases;
static const char *case_name;
static unsigned case_checks, case_failures;

static void check(bool condition, const char *expression, unsigned line)
{
    ++checks;
    if (!condition)
    {
        ++failures;
        printf("{\"case\":\"%s\",\"line\":%u,\"check\":\"%s\",\"passed\":false}\n",
               case_name, line, expression);
    }
}
#define CHECK(expression) check((expression), #expression, __LINE__)

static void begin_case(const char *name)
{
    case_name = name;
    case_checks = checks;
    case_failures = failures;
    ++cases;
}

static void end_case(void)
{
    printf("{\"case\":\"%s\",\"checks\":%u,\"failures\":%u,\"passed\":%s}\n",
           case_name, checks - case_checks, failures - case_failures,
           failures == case_failures ? "true" : "false");
}

static void release_block(block_t *block)
{
    struct tracked_block *tracked = (struct tracked_block *)block;
    CHECK(tracked->owner != NULL);
    ++tracked->releases;
    CHECK(tracked->releases == 1);
    /* Storage remains in the fixture so a duplicate release is observable. */
}
static const struct vlc_block_callbacks callbacks = { .free = release_block };

static void init(struct fixture *f)
{
    *f = (struct fixture){0};
    avformatMuxQueueInit(&f->queue);
    CHECK(f->queue.head == NULL && f->queue.tail == &f->queue.head);
    CHECK(f->queue.header_pending && !f->queue.failed);
}

static void add_stream(struct fixture *f, unsigned slot, unsigned index, int cat)
{
    assert(slot < MAX_STREAMS && f->fifos[slot] == NULL);
    f->fifos[slot] = block_FifoNew();
    assert(f->fifos[slot] != NULL);
    f->streams[slot] = avformatMuxStreamNew(f->fifos[slot], index, cat, malloc);
    assert(f->streams[slot] != NULL);
    f->indices[slot] = index;
    f->categories[slot] = cat;
    avformatMuxQueueAdd(&f->queue, f->streams[slot]);
    CHECK(f->streams[slot]->live_fifo == f->fifos[slot]);
    CHECK(f->streams[slot]->committed == NULL);
}

static unsigned put(struct fixture *f, unsigned slot, vlc_tick_t dts, size_t bytes)
{
    assert(slot < MAX_STREAMS && f->fifos[slot] != NULL);
    assert(f->block_count < MAX_BLOCKS && bytes <= MAX_BYTES);
    struct tracked_block *b = &f->blocks[f->block_count++];
    b->owner = f;
    b->tag = f->block_count;
    b->index = f->indices[slot];
    b->category = f->categories[slot];
    b->bytes = bytes;
    b->dts = dts;
    b->pts = dts == VLC_TICK_INVALID ? VLC_TICK_0 + 12345 : dts + 1;
    b->length = 1000 + b->tag;
    b->flags = b->tag % 2 ? BLOCK_FLAG_DISCONTINUITY : BLOCK_FLAG_TYPE_I;
    for (size_t i = 0; i < bytes; ++i)
        b->payload[i] = (uint8_t)(b->tag * 29 + i);
    block_Init(&b->block, &callbacks, b->payload, bytes);
    b->block.i_dts = b->dts;
    b->block.i_pts = b->pts;
    b->block.i_length = b->length;
    b->block.i_flags = b->flags;
    block_FifoPut(f->fifos[slot], &b->block);
    return b->tag;
}

static int header(void *opaque)
{
    struct fixture *f = opaque;
    ++f->header_calls;
    CHECK(f->queue.header_pending); /* Must stay true during header I/O. */
    CHECK(f->header_calls == 1);
    CHECK(f->write_calls == 0 && f->trailer_calls == 0);
    return f->fail_header ? VLC_EGENERIC : VLC_SUCCESS;
}

static int write_block(void *opaque, unsigned index, int category,
                       const block_t *block)
{
    struct fixture *f = opaque;
    const struct tracked_block *b = (const struct tracked_block *)block;
    CHECK(!f->queue.header_pending && f->header_calls == 1);
    CHECK(f->trailer_calls == 0);
    CHECK(b->owner == f && b->releases == 0 && b->writes == 0);
    CHECK(index == b->index && category == b->category);
    CHECK(block->p_next == NULL);
    CHECK(block->p_buffer == b->payload && block->i_buffer == b->bytes);
    CHECK(block->i_dts == b->dts && block->i_pts == b->pts);
    CHECK(block->i_length == b->length && block->i_flags == b->flags);
    bool payload_unchanged = true;
    for (size_t i = 0; i < b->bytes; ++i)
        payload_unchanged &= block->p_buffer[i] == (uint8_t)(b->tag * 29 + i);
    CHECK(payload_unchanged);
    assert(f->write_calls < MAX_BLOCKS);
    f->written[f->write_calls++] = b->tag;
    ++f->blocks[b->tag - 1].writes;
    return f->fail_write_at == f->write_calls ? VLC_EGENERIC : VLC_SUCCESS;
}

static int trailer(void *opaque)
{
    struct fixture *f = opaque;
    ++f->trailer_calls;
    CHECK(!f->queue.header_pending && !f->queue.failed);
    CHECK(f->header_calls == 1 && f->trailer_calls == 1);
    for (const struct avformat_mux_stream *s = f->queue.head; s; s = s->next)
        CHECK(s->live_fifo == NULL && s->committed == NULL);
    return f->fail_trailer ? VLC_EGENERIC : VLC_SUCCESS;
}

static void retire(struct fixture *f, unsigned slot)
{
    block_fifo_t *fifo = f->fifos[slot];
    assert(fifo != NULL);
    avformatMuxStreamRetire(f->streams[slot]);
    CHECK(f->streams[slot]->live_fifo == NULL);
    CHECK(block_FifoCount(fifo) == 0);
    block_FifoRelease(fifo); /* Core destroys it immediately after Del returns. */
    f->fifos[slot] = NULL;
}

static void written_prefix(struct fixture *f, const unsigned *expected, size_t count)
{
    CHECK(f->write_calls <= count);
    for (unsigned i = 0; i < f->write_calls && i < count; ++i)
        CHECK(f->written[i] == expected[i]);
}

static void clear(struct fixture *f)
{
    avformatMuxQueueClear(&f->queue);
    CHECK(f->queue.head == NULL && f->queue.tail == &f->queue.head);
    CHECK(f->queue.header_pending && !f->queue.failed);
    avformatMuxQueueClear(&f->queue); /* Repeated cleanup owns nothing twice. */
    for (unsigned i = 0; i < MAX_STREAMS; ++i)
        if (f->fifos[i] != NULL)
        {
            block_FifoRelease(f->fifos[i]);
            f->fifos[i] = NULL;
        }
    for (unsigned i = 0; i < f->block_count; ++i)
    {
        CHECK(f->blocks[i].releases == 1);
        CHECK(f->blocks[i].writes <= 1);
    }
}

static void deletion_permutation(const unsigned order[4])
{
    char name[80];
    snprintf(name, sizeof(name), "all-track-delete-order-%u%u%u%u",
             order[0], order[1], order[2], order[3]);
    begin_case(name);
    struct fixture f;
    init(&f);
    add_stream(&f, 0, 0, AUDIO_ES);
    add_stream(&f, 1, 1, VIDEO_ES);
    add_stream(&f, 2, 2, SPU_ES);
    add_stream(&f, 3, 3, SPU_ES);
    put(&f, 0, 10, 21); put(&f, 0, 30, 22); put(&f, 0, 60, 619);
    put(&f, 1, 20, 24); put(&f, 1, 40, 25); put(&f, 1, 59, 26);
    put(&f, 2, 15, 27); put(&f, 2, 60, 28);
    put(&f, 3, 25, 29); put(&f, 3, 65, 30);
    const unsigned expected[] = {1, 7, 4, 9, 2, 5, 6, 3, 8, 10};
    CHECK(avformatMuxRun(&f.queue, header, write_block, &f) == VLC_SUCCESS);
    CHECK(f.write_calls == 7); /* The now-empty live video still gates the tail. */
    CHECK(f.blocks[2].releases == 0 && block_FifoCount(f.fifos[0]) == 1);
    written_prefix(&f, expected, ARRAY_SIZE(expected));
    for (unsigned i = 0; i < 4; ++i)
    {
        retire(&f, order[i]);
        CHECK(avformatMuxRun(&f.queue, header, write_block, &f) == VLC_SUCCESS);
        written_prefix(&f, expected, ARRAY_SIZE(expected));
    }
    CHECK(f.write_calls == ARRAY_SIZE(expected));
    CHECK(avformatMuxFinalize(&f.queue, write_block, trailer, &f) == VLC_SUCCESS);
    CHECK(f.header_calls == 1 && f.trailer_calls == 1);
    clear(&f);
    end_case();
}

static void live_gaps_and_sparse_subtitles(void)
{
    begin_case("live-gaps-never-predequeue-active-packets");
    struct fixture f;
    init(&f);
    add_stream(&f, 0, 0, AUDIO_ES);
    add_stream(&f, 1, 1, VIDEO_ES);
    add_stream(&f, 2, 2, SPU_ES);
    put(&f, 0, 10, 10); put(&f, 2, 5, 10);
    CHECK(avformatMuxRun(&f.queue, header, write_block, &f) == VLC_SUCCESS);
    CHECK(f.write_calls == 0 && block_FifoCount(f.fifos[0]) == 1);
    CHECK(block_FifoCount(f.fifos[2]) == 1);
    put(&f, 1, 20, 10);
    CHECK(avformatMuxRun(&f.queue, header, write_block, &f) == VLC_SUCCESS);
    const unsigned expected[] = {2, 1, 3};
    CHECK(f.write_calls == 2 && block_FifoCount(f.fifos[1]) == 1);
    CHECK(f.blocks[2].releases == 0);
    retire(&f, 0);
    CHECK(avformatMuxRun(&f.queue, header, write_block, &f) == VLC_SUCCESS);
    CHECK(f.write_calls == 3); /* Empty SPU does not gate the remaining video. */
    written_prefix(&f, expected, ARRAY_SIZE(expected));
    retire(&f, 1); retire(&f, 2);
    CHECK(avformatMuxFinalize(&f.queue, write_block, trailer, &f) == VLC_SUCCESS);
    clear(&f);
    end_case();
}

static void flush_boundaries(void)
{
    begin_case("flush-before-delete-drops-only-live-queued-data");
    struct fixture f;
    init(&f); add_stream(&f, 0, 0, AUDIO_ES);
    put(&f, 0, 10, 10); put(&f, 0, 20, 10);
    block_FifoEmpty(f.fifos[0]);
    CHECK(f.blocks[0].releases == 1 && f.blocks[1].releases == 1);
    retire(&f, 0);
    CHECK(avformatMuxRun(&f.queue, header, write_block, &f) == VLC_SUCCESS);
    CHECK(f.write_calls == 0);
    CHECK(avformatMuxFinalize(&f.queue, write_block, trailer, &f) == VLC_SUCCESS);
    clear(&f); end_case();

    begin_case("flush-other-live-stream-preserves-committed-deleted-tail");
    init(&f); add_stream(&f, 0, 0, AUDIO_ES); add_stream(&f, 1, 1, VIDEO_ES);
    put(&f, 0, 60, 619); put(&f, 1, 50, 10);
    retire(&f, 0);
    block_FifoEmpty(f.fifos[1]);
    CHECK(f.blocks[1].releases == 1 && f.blocks[0].releases == 0);
    CHECK(avformatMuxRun(&f.queue, header, write_block, &f) == VLC_SUCCESS);
    CHECK(f.write_calls == 0); /* The still-live empty video gates commitment. */
    put(&f, 1, 55, 10);
    CHECK(avformatMuxRun(&f.queue, header, write_block, &f) == VLC_SUCCESS);
    CHECK(f.write_calls == 1 && f.written[0] == 3 && f.blocks[0].releases == 0);
    block_FifoEmpty(f.fifos[1]); block_FifoEmpty(f.fifos[1]);
    put(&f, 1, 70, 10);
    CHECK(avformatMuxRun(&f.queue, header, write_block, &f) == VLC_SUCCESS);
    const unsigned committed[] = {3, 1, 4};
    CHECK(f.write_calls == 3);
    written_prefix(&f, committed, ARRAY_SIZE(committed));
    retire(&f, 1);
    CHECK(avformatMuxFinalize(&f.queue, write_block, trailer, &f) == VLC_SUCCESS);
    clear(&f); end_case();

    begin_case("repeated-live-flush-send-does-not-create-private-active-backlog");
    init(&f); add_stream(&f, 0, 0, AUDIO_ES);
    put(&f, 0, 10, 10); block_FifoEmpty(f.fifos[0]);
    put(&f, 0, 20, 10);
    CHECK(avformatMuxRun(&f.queue, header, write_block, &f) == VLC_SUCCESS);
    block_FifoEmpty(f.fifos[0]);
    put(&f, 0, 30, 10); block_FifoEmpty(f.fifos[0]);
    put(&f, 0, 40, 10);
    CHECK(avformatMuxRun(&f.queue, header, write_block, &f) == VLC_SUCCESS);
    const unsigned sent[] = {2, 4};
    CHECK(f.write_calls == 2);
    written_prefix(&f, sent, ARRAY_SIZE(sent));
    retire(&f, 0);
    CHECK(avformatMuxFinalize(&f.queue, write_block, trailer, &f) == VLC_SUCCESS);
    clear(&f); end_case();
}

static void startup_and_subtitle_only(void)
{
    begin_case("short-startup-delete-forces-header-once-before-final-tail");
    struct fixture f;
    init(&f); add_stream(&f, 0, 0, AUDIO_ES); add_stream(&f, 1, 1, VIDEO_ES);
    put(&f, 0, VLC_TICK_0 + 1000, 619);
    /* Core's waiting-stream Delete invokes Mux before removing that input.
     * No cached duration threshold is needed to initialize this one header. */
    CHECK(avformatMuxRun(&f.queue, header, write_block, &f) == VLC_SUCCESS);
    CHECK(f.header_calls == 1 && f.write_calls == 0);
    retire(&f, 0);
    CHECK(avformatMuxRun(&f.queue, header, write_block, &f) == VLC_SUCCESS);
    CHECK(f.write_calls == 0 && f.blocks[0].releases == 0);
    retire(&f, 1);
    CHECK(avformatMuxRun(&f.queue, header, write_block, &f) == VLC_SUCCESS);
    CHECK(f.header_calls == 1 && f.write_calls == 1 && f.blocks[0].releases == 1);
    CHECK(avformatMuxFinalize(&f.queue, write_block, trailer, &f) == VLC_SUCCESS);
    CHECK(f.trailer_calls == 1);
    clear(&f); end_case();

    begin_case("pending-header-close-never-invents-header-or-trailer");
    init(&f); add_stream(&f, 0, 0, AUDIO_ES); put(&f, 0, 60, 619);
    retire(&f, 0);
    CHECK(avformatMuxFinalize(&f.queue, write_block, trailer, &f) == VLC_SUCCESS);
    CHECK(f.header_calls == 0 && f.write_calls == 0 && f.trailer_calls == 0);
    CHECK(f.blocks[0].releases == 0);
    clear(&f); end_case();

    begin_case("clear-pending-header-leaves-core-owned-live-fifo-intact");
    init(&f); add_stream(&f, 0, 0, AUDIO_ES); put(&f, 0, 10, 10);
    CHECK(avformatMuxFinalize(&f.queue, write_block, trailer, &f) == VLC_SUCCESS);
    avformatMuxQueueClear(&f.queue);
    CHECK(block_FifoCount(f.fifos[0]) == 1 && f.blocks[0].releases == 0);
    CHECK(f.header_calls == 0 && f.write_calls == 0 && f.trailer_calls == 0);
    clear(&f); end_case();

    begin_case("subtitle-only-empty-live-stream-is-not-an-av-gate");
    init(&f); add_stream(&f, 0, 0, SPU_ES); add_stream(&f, 1, 1, SPU_ES);
    put(&f, 0, 10, 10);
    CHECK(avformatMuxRun(&f.queue, header, write_block, &f) == VLC_SUCCESS);
    put(&f, 1, 20, 10);
    CHECK(avformatMuxRun(&f.queue, header, write_block, &f) == VLC_SUCCESS);
    CHECK(f.write_calls == 2 && f.header_calls == 1);
    retire(&f, 1); retire(&f, 0);
    CHECK(avformatMuxFinalize(&f.queue, write_block, trailer, &f) == VLC_SUCCESS);
    clear(&f); end_case();

    begin_case("invalid-dts-preserved-and-equal-dts-use-registration-order");
    init(&f); add_stream(&f, 0, 40, AUDIO_ES); add_stream(&f, 1, 10, VIDEO_ES);
    add_stream(&f, 2, 30, SPU_ES);
    put(&f, 0, 20, 10); put(&f, 1, 20, 10); put(&f, 2, VLC_TICK_INVALID, 10);
    retire(&f, 1); retire(&f, 2); retire(&f, 0);
    CHECK(avformatMuxRun(&f.queue, header, write_block, &f) == VLC_SUCCESS);
    const unsigned expected[] = {3, 1, 2};
    CHECK(f.write_calls == 3);
    written_prefix(&f, expected, ARRAY_SIZE(expected));
    CHECK(avformatMuxFinalize(&f.queue, write_block, trailer, &f) == VLC_SUCCESS);
    clear(&f); end_case();
}

static unsigned failed_allocations;
static void *fail_allocate(size_t bytes)
{
    CHECK(bytes == sizeof(struct avformat_mux_stream));
    ++failed_allocations;
    return NULL;
}

static void failures_and_cleanup(void)
{
    struct fixture f;
    begin_case("allocator-failure-does-not-own-or-drain-the-live-fifo");
    init(&f);
    f.fifos[0] = block_FifoNew(); assert(f.fifos[0] != NULL);
    f.categories[0] = AUDIO_ES;
    put(&f, 0, 10, 10);
    failed_allocations = 0;
    CHECK(avformatMuxStreamNew(f.fifos[0], 0, AUDIO_ES, fail_allocate) == NULL);
    CHECK(failed_allocations == 1 && f.queue.head == NULL);
    CHECK(block_FifoCount(f.fifos[0]) == 1 && f.blocks[0].releases == 0);
    clear(&f); end_case();

    begin_case("header-error-is-sticky-and-committed-cleanup-is-exactly-once");
    init(&f); add_stream(&f, 0, 0, AUDIO_ES); put(&f, 0, 10, 10); retire(&f, 0);
    f.fail_header = true;
    CHECK(avformatMuxRun(&f.queue, header, write_block, &f) != VLC_SUCCESS);
    CHECK(f.queue.failed && f.blocks[0].releases == 0);
    CHECK(avformatMuxRun(&f.queue, header, write_block, &f) != VLC_SUCCESS);
    CHECK(avformatMuxFinalize(&f.queue, write_block, trailer, &f) != VLC_SUCCESS);
    CHECK(f.header_calls == 1 && f.write_calls == 0 && f.trailer_calls == 0);
    clear(&f); end_case();

    begin_case("committed-writer-error-releases-failed-block-and-stops");
    init(&f); add_stream(&f, 0, 0, AUDIO_ES);
    put(&f, 0, 10, 10); put(&f, 0, 20, 10); retire(&f, 0);
    f.fail_write_at = 1;
    CHECK(avformatMuxRun(&f.queue, header, write_block, &f) != VLC_SUCCESS);
    CHECK(f.blocks[0].releases == 1 && f.blocks[1].releases == 0 && f.queue.failed);
    CHECK(avformatMuxRun(&f.queue, header, write_block, &f) != VLC_SUCCESS);
    CHECK(avformatMuxFinalize(&f.queue, write_block, trailer, &f) != VLC_SUCCESS);
    CHECK(f.write_calls == 1 && f.trailer_calls == 0);
    clear(&f); end_case();

    begin_case("live-writer-error-keeps-remaining-core-fifo-ownership");
    init(&f); add_stream(&f, 0, 0, AUDIO_ES);
    put(&f, 0, 10, 10); put(&f, 0, 20, 10); f.fail_write_at = 1;
    CHECK(avformatMuxRun(&f.queue, header, write_block, &f) != VLC_SUCCESS);
    CHECK(f.blocks[0].releases == 1 && f.blocks[1].releases == 0);
    avformatMuxQueueClear(&f.queue);
    CHECK(block_FifoCount(f.fifos[0]) == 1 && f.blocks[1].releases == 0);
    clear(&f); end_case();

    begin_case("finalize-writer-error-prevents-trailer-and-cleans-remainder");
    init(&f); add_stream(&f, 0, 0, AUDIO_ES);
    CHECK(avformatMuxRun(&f.queue, header, write_block, &f) == VLC_SUCCESS);
    put(&f, 0, 10, 10); put(&f, 0, 20, 10); retire(&f, 0); f.fail_write_at = 1;
    CHECK(avformatMuxFinalize(&f.queue, write_block, trailer, &f) != VLC_SUCCESS);
    CHECK(f.write_calls == 1 && f.trailer_calls == 0 && f.blocks[0].releases == 1);
    clear(&f); end_case();

    begin_case("unselectable-committed-head-cannot-report-complete-trailer");
    init(&f); add_stream(&f, 0, 0, AUDIO_ES);
    CHECK(avformatMuxRun(&f.queue, header, write_block, &f) == VLC_SUCCESS);
    put(&f, 0, VLC_TICK_MAX, 10); retire(&f, 0);
    CHECK(avformatMuxFinalize(&f.queue, write_block, trailer, &f) != VLC_SUCCESS);
    CHECK(f.queue.failed && f.write_calls == 0 && f.trailer_calls == 0);
    CHECK(f.blocks[0].releases == 0);
    clear(&f); end_case();

    begin_case("trailer-error-is-sticky-without-double-release-or-retry");
    init(&f); add_stream(&f, 0, 0, AUDIO_ES); put(&f, 0, 10, 10); retire(&f, 0);
    CHECK(avformatMuxRun(&f.queue, header, write_block, &f) == VLC_SUCCESS);
    f.fail_trailer = true;
    CHECK(avformatMuxFinalize(&f.queue, write_block, trailer, &f) != VLC_SUCCESS);
    CHECK(f.queue.failed && f.trailer_calls == 1 && f.blocks[0].releases == 1);
    CHECK(avformatMuxFinalize(&f.queue, write_block, trailer, &f) != VLC_SUCCESS);
    CHECK(f.trailer_calls == 1);
    clear(&f); end_case();

    begin_case("initialized-finalize-with-live-stream-rejects-partial-tail");
    init(&f); add_stream(&f, 0, 0, AUDIO_ES); add_stream(&f, 1, 1, VIDEO_ES);
    put(&f, 1, 10, 10); retire(&f, 1);
    CHECK(avformatMuxRun(&f.queue, header, write_block, &f) == VLC_SUCCESS);
    CHECK(f.write_calls == 0);
    CHECK(avformatMuxFinalize(&f.queue, write_block, trailer, &f) != VLC_SUCCESS);
    CHECK(f.queue.failed && f.write_calls == 0 && f.trailer_calls == 0);
    CHECK(f.blocks[0].releases == 0);
    clear(&f); end_case();
}

int main(void)
{
    for (unsigned a = 0; a < 4; ++a)
        for (unsigned b = 0; b < 4; ++b)
            for (unsigned c = 0; c < 4; ++c)
                for (unsigned d = 0; d < 4; ++d)
                    if (a != b && a != c && a != d && b != c && b != d && c != d)
                    {
                        const unsigned order[] = {a, b, c, d};
                        deletion_permutation(order);
                    }
    live_gaps_and_sparse_subtitles();
    flush_boundaries();
    startup_and_subtitle_only();
    failures_and_cleanup();
    printf("{\"cases\":%u,\"checks\":%u,\"failures\":%u}\n", cases, checks, failures);
    return failures ? 1 : 0;
}
