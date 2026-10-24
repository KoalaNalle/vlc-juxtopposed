/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Exercise the real module's Stop, main loop, timers and block queue. */
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#undef NDEBUG
#define INITGUID
#define COBJMACROS
#define CONST_VTABLE
#include <windows.h>
static DWORD WINAPI observed_wait(DWORD, const HANDLE *, BOOL, DWORD);
#define WaitForMultipleObjects observed_wait
#include "../../../modules/audio_output/mmdevice.c"
#undef WaitForMultipleObjects
#include <vlc/vlc.h>
#include "../../../lib/libvlc_internal.h"
#include <stdio.h>

enum probe_mode { TIMER, PLAY, BUFFER_WAIT, IDLE };
struct stop_probe {
    audio_output_t *aout;
    enum probe_mode mode;
    HANDLE entered, release_worker, stopping, callback;
    HANDLE observed_buffer;
    DWORD worker_id, caller_id, callback_thread;
    volatile LONG worker_active, inside_wait, overlap, callback_count;
};
static struct stop_probe *probe;
static volatile LONG released_blocks;

static DWORD WINAPI observed_wait(DWORD count, const HANDLE *events,
                                 BOOL all, DWORD timeout)
{
    struct stop_probe *p = probe;
    if (p->mode == BUFFER_WAIT)
    {
        assert(count == 2);
        p->observed_buffer = events[1];
        InterlockedExchange(&p->inside_wait, 1);
        SetEvent(p->entered);
    }
    DWORD result = WaitForMultipleObjects(count, events, all, timeout);
    InterlockedExchange(&p->inside_wait, 0);
    return result;
}

struct queued_block { block_t block; unsigned char bytes[8]; };
static void free_queued_block(block_t *block)
{
    InterlockedIncrement(&released_blocks);
    free(container_of(block, struct queued_block, block));
}
static const struct vlc_block_callbacks queued_callbacks = {
    .free = free_queued_block,
};

static void queue_blocks(struct aout_stream_owner *owner, unsigned count)
{
    InterlockedExchange(&released_blocks, 0);
    for (unsigned i = 0; i < count; ++i)
    {
        struct queued_block *queued = calloc(1, sizeof(*queued));
        assert(queued);
        block_Init(&queued->block, &queued_callbacks, queued->bytes,
                   sizeof(queued->bytes));
        aout_stream_owner_AppendBlock(owner, &queued->block, VLC_TICK_INVALID);
    }
}

static void hold_backend_call(void)
{
    InterlockedExchange(&probe->worker_active, 1);
    SetEvent(probe->entered);
    assert(WaitForSingleObject(probe->release_worker, 3000) == WAIT_OBJECT_0);
    InterlockedExchange(&probe->worker_active, 0);
}

static void fake_timer(aout_stream_t *stream)
{
    (void)stream;
    hold_backend_call();
}

static HRESULT fake_play(aout_stream_t *stream, block_t *block, vlc_tick_t date)
{
    (void)stream; (void)date;
    hold_backend_call();
    block_Release(block);
    return S_OK;
}

static void fake_stream_stop(aout_stream_t *stream)
{
    (void)stream;
    probe->callback_thread = GetCurrentThreadId();
    InterlockedExchange(&probe->overlap,
        InterlockedCompareExchange(&probe->worker_active, 0, 0) ||
        InterlockedCompareExchange(&probe->inside_wait, 0, 0));
    InterlockedIncrement(&probe->callback_count);
    /* End this owned main loop once it has acknowledged the stop. */
    ((aout_sys_t *)probe->aout->sys)->device_status = DEVICE_PENDING;
    SetEvent(probe->callback);
}

static DWORD WINAPI playback_worker(void *opaque)
{
    struct stop_probe *p = opaque;
    aout_sys_t *sys = p->aout->sys;
    p->worker_id = GetCurrentThreadId();
    EnterMTA();
    vlc_mutex_lock(&sys->lock);
    if (p->mode == IDLE)
    {
        /* The outer MMThread uses this dispatch when no session is active. */
        vlc_mutex_unlock(&sys->lock);
        SetEvent(p->entered);
        assert(WaitForSingleObject(sys->work_event, 3000) == WAIT_OBJECT_0);
        vlc_mutex_lock(&sys->lock);
        assert(MMProcessStreamStop(p->aout));
    }
    else
        MMSessionMainloop(p->aout, NULL);
    vlc_mutex_unlock(&sys->lock);
    LeaveMTA();
    return 0;
}

static DWORD WINAPI stop_worker(void *opaque)
{
    struct stop_probe *p = opaque;
    p->caller_id = GetCurrentThreadId();
    SetEvent(p->stopping);
    Stop(p->aout);
    return 0;
}

