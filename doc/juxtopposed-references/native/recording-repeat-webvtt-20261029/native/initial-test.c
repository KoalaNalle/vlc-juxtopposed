// SPDX-License-Identifier: LGPL-2.1-or-later
// Real-input recording notifications at input replacement and completion.
#include "../../libvlc/test.h"
#include "../../../lib/libvlc_internal.h"
#include <vlc_player.h>
#include <vlc_url.h>

struct observation
{
    vlc_mutex_t mutex;
    vlc_cond_t wait;
    unsigned media_count, record_count;
    bool records[8];
    unsigned records_at_second_media;
    bool recording_at_second_media;
};

static void
signal_observation(struct observation *obs)
{
    vlc_mutex_lock(&obs->mutex);
    vlc_cond_signal(&obs->wait);
    vlc_mutex_unlock(&obs->mutex);
}

static void
on_media(vlc_player_t *player, input_item_t *media, void *opaque)
{
    struct observation *obs = opaque;
    if (media && ++obs->media_count == 2)
    {
        obs->records_at_second_media = obs->record_count;
        obs->recording_at_second_media = vlc_player_IsRecording(player);
    }
    signal_observation(obs);
}

static void
on_record(vlc_player_t *player, bool recording, void *opaque)
{
    (void) player;
    struct observation *obs = opaque;
    if (obs->record_count < ARRAY_SIZE(obs->records))
        obs->records[obs->record_count] = recording;
    ++obs->record_count;
    signal_observation(obs);
}

static void
on_state(vlc_player_t *player, enum vlc_player_state state, void *opaque)
{
    (void) player; (void) state;
    signal_observation(opaque);
}

/* Keep every observation under the player lock, but bound failed waits so a
 * missing notification produces a useful normal test failure on Windows too. */
static bool
wait_for(vlc_player_t *player, struct observation *obs, unsigned records,
         unsigned medias, enum vlc_player_state state, bool check_state)
{
    const vlc_tick_t deadline = vlc_tick_now() + VLC_TICK_FROM_SEC(15);
    for (;;)
    {
        if (obs->record_count >= records && obs->media_count >= medias
         && (!check_state || vlc_player_GetState(player) == state))
            return true;
        if (vlc_tick_now() >= deadline)
            return false;
        /* Holding the wait mutex before releasing the player lock prevents a
         * callback from signalling between the condition check and the wait. */
        vlc_mutex_lock(&obs->mutex);
        vlc_player_Unlock(player);
        vlc_cond_timedwait(&obs->wait, &obs->mutex, deadline);
        vlc_mutex_unlock(&obs->mutex);
        vlc_player_Lock(player);
    }
}

enum ending { MANUAL_OFF, NATURAL_EOS, NEXT_SAME, NEXT_OTHER,
              REPLACE_PAUSED, STOP_PAUSED, OFF_THEN_EOS };

static bool
run_case(libvlc_instance_t *instance, const char *uri, const char *directory,
         enum ending ending, const char *name)
{
    struct observation obs = {0};
    vlc_mutex_init(&obs.mutex);
    vlc_cond_init(&obs.wait);
    static const struct vlc_player_cbs cbs = {
        .on_current_media_changed = on_media,
        .on_recording_changed = on_record,
        .on_state_changed = on_state,
    };
    vlc_player_t *player = vlc_player_New(VLC_OBJECT(instance->p_libvlc_int),
                                          VLC_PLAYER_LOCK_NORMAL);
    assert(player);
    input_item_t *first = input_item_New(uri, "record-first");
    input_item_t *other = input_item_New(uri, "record-next");
    assert(first && other);
    vlc_player_Lock(player);
    vlc_player_listener_id *listener = vlc_player_AddListener(player, &cbs, &obs);
    assert(listener);
    bool passed = false;
    const char *failure = "initial input did not pause";
    vlc_player_SetStartPaused(player, true);
    if (vlc_player_SetCurrentMedia(player, first) != VLC_SUCCESS
     || vlc_player_Start(player) != VLC_SUCCESS
     || !wait_for(player, &obs, 0, 1, VLC_PLAYER_STATE_PAUSED, true))
        goto finish;

