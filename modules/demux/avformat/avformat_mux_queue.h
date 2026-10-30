// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifndef VLC_AVFORMAT_MUX_QUEUE_H
#define VLC_AVFORMAT_MUX_QUEUE_H

#include <vlc_common.h>
#include <vlc_block.h>
#include <vlc_es.h>
#include <stdlib.h>

/* Protected by the calling sout stream's lock. A live FIFO belongs to the
 * core: never take packets from it except for an immediate writer call. Only
 * DelStream transfers its remaining packets into a committed, owned chain.
 * The deleted core input/FIFO/format must never be borrowed afterward. */
struct avformat_mux_stream
{
    struct avformat_mux_stream *next;
    block_fifo_t *live_fifo;
    block_t *committed;
    unsigned index;
    int category;
};

struct avformat_mux_queue
{
    struct avformat_mux_stream *head;
    struct avformat_mux_stream **tail;
    bool header_pending;
    bool failed;
    bool payload_observed;
    bool failure_reported;
};

typedef int (*avformat_mux_header_cb)(void *);
/* The writer borrows the block for this call. The queue releases it exactly
 * once afterward, including on failure. The writer must not retain it. */
typedef int (*avformat_mux_write_cb)(void *, unsigned, int, const block_t *);
typedef int (*avformat_mux_trailer_cb)(void *);

static inline void
avformatMuxQueueInit(struct avformat_mux_queue *queue)
{
    queue->head = NULL;
    queue->tail = &queue->head;
    queue->header_pending = true;
    queue->failed = false;
    queue->payload_observed = false;
    queue->failure_reported = false;
}

/* Record probes add/delete streams without sending any media. Observe bytes
 * before header work, including after a sticky failure, so a real later send
 * can report that failure without making empty format probes noisy. FIFO locks
 * are released here; no reporting callback runs while a FIFO lock is held. */
static inline void
avformatMuxObservePayload(struct avformat_mux_queue *queue)
{
    if (queue->payload_observed)
        return;
    for (const struct avformat_mux_stream *stream = queue->head;
         stream != NULL; stream = stream->next)
    {
        if (stream->live_fifo != NULL)
        {
            vlc_fifo_Lock(stream->live_fifo);
            const size_t bytes = vlc_fifo_GetBytes(stream->live_fifo);
            vlc_fifo_Unlock(stream->live_fifo);
            if (bytes != 0)
            {
                queue->payload_observed = true;
                return;
            }
        }
        else
            for (const block_t *block = stream->committed;
                 block != NULL; block = block->p_next)
                if (block->i_buffer != 0)
                {
                    queue->payload_observed = true;
                    return;
                }
    }
}

/* Latch before invoking a provider: repeated Mux/Close calls get one reporting
 * attempt per mux, including when interaction is disabled or no provider exists. */
static inline bool
avformatMuxTakeFailureNotice(struct avformat_mux_queue *queue)
{
    if (!queue->failed || !queue->payload_observed || queue->failure_reported)
        return false;
    queue->failure_reported = true;
    return true;
}

static inline struct avformat_mux_stream *
avformatMuxStreamNew(block_fifo_t *fifo, unsigned index, int category,
                     void *(*allocate)(size_t))
{
    struct avformat_mux_stream *stream = allocate(sizeof(*stream));
    if (stream != NULL)
        *stream = (struct avformat_mux_stream) {
            .live_fifo = fifo, .index = index, .category = category,
        };
    return stream;
}

static inline void
avformatMuxQueueAdd(struct avformat_mux_queue *queue,
                    struct avformat_mux_stream *stream)
{
    assert(stream != NULL && stream->next == NULL);
    *queue->tail = stream;
    queue->tail = &stream->next;
}

static inline void
avformatMuxStreamRetire(struct avformat_mux_stream *stream)
{
    assert(stream->live_fifo != NULL && stream->committed == NULL);
    vlc_fifo_Lock(stream->live_fifo);
    stream->committed = vlc_fifo_DequeueAllUnlocked(stream->live_fifo);
    vlc_fifo_Unlock(stream->live_fifo);
    stream->live_fifo = NULL;
}

