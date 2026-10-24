// SPDX-License-Identifier: LGPL-2.1-or-later
// Real native player/window/hotkeys integration. Private display fixture only.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include <vlc_interface.h>
#include <vlc_playlist.h>
#include <vlc_player.h>
#include <vlc_window.h>
#include <vlc_mouse.h>
#include <vlc_actions.h>
#include <vlc_vout.h>
#include <vlc_input_item.h>
#include "../../../lib/libvlc_internal.h"

static unsigned checks;
static void check(bool passed, const char *name)
{
    printf("%s %u %s\n", passed ? "ok" : "not ok", ++checks, name);
    fflush(stdout);
    if (!passed) exit(1);
}
static enum vlc_player_state state(vlc_player_t *player)
{
    vlc_player_Lock(player);
    enum vlc_player_state result = vlc_player_GetState(player);
    vlc_player_Unlock(player);
    return result;
}
static bool wait_state(vlc_player_t *player, enum vlc_player_state expected)
{
    vlc_tick_t deadline = vlc_tick_now() + VLC_TICK_FROM_SEC(8);
    do {
        if (state(player) == expected) return true;
        vlc_tick_sleep(VLC_TICK_FROM_MS(10));
    } while (vlc_tick_now() < deadline);
    return false;
}
static vlc_window_t *wait_window(libvlc_int_t *root)
{
    vlc_tick_t deadline = vlc_tick_now() + VLC_TICK_FROM_SEC(8);
    do {
        if (var_Type(root, "controls-fixture-window") == VLC_VAR_ADDRESS
            && var_GetInteger(root, "controls-fixture-frames") > 2)
        {
            vlc_window_t *window = var_GetAddress(root, "controls-fixture-window");
            if (window) return window;
        }
        vlc_tick_sleep(VLC_TICK_FROM_MS(10));
    } while (vlc_tick_now() < deadline);
    return NULL;
}
static void click(vlc_window_t *window)
{
    vlc_window_ReportMousePressed(window, MOUSE_BUTTON_LEFT);
    vlc_window_ReportMouseReleased(window, MOUSE_BUTTON_LEFT);
}
static void double_click(vlc_window_t *window)
{
    click(window);
    vlc_window_ReportMouseDoubleClick(window, MOUSE_BUTTON_LEFT);
    vlc_window_ReportMouseReleased(window, MOUSE_BUTTON_LEFT);
}
static void settle(void)
{
    // The fixture reports native double clicks; this exceeds Windows' default
    // double-click interval. The runner passes the actual host interval below.
    unsigned ms = 700;
    const char *value = getenv("CONTROLS_FIXTURE_SETTLE_MS");
    if (value) ms = strtoul(value, NULL, 10);
    vlc_tick_sleep(VLC_TICK_FROM_MS(ms));
}
static void policy(libvlc_int_t *root, int single, int dbl)
{
    var_Create(root, "video-singleclick-action", VLC_VAR_INTEGER);
    var_SetInteger(root, "video-singleclick-action", single);
    var_Create(root, "video-doubleclick-action", VLC_VAR_INTEGER);
    var_SetInteger(root, "video-doubleclick-action", dbl);
}
static void resume(vlc_player_t *player)
{
    vlc_player_Lock(player);
    vlc_player_Resume(player);
    vlc_player_Unlock(player);
    check(wait_state(player, VLC_PLAYER_STATE_PLAYING), "resume reaches playing");
}
static int capture_action(vlc_object_t *object, const char *name,
                          vlc_value_t old, vlc_value_t value, void *opaque)
{
    (void)object; (void)name; (void)old;
    *(int *)opaque = value.i_int;
    return VLC_SUCCESS;
}
static int trace_click(vlc_object_t *object, const char *name,
                       vlc_value_t old, vlc_value_t value, void *opaque)
{
    (void)object; (void)name; (void)old; (void)opaque;
    fprintf(stderr, "owned click event: %lld\n", (long long)value.i_int);
    return VLC_SUCCESS;
}
static void wheel(libvlc_int_t *root, int *action, int key, int expected,
                  const char *name)
{
    *action = ACTIONID_NONE;
    var_SetInteger(root, "key-pressed", key);
    check(*action == expected, name);
}

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc < 3) return 2;
    const bool chain = !strcmp(argv[1], "chain");
    const bool reverse = !strcmp(argv[1], "reverse");
    const bool legacy = !strcmp(argv[1], "legacy");
    const bool binding = !strcmp(argv[1], "binding");
    const bool embedding = !strcmp(argv[1], "embedding");
    const bool disabled = !strcmp(argv[1], "legacy-disabled");
    const char *args[] = {
        "--ignore-config", "--no-media-library", "--no-plugins-cache",
        "--no-save-recentplay", "--no-restore-playback-states",
        "--no-metadata-network-access", "--no-auto-preparse", "--no-one-instance",
        "--no-video-title-show", "--verbose=2",
        "--vout=controls-fixture", "--aout=dummy",
        "--extraintf=controls-fixture-intf{marker='first:slot'}:controls-fixture-intf{marker=\"second:slot\"}",
        "--control=controls-fixture-intf{marker='third:slot'}",
        reverse || legacy ? "--hotkeys-wheel-reverse" : "--no-hotkeys-wheel-reverse",
        legacy ? "--hotkeys-y-wheel-mode=3" : "--hotkeys-y-wheel-mode=0",
        legacy ? "--hotkeys-x-wheel-mode=-1" : "--hotkeys-x-wheel-mode=2",
        binding ? "--key-play-pause=Mouse Wheel Up" : "--key-play-pause=Space",
    };
    // The chain-only negative pair must also run against the previous core,
    // which intentionally does not recognize the new wheel option.
    unsigned count = chain ? ARRAY_SIZE(args) - 4 : ARRAY_SIZE(args);
    libvlc_instance_t *instance = libvlc_new(count, args);
    check(instance != NULL, "native instance starts");
    libvlc_int_t *root = instance->p_libvlc_int;
    char *markers = var_GetString(root, "controls-fixture-markers");
    check(markers && !strcmp(markers, "first:slot|second:slot|third:slot"),
          "quoted interface chain preserves colons, order and all option blocks");
    free(markers);
    if (chain) { libvlc_release(instance); return 0; }
    // Window providers are selected by an inherited object variable, not a
    // public command-line option on this VLC revision.
    var_Create(root, "window", VLC_VAR_STRING);
    var_SetString(root, "window", "controls-fixture");
    const int hotkeys = libvlc_InternalAddIntf(root, "hotkeys");
    check(hotkeys == VLC_SUCCESS && var_GetInteger(root, "mouse-click-policy-version") == 1,
          "actual hotkeys consumer advertises click policy");
    intf_thread_t *probe = vlc_object_create(VLC_OBJECT(root), sizeof(*probe));
    check(probe != NULL, "owned interface accessor allocated");
    vlc_player_t *player = vlc_playlist_GetPlayer(vlc_intf_GetMainPlaylist(probe));
    vlc_object_delete(probe);
    int action;
    var_AddCallback(root, "key-action", capture_action, &action);
    wheel(root, &action, KEY_MOUSEWHEELUP, binding ? ACTIONID_PLAY_PAUSE
        : reverse ? ACTIONID_COMBO_VOL_FOV_DOWN : legacy ? ACTIONID_JUMP_FORWARD_EXTRASHORT
        : ACTIONID_COMBO_VOL_FOV_UP, "vertical wheel dispatches configured real action without video");
    wheel(root, &action, KEY_MOUSEWHEELDOWN, reverse ? ACTIONID_COMBO_VOL_FOV_UP
        : legacy ? ACTIONID_JUMP_BACKWARD_EXTRASHORT : ACTIONID_COMBO_VOL_FOV_DOWN,
        "opposite vertical wheel dispatches inverse action");
    wheel(root, &action, KEY_MOUSEWHEELRIGHT, legacy ? ACTIONID_NONE
        : reverse ? ACTIONID_JUMP_BACKWARD_EXTRASHORT : ACTIONID_JUMP_FORWARD_EXTRASHORT,
        "horizontal seek direction or ignore follows startup policy");
    wheel(root, &action, KEY_MOUSEWHEELLEFT, legacy ? ACTIONID_NONE
        : reverse ? ACTIONID_JUMP_FORWARD_EXTRASHORT : ACTIONID_JUMP_BACKWARD_EXTRASHORT,
        "opposite horizontal direction follows startup policy");
    var_DelCallback(root, "key-action", capture_action, &action);
    if (reverse || legacy || binding) { libvlc_release(instance); return 0; }

    if (embedding)
    {
        player = vlc_player_New(VLC_OBJECT(root), VLC_PLAYER_LOCK_NORMAL);
        check(player != NULL, "second embedding player allocated in same instance");
    }

    input_item_t *media = input_item_New(argv[2], "Owned video one");
    vlc_player_Lock(player);
    int result = vlc_player_SetCurrentMedia(player, media);
    if (result == VLC_SUCCESS) result = vlc_player_Start(player);
    vlc_player_Unlock(player);
    check(result == VLC_SUCCESS && wait_state(player, VLC_PLAYER_STATE_PLAYING),
          "actual owned video starts");
    vlc_window_t *window = wait_window(root);
    check(window != NULL, "private native window displays decoded frames");
    vlc_player_Lock(player);
    vout_thread_t *vout = vlc_player_vout_Hold(player);
    vlc_player_Unlock(player);
    check(vout != NULL, "actual vout is held");
    var_AddCallback(vout, "mouse-click-event", trace_click, NULL);
    policy(root, VLC_MOUSE_ACTION_PLAY_PAUSE, VLC_MOUSE_ACTION_FULLSCREEN);
    if (embedding || disabled)
    {
        if (disabled) var_SetString(root, "mouse-pause-policy", "disabled");
        click(window);
        if (embedding)
        {
            check(wait_state(player, VLC_PLAYER_STATE_PAUSED), "other embedding player retains release-to-pause behavior");
            click(window);
            check(wait_state(player, VLC_PLAYER_STATE_PLAYING), "other embedding player retains release-to-resume behavior");
            vlc_window_ReportMousePressed(window, MOUSE_BUTTON_LEFT);
            vlc_window_ReportMouseCancelled(window); settle();
            check(state(player) == VLC_PLAYER_STATE_PLAYING, "cancelled embedding gesture never becomes a legacy pause");
        }
        else
        {
            settle();
            check(state(player) == VLC_PLAYER_STATE_PLAYING, "legacy disabled pause policy remains effective");
            double_click(window); settle();
            check(var_GetBool(vout, "fullscreen") && state(player) == VLC_PLAYER_STATE_PLAYING,
                  "legacy pause opt-out preserves double-click fullscreen");
        }
        var_DelCallback(vout, "mouse-click-event", trace_click, NULL);
        vout_Release(vout); input_item_Release(media);
        if (embedding) vlc_player_Delete(player);
        libvlc_release(instance);
        check(true, "instance and optional embedding player shut down normally");
        printf("1..%u\n", checks);
        return 0;
    }
    click(window);
    check(wait_state(player, VLC_PLAYER_STATE_PAUSED), "single click pauses after double-click interval");
    click(window);
    check(wait_state(player, VLC_PLAYER_STATE_PLAYING), "single click resumes paused video");
    double_click(window);
    settle();
    vlc_player_Lock(player);
    fprintf(stderr, "after owned double: fullscreen=%d state=%d time=%lld length=%lld frames=%lld\n",
        var_GetBool(vout, "fullscreen"), vlc_player_GetState(player), (long long)vlc_player_GetTime(player),
        (long long)vlc_player_GetLength(player), (long long)var_GetInteger(root, "controls-fixture-frames"));
    vlc_player_Unlock(player);
    check(var_GetBool(vout, "fullscreen") && state(player) == VLC_PLAYER_STATE_PLAYING,
          "double click enters fullscreen without firing pending pause");
    double_click(window);
    settle();
    check(!var_GetBool(vout, "fullscreen") && state(player) == VLC_PLAYER_STATE_PLAYING,
          "second double click leaves fullscreen without pausing");
    vlc_window_ReportMouseMoved(window, 10, 10);
    vlc_window_ReportMousePressed(window, MOUSE_BUTTON_LEFT);
    vlc_window_ReportMouseMoved(window, 80, 80);
    vlc_window_ReportMouseReleased(window, MOUSE_BUTTON_LEFT);
    settle();
    vlc_player_Lock(player);
    fprintf(stderr, "after owned drag: state=%d time=%lld length=%lld frames=%lld\n",
        vlc_player_GetState(player), (long long)vlc_player_GetTime(player),
        (long long)vlc_player_GetLength(player), (long long)var_GetInteger(root, "controls-fixture-frames"));
    vlc_player_Unlock(player);
    check(state(player) == VLC_PLAYER_STATE_PLAYING, "drag never becomes a single click");
    vlc_window_ReportMousePressed(window, MOUSE_BUTTON_LEFT);
    vlc_window_ReportMouseCancelled(window); settle();
    check(state(player) == VLC_PLAYER_STATE_PLAYING, "lost pointer grab cancels the gesture without a click");
    click(window);
    var_SetBool(root, "intf-input-blocked", true);
    var_SetBool(root, "intf-input-blocked", false);
    settle();
    check(state(player) == VLC_PLAYER_STATE_PLAYING, "opening and closing modal cancels queued click");
    vlc_window_ReportMousePressed(window, MOUSE_BUTTON_LEFT);
    var_SetBool(root, "intf-input-blocked", true);
    var_SetBool(root, "intf-input-blocked", false);
    vlc_window_ReportMouseReleased(window, MOUSE_BUTTON_LEFT);
    settle();
    check(state(player) == VLC_PLAYER_STATE_PLAYING, "modal opened during a pressed gesture cancels its later release");
    var_SetBool(root, "intf-input-blocked", true);
    vlc_window_ReportMousePressed(window, MOUSE_BUTTON_LEFT);
    var_SetBool(root, "intf-input-blocked", false);
    vlc_window_ReportMouseReleased(window, MOUSE_BUTTON_LEFT);
    settle();
    check(state(player) == VLC_PLAYER_STATE_PLAYING, "press inside modal cannot become a video click after closing");
    click(window);
    var_SetBool(root, "intf-input-blocked", true); var_SetBool(root, "intf-input-blocked", false);
    vlc_window_ReportMouseDoubleClick(window, MOUSE_BUTTON_LEFT);
    vlc_window_ReportMouseReleased(window, MOUSE_BUTTON_LEFT); settle();
    check(!var_GetBool(vout, "fullscreen") && state(player) == VLC_PLAYER_STATE_PLAYING,
          "double click spanning a modal transition cannot change fullscreen");
    policy(root, VLC_MOUSE_ACTION_PLAY_PAUSE, VLC_MOUSE_ACTION_PLAY_PAUSE);
    vlc_window_ReportMousePressed(window, MOUSE_BUTTON_LEFT);
    vlc_window_ReportMouseCancelled(window);
    vlc_window_ReportMousePressed(window, MOUSE_BUTTON_RIGHT);
    vlc_window_ReportMouseReleased(window, MOUSE_BUTTON_RIGHT);
    vlc_window_ReportMouseDoubleClick(window, MOUSE_BUTTON_RIGHT);
    vlc_window_ReportMouseReleased(window, MOUSE_BUTTON_RIGHT); settle();
    check(state(player) == VLC_PLAYER_STATE_PLAYING, "right-button double click cannot invoke left-video play/pause");
    policy(root, VLC_MOUSE_ACTION_PLAY_PAUSE, VLC_MOUSE_ACTION_FULLSCREEN);
    var_SetBool(root, "intf-input-blocked", true);
    double_click(window);
    var_SetInteger(root, "key-action", ACTIONID_PLAY_PAUSE);
    var_SetBool(root, "intf-input-blocked", false);
    settle();
    check(!var_GetBool(vout, "fullscreen") && state(player) == VLC_PLAYER_STATE_PLAYING,
          "modal blocks double-click fullscreen and native keyboard action");
    click(window);
    vlc_window_ReportMousePressed(window, MOUSE_BUTTON_RIGHT);
    vlc_window_ReportMouseReleased(window, MOUSE_BUTTON_RIGHT);
    settle();
    check(state(player) == VLC_PLAYER_STATE_PLAYING, "context-menu gesture cancels pending pause");
    policy(root, VLC_MOUSE_ACTION_NONE, VLC_MOUSE_ACTION_NONE);
    click(window); double_click(window); settle();
    check(!var_GetBool(vout, "fullscreen") && state(player) == VLC_PLAYER_STATE_PLAYING,
          "disabled single and double click have no player effect");
    policy(root, VLC_MOUSE_ACTION_FULLSCREEN, VLC_MOUSE_ACTION_NONE);
    click(window); settle();
    check(var_GetBool(vout, "fullscreen"), "single click can select fullscreen action");
    var_SetBool(vout, "fullscreen", false);
    policy(root, VLC_MOUSE_ACTION_PLAY_PAUSE, VLC_MOUSE_ACTION_NONE);
    double_click(window); settle();
    check(state(player) == VLC_PLAYER_STATE_PLAYING, "disabled double click still suppresses pending single");
    policy(root, VLC_MOUSE_ACTION_NONE, VLC_MOUSE_ACTION_PLAY_PAUSE);
    double_click(window);
    check(wait_state(player, VLC_PLAYER_STATE_PAUSED), "double click can select play/pause action");
    resume(player);
    policy(root, VLC_MOUSE_ACTION_PLAY_PAUSE, VLC_MOUSE_ACTION_FULLSCREEN);
    click(window);
    input_item_t *next = input_item_New(argv[2], "Owned video two");
    vlc_player_Lock(player);
    result = vlc_player_SetCurrentMedia(player, next);
    vlc_player_Unlock(player);
    check(result == VLC_SUCCESS && wait_state(player, VLC_PLAYER_STATE_PLAYING), "second media starts");
    settle();
    check(state(player) == VLC_PLAYER_STATE_PLAYING, "old media click cannot pause replacement media");
    var_DelCallback(vout, "mouse-click-event", trace_click, NULL);
    vout_Release(vout);
    window = wait_window(root);
    check(window != NULL, "replacement media has a window");
    click(window);
    input_item_Release(media); input_item_Release(next);
    libvlc_release(instance);
    check(true, "instance shuts down normally with pending click timer");
    printf("1..%u\n", checks);
    return 0;
}