int main(void)
{
    _set_error_mode(_OUT_TO_STDERR);
    const char *args[] = { "--ignore-config", "--no-media-library",
        "--no-metadata-network-access", "--no-ml-network-discovery",
        "--no-qt-updates-notif" };
    libvlc_instance_t *instance = libvlc_new(ARRAY_SIZE(args), args);
    assert(instance);
    audio_output_t *aout = vlc_object_create(VLC_OBJECT(instance->p_libvlc_int),
                                            sizeof(*aout));
    assert(aout);
    const char *names[] = { "timer", "play", "buffer-wait", "idle" };
    unsigned failures = 0;
    for (enum probe_mode mode = TIMER; mode <= IDLE; ++mode)
    for (unsigned cycle = 0; cycle < 8; ++cycle)
    {
        aout_sys_t *sys = calloc(1, sizeof(*sys));
        assert(sys);
        aout->sys = sys;
        vlc_mutex_init(&sys->lock);
        vlc_cond_init(&sys->ready);
        sys->device_status = DEVICE_ACQUIRED;
        sys->work_event = CreateEvent(NULL, FALSE, mode == PLAY, NULL);
        sys->stream = aout_stream_owner_New(aout, sizeof(*sys->stream),
                                            ActivateDevice);
        assert(sys->work_event && sys->stream);
        sys->stream->s.stop = fake_stream_stop;
        sys->stream->s.play = fake_play;
        const unsigned queued = mode == PLAY ? 1 : 2;
        queue_blocks(sys->stream, queued);
        if (mode == TIMER)
        {
            sys->stream->timer.callback = fake_timer;
            sys->stream->timer.deadline = vlc_tick_now() - 1;
        }
        struct stop_probe p = { .aout = aout, .mode = mode };
        probe = &p;
        p.entered = CreateEvent(NULL, FALSE, FALSE, NULL);
        p.release_worker = CreateEvent(NULL, FALSE, FALSE, NULL);
        p.stopping = CreateEvent(NULL, FALSE, FALSE, NULL);
        p.callback = CreateEvent(NULL, FALSE, FALSE, NULL);
        assert(p.entered && p.release_worker && p.stopping && p.callback);
        HANDLE worker = CreateThread(NULL, 0, playback_worker, &p, 0, NULL);
        assert(worker);
        assert(WaitForSingleObject(p.entered, 3000) == WAIT_OBJECT_0);
        HANDLE stopper = CreateThread(NULL, 0, stop_worker, &p, 0, NULL);
        assert(stopper);
        assert(WaitForSingleObject(p.stopping, 3000) == WAIT_OBJECT_0);
        bool early_callback = false;
        if (mode == TIMER || mode == PLAY)
        {
            early_callback = WaitForSingleObject(p.callback, 100) == WAIT_OBJECT_0;
            SetEvent(p.release_worker);
        }
        assert(WaitForSingleObject(worker, 3000) == WAIT_OBJECT_0);
        assert(WaitForSingleObject(stopper, 3000) == WAIT_OBJECT_0);
        DWORD worker_exit = 1, caller_exit = 1, flags = 0;
        assert(GetExitCodeThread(worker, &worker_exit));
        assert(GetExitCodeThread(stopper, &caller_exit));
        bool handle_closed = mode != BUFFER_WAIT ||
            !GetHandleInformation(p.observed_buffer, &flags);
        bool same_thread = p.callback_thread == p.worker_id &&
                           p.callback_thread != p.caller_id;
        bool passed = !early_callback && !p.overlap && p.callback_count == 1 &&
            sys->stream == NULL && !sys->stop_requested &&
            worker_exit == 0 && caller_exit == 0 && handle_closed &&
            same_thread && released_blocks == (LONG)queued;
        printf("{\"mode\":\"%s\",\"cycle\":%u,\"stopOnAudioThread\":%s,"
               "\"overlap\":%s,\"callbackCount\":%ld,\"streamDetached\":%s,"
               "\"queuedBlocksReleased\":%ld,\"bufferHandleClosed\":%s,"
               "\"passed\":%s}\n", names[mode], cycle,
               same_thread ? "true" : "false", p.overlap ? "true" : "false",
               p.callback_count, sys->stream == NULL ? "true" : "false",
               released_blocks, handle_closed ? "true" : "false",
               passed ? "true" : "false");
        fflush(stdout);
        failures += !passed;
        CloseHandle(worker); CloseHandle(stopper);
        CloseHandle(p.entered); CloseHandle(p.release_worker);
        CloseHandle(p.stopping); CloseHandle(p.callback);
        CloseHandle(sys->work_event);
        free(sys);
        aout->sys = NULL;
    }
    vlc_object_delete(aout);
    libvlc_release(instance);
    return failures ? 1 : 0;
}