static inline struct avformat_mux_stream *
avformatMuxSelect(struct avformat_mux_queue *queue)
{
    struct avformat_mux_stream *selected = NULL;
    vlc_tick_t dts = VLC_TICK_MAX;
    for (struct avformat_mux_stream *stream = queue->head;
         stream != NULL; stream = stream->next)
    {
        const block_t *head;
        if (stream->live_fifo != NULL)
        {
            if (block_FifoCount(stream->live_fifo) == 0)
            {
                if (stream->category != SPU_ES)
                    return NULL; /* Preserve the live A/V readiness rule. */
                continue;
            }
            head = block_FifoShow(stream->live_fifo);
        }
        else
            head = stream->committed;

        /* Registration order breaks ties, just as the original input array.
         * Preserve original DTS values, including their invalid sentinel. */
        if (head != NULL && head->i_dts < dts)
        {
            selected = stream;
            dts = head->i_dts;
        }
    }
    return selected;
}

static inline int
avformatMuxRun(struct avformat_mux_queue *queue,
               avformat_mux_header_cb header, avformat_mux_write_cb write,
               void *opaque)
{
    avformatMuxObservePayload(queue);
    if (queue->failed)
        return VLC_EGENERIC;
    if (queue->header_pending)
    {
        assert(header != NULL);
        /* It stays pending during the callback, including its I/O flush, so
         * the access output retains the existing header flag behavior. */
        const int result = header(opaque);
        queue->header_pending = false;
        if (result != VLC_SUCCESS)
        {
            queue->failed = true;
            return result;
        }
    }

    struct avformat_mux_stream *stream;
    while ((stream = avformatMuxSelect(queue)) != NULL)
    {
        block_t *block;
        if (stream->live_fifo != NULL)
            block = block_FifoGet(stream->live_fifo);
        else
        {
            block = stream->committed;
            stream->committed = block->p_next;
        }
        block->p_next = NULL;
        const int result = write(opaque, stream->index, stream->category, block);
        block_Release(block);
        if (result != VLC_SUCCESS)
        {
            queue->failed = true;
            return result;
        }
    }
    return VLC_SUCCESS;
}

static inline int
avformatMuxFinalize(struct avformat_mux_queue *queue,
                    avformat_mux_write_cb write,
                    avformat_mux_trailer_cb trailer, void *opaque)
{
    avformatMuxObservePayload(queue);
    if (queue->failed)
        return VLC_EGENERIC;
    if (queue->header_pending)
        return VLC_SUCCESS; /* Preserve Close without an initialized header. */
    for (struct avformat_mux_stream *stream = queue->head;
         stream != NULL; stream = stream->next)
        if (stream->live_fifo != NULL)
        {
            /* Core inputs must end before mux Close. Do not guess that an
             * empty but live FIFO is EOF or finalize a partial committed tail. */
            queue->failed = true;
            return VLC_EGENERIC;
        }
    int result = avformatMuxRun(queue, NULL, write, opaque);
    if (result == VLC_SUCCESS)
        for (struct avformat_mux_stream *stream = queue->head;
             stream != NULL; stream = stream->next)
            if (stream->committed != NULL)
            {
                /* A head that cannot be selected must not silently become
                 * a successful trailer and discarded committed output. */
                queue->failed = true;
                return VLC_EGENERIC;
            }
    if (result == VLC_SUCCESS)
        result = trailer(opaque);
    if (result != VLC_SUCCESS)
        queue->failed = true;
    return result;
}

static inline void
avformatMuxQueueClear(struct avformat_mux_queue *queue)
{
    struct avformat_mux_stream *stream = queue->head;
    while (stream != NULL)
    {
        struct avformat_mux_stream *next = stream->next;
        block_ChainRelease(stream->committed);
        /* live_fifo remains core-owned, even on an error path. */
        free(stream);
        stream = next;
    }
    avformatMuxQueueInit(queue);
}
#endif
