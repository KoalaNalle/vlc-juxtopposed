// SPDX-License-Identifier: LGPL-2.1-or-later
// Owned loopback HTTP integration driver. Never distribute this executable.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include <vlc_interface.h>
#include <vlc_playlist.h>
#include <vlc_player.h>
#include <vlc_input_item.h>
#include <vlc_configuration.h>
#include "../../../lib/libvlc_internal.h"

static char *service_state(libvlc_int_t *root)
{
    return var_Type(root, "http-service-state") == VLC_VAR_STRING
        ? var_GetString(root, "http-service-state") : strdup("unavailable");
}
int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc != 4) return 2; // port, owned media URI, owned ready-file path
    char *end = NULL;
    unsigned long port = strtoul(argv[1], &end, 10);
    if (!end || *end || port < 1024 || port > 65535) return 2;
    const char *options[] = {"--ignore-config", "--no-media-library", "--no-plugins-cache",
        "--no-save-recentplay", "--no-restore-playback-states", "--no-auto-preparse",
        "--no-metadata-network-access", "--no-one-instance", "--extraintf=", "--control=",
        "--aout=mmdevice", "--gain=0", "--quiet", "--hotkeys-y-wheel-mode=2", "--hotkeys-x-wheel-mode=-1"};
    libvlc_instance_t *instance = libvlc_new(ARRAY_SIZE(options), options);
    if (!instance) return 3;
    libvlc_int_t *root = instance->p_libvlc_int;
    var_Create(root, "http-host", VLC_VAR_STRING); var_SetString(root, "http-host", "127.0.0.1");
    var_Create(root, "http-port", VLC_VAR_INTEGER); var_SetInteger(root, "http-port", port);
    var_Create(root, "http-password", VLC_VAR_STRING);
    const char *password = getenv("CONTROLS_FIXTURE_HTTP_PASSWORD");
    var_SetString(root, "http-password", password ? password : "");
    intf_thread_t *probe = vlc_object_create(VLC_OBJECT(root), sizeof(*probe));
    if (!probe) return 3;
    vlc_player_t *player = vlc_playlist_GetPlayer(vlc_intf_GetMainPlaylist(probe));
    vlc_object_delete(probe);
    libvlc_InternalAddIntf(root, "http");
    char *state = NULL;
    vlc_tick_t deadline = vlc_tick_now() + VLC_TICK_FROM_SEC(10);
    do {
        free(state); state = service_state(root);
        if (strcmp(state, "starting")) break;
        vlc_tick_sleep(VLC_TICK_FROM_MS(20));
    } while (vlc_tick_now() < deadline);
    if (!strcmp(state, "active"))
    {
        input_item_t *media = input_item_New(argv[2], "Owned silent remote fixture");
        vlc_player_Lock(player);
        int result = vlc_player_SetCurrentMedia(player, media);
        if (result == VLC_SUCCESS) result = vlc_player_Start(player);
        vlc_player_Unlock(player);
        input_item_Release(media);
        if (result != VLC_SUCCESS) return 4;
        bool playing = false, primed = false;
        deadline = vlc_tick_now() + VLC_TICK_FROM_SEC(8);
        do {
            vlc_player_Lock(player);
            playing = vlc_player_GetState(player) == VLC_PLAYER_STATE_PLAYING;
            primed = playing && vlc_player_CanSeek(player)
                && vlc_player_GetTime(player) >= VLC_TICK_FROM_MS(200);
            vlc_player_Unlock(player);
            if (primed && vlc_player_aout_GetVolume(player) >= 0.f) break;
            vlc_tick_sleep(VLC_TICK_FROM_MS(20));
        } while (vlc_tick_now() < deadline);
        if (!primed || vlc_player_aout_SetVolume(player, 0.5f) != VLC_SUCCESS) return 4;
    }
    FILE *ready = fopen(argv[3], "wb");
    if (!ready) return 5;
    fprintf(ready, "%s\n", state); fclose(ready); free(state);
    char line[128];
    while (fgets(line, sizeof(line), stdin))
    {
        if (!strcmp(line, "QUIT\n")) break;
        char axis; int mode, reverse;
        if (sscanf(line, "MAP %c %d %d", &axis, &mode, &reverse) == 3
            && (axis == 'x' || axis == 'y') && (mode == -1 || mode == 0 || mode == 2 || mode == 3)
            && (reverse == 0 || reverse == 1))
        {
            const char *name = axis == 'x' ? "http-wheel-x-mode" : "http-wheel-y-mode";
            var_Create(root, name, VLC_VAR_INTEGER); var_SetInteger(root, name, mode);
            var_Create(root, "http-wheel-reverse", VLC_VAR_BOOL);
            var_SetBool(root, "http-wheel-reverse", reverse);
            puts("OK");
        }
        else if (!strcmp(line, "LINK\n"))
            printf("LINK %d\n", var_GetBool(root, "http-wheel-linked"));
        else if (sscanf(line, "LINK %d", &mode) == 1 && (mode == 0 || mode == 1))
        {
            var_SetBool(root, "http-wheel-linked", mode != 0);
            puts("OK");
        }
        else if (sscanf(line, "SAVED %c %d %d", &axis, &mode, &reverse) == 3
            && (axis == 'x' || axis == 'y') && (mode == -1 || mode == 0 || mode == 2 || mode == 3)
            && (reverse == 0 || reverse == 1))
        {
            config_PutInt(axis == 'x' ? "hotkeys-x-wheel-mode" : "hotkeys-y-wheel-mode", mode);
            config_PutInt("hotkeys-wheel-reverse", reverse);
            puts("OK");
        }
        else if (!strncmp(line, "SEEK ", 5))
        {
            long seconds = strtol(line + 5, &end, 10);
            if (seconds < 0 || seconds > 50 || (*end && *end != '\n')) { puts("ERROR"); continue; }
            vlc_player_Lock(player); vlc_player_SetTime(player, VLC_TICK_FROM_SEC(seconds)); vlc_player_Unlock(player);
            puts("OK");
        }
        else if (!strcmp(line, "TIME\n"))
        {
            vlc_player_Lock(player);
            printf("TIME %lld\n", (long long)vlc_player_GetTime(player));
            vlc_player_Unlock(player);
        }
        else puts("ERROR");
    }
    libvlc_release(instance);
    puts("CLOSED");
    return 0;
}
