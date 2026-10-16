// SPDX-License-Identifier: LGPL-2.1-or-later
// Actual media-library history transaction, failure propagation and retry.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include <vlc_media_library.h>
#include <vlc_url.h>
#include "../../libvlc/test.h"
#include "../../../lib/libvlc_internal.h"

static unsigned checks, failures;
static void check(bool passed, const char *name)
{
    checks++;
    failures += !passed;
    printf("%s: %s\n", passed ? "PASS" : "FAIL", name);
    fflush(stdout);
}
static bool command(const char *expected)
{
    char buffer[64];
    return fgets(buffer, sizeof(buffer), stdin) && !strcmp(buffer, expected);
}
static libvlc_instance_t *new_instance(void)
{
    const char *args[] = {"--ignore-config", "--media-library", "--quiet",
        "--no-ml-network-discovery", "--no-metadata-network-access",
        "--no-auto-preparse", "--extraintf=", "--control="};
    return libvlc_new(ARRAY_SIZE(args), args);
}
static bool retained(vlc_medialibrary_t *ml, int64_t media_id, int64_t playlist_id)
{
    vlc_ml_media_t *media = vlc_ml_get_media(ml, media_id);
    vlc_ml_playlist_t *playlist = vlc_ml_get_playlist(ml, playlist_id);
    char *speed = NULL;
    int code = vlc_ml_media_get_playback_state(ml, media_id,
                                               VLC_ML_PLAYBACK_STATE_SPEED, &speed);
    bool passed = media && media->b_is_favorite && playlist
               && playlist->i_nb_media == 2 && code == VLC_SUCCESS
               && speed && !strcmp(speed, "1.25");
    free(speed);
    if (media) vlc_ml_release(media);
    if (playlist) vlc_ml_release(playlist);
    return passed;
}
int main(void)
{
    if (getenv("VLC_PLUGIN_PATH") == NULL) test_init();
    const char *fixture = getenv("VLC_HISTORY_FIXTURE");
    if (!fixture || !getenv("VLC_USERDATA_PATH")) return 2;
    libvlc_instance_t *vlc = new_instance();
    if (!vlc) return 2;
    vlc_medialibrary_t *ml = vlc_ml_instance_get(vlc->p_libvlc_int);
    if (!ml || vlc_ml_count_history(ml, NULL, VLC_ML_HISTORY_TYPE_GLOBAL) != 0) return 2;
    char *uri = vlc_path2uri(fixture, NULL);
    vlc_ml_media_t *local = uri ? vlc_ml_new_external_media(ml, uri) : NULL;
    free(uri);
    // Insert an owned URL record only. This test never opens or fetches it.
    vlc_ml_media_t *network = vlc_ml_new_stream(ml, "http://127.0.0.1:9/owned-history-test");
    vlc_ml_playlist_t *playlist = vlc_ml_playlist_create(ml, "Retained history fixture");
    if (!local || !network || !playlist) return 2;
    const int64_t local_id = local->i_id, network_id = network->i_id;
    const int64_t playlist_id = playlist->i_id;
    vlc_ml_release(local);vlc_ml_release(network);vlc_ml_release(playlist);
    const int64_t ids[] = {local_id, network_id};
    if (vlc_ml_media_set_type(ml, local_id, VLC_ML_MEDIA_TYPE_AUDIO) != VLC_SUCCESS
     || vlc_ml_media_set_type(ml, network_id, VLC_ML_MEDIA_TYPE_VIDEO) != VLC_SUCCESS
     || vlc_ml_playlist_append(ml, playlist_id, ids, ARRAY_SIZE(ids)) != VLC_SUCCESS
     || vlc_ml_media_set_favorite(ml, local_id, true) != VLC_SUCCESS
     || vlc_ml_media_set_playback_state(ml, local_id, VLC_ML_PLAYBACK_STATE_SPEED, "1.25") != VLC_SUCCESS
     || vlc_ml_media_update_progress(ml, local_id, .375) != VLC_SUCCESS
     || vlc_ml_media_update_progress(ml, network_id, .625) != VLC_SUCCESS
     || vlc_ml_count_history(ml, NULL, VLC_ML_HISTORY_TYPE_GLOBAL) != 2)
        return 2;
    printf("READY local=%"PRId64" network=%"PRId64"\n", local_id, network_id);
    fflush(stdout);
    if (!command("blocked\n")) return 2;
    const int blocked = vlc_ml_clear_history(ml, VLC_ML_HISTORY_TYPE_GLOBAL);
    check(blocked != VLC_SUCCESS, "database failure reaches native caller");
    local = vlc_ml_get_media(ml, local_id);network = vlc_ml_get_media(ml, network_id);
    check(local && network && local->f_progress == .375 && network->f_progress == .625
          && vlc_ml_count_history(ml, NULL, VLC_ML_HISTORY_TYPE_GLOBAL) == 2
          && retained(ml, local_id, playlist_id), "failed transaction leaves history and saved data unchanged");
    if (local) vlc_ml_release(local);
    if (network) vlc_ml_release(network);
    printf("RETRY\n");fflush(stdout);
    if (!command("retry\n")) return 2;
    check(vlc_ml_clear_history(ml, VLC_ML_HISTORY_TYPE_GLOBAL) == VLC_SUCCESS,
          "clear succeeds after database failure is removed");
    local = vlc_ml_get_media(ml, local_id);network = vlc_ml_get_media(ml, network_id);
    check(local && network && local->f_progress <= 0 && network->f_progress <= 0
          && vlc_ml_count_history(ml, NULL, VLC_ML_HISTORY_TYPE_GLOBAL) == 0,
          "successful clear removes both history scopes and playback progress");
    if (local) vlc_ml_release(local);
    if (network) vlc_ml_release(network);
    check(retained(ml, local_id, playlist_id), "favorites, named playlist and explicit preferences survive clear");
    check(vlc_ml_clear_history(ml, VLC_ML_HISTORY_TYPE_GLOBAL) == VLC_SUCCESS,
          "clearing an empty history is repeatable");
    libvlc_release(vlc);
    vlc = new_instance();
    ml = vlc ? vlc_ml_instance_get(vlc->p_libvlc_int) : NULL;
    check(ml && vlc_ml_count_history(ml, NULL, VLC_ML_HISTORY_TYPE_GLOBAL) == 0
          && retained(ml, local_id, playlist_id), "cleared history and retained data survive native restart");
    if (vlc) libvlc_release(vlc);
    printf("RESULT: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