    failure = "record-on notification missing";
    vlc_player_SetRecordingEnabled(player, true, directory);
    if (!wait_for(player, &obs, 1, 1, 0, false)
     || obs.record_count != 1 || !obs.records[0]
     || !vlc_player_IsRecording(player))
        goto finish;

    if (ending == MANUAL_OFF || ending == OFF_THEN_EOS)
    {
        failure = "manual record-off notification missing";
        vlc_player_SetRecordingEnabled(player, false, NULL);
        if (!wait_for(player, &obs, 2, 1, 0, false)
         || obs.record_count != 2 || obs.records[1]
         || vlc_player_IsRecording(player))
            goto finish;
    }

    if (ending == NEXT_SAME || ending == NEXT_OTHER)
        vlc_player_SetNextMedia(player, ending == NEXT_SAME ? first : other);

    if (ending == REPLACE_PAUSED)
        vlc_player_SetCurrentMedia(player, other);
    else if (ending == STOP_PAUSED || ending == MANUAL_OFF)
        vlc_player_Stop(player);
    else
        vlc_player_Resume(player);

    const bool has_next = ending == NEXT_SAME || ending == NEXT_OTHER
                       || ending == REPLACE_PAUSED;
    failure = "input did not complete or advance";
    if (!wait_for(player, &obs, 0, has_next ? 2 : 1,
                  VLC_PLAYER_STATE_STOPPED, !has_next))
        goto finish;

    failure = "record-off missing before next input or final stop";
    if (obs.record_count != 2 || obs.records[1]
     || vlc_player_IsRecording(player)
     || (has_next && (obs.records_at_second_media != 2
                  || obs.recording_at_second_media)))
        goto finish;

    if (has_next)
    {
        /* A cleared UI must be able to start and stop a new recording. */
        failure = "next input did not become playable";
        if (!wait_for(player, &obs, 2, 2, VLC_PLAYER_STATE_PAUSED, true))
            goto finish;
        failure = "record-on/off on replacement input failed";
        vlc_player_SetRecordingEnabled(player, true, directory);
        if (!wait_for(player, &obs, 3, 2, 0, false)
         || obs.record_count != 3 || !obs.records[2])
            goto finish;
        vlc_player_SetRecordingEnabled(player, false, NULL);
        if (!wait_for(player, &obs, 4, 2, 0, false)
         || obs.record_count != 4 || obs.records[3]
         || vlc_player_IsRecording(player))
            goto finish;
    }
    passed = true;

finish:
    printf("CASE %s %s records=%u second_media_records=%u current_recording=%d",
           name, passed ? "PASS" : "FAIL", obs.record_count,
           obs.records_at_second_media, vlc_player_IsRecording(player));
    if (!passed)
        printf(" reason=%s", failure);
    putchar('\n');
    fflush(stdout);
    vlc_player_Stop(player);
    vlc_player_RemoveListener(player, listener);
    vlc_player_Unlock(player);
    vlc_player_Delete(player);
    input_item_Release(first);
    input_item_Release(other);
    return passed;
}

int
main(int argc, char **argv)
{
#ifndef ENABLE_SOUT
    (void) argc; (void) argv;
    return 77;
#else
    if (argc != 3)
    {
        fprintf(stderr, "Use a generated short A/V fixture and owned output directory\n");
        return 77;
    }
    test_init();
    const char *args[] = {
        "--ignore-config", "--no-media-library", "--no-metadata-network-access",
        "--no-ml-network-discovery", "--vout=vdummy", "--aout=adummy",
        "--no-hw-dec", "--no-osd", "--no-sub-autodetect-file",
        "--no-video-title-show", "--verbose=1",
    };
    libvlc_instance_t *instance = libvlc_new(ARRAY_SIZE(args), args);
    assert(instance);
    char *uri = vlc_path2uri(argv[1], NULL);
    assert(uri);
    static const char *names[] = {
        "manual-off-paused", "natural-eos", "next-same-media", "next-other-media",
        "replace-paused", "stop-paused", "off-then-eos-no-duplicate",
    };
    unsigned failures = 0;
    for (unsigned i = 0; i < ARRAY_SIZE(names); ++i)
        failures += !run_case(instance, uri, argv[2], i, names[i]);
    free(uri);
    libvlc_release(instance);
    printf("RESULT cases=%zu failures=%u\n", ARRAY_SIZE(names), failures);
    return failures != 0;
#endif
}
