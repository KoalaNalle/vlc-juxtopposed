// SPDX-License-Identifier: LGPL-2.1-or-later
// Actual media-library history transaction, failure propagation and retry.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include <vlc_configuration.h>
#include <vlc_media_library.h>
#include <vlc_url.h>
#include <vlc_fs.h>
#include "../../../src/player/player.h"
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
static libvlc_instance_t *new_instance(bool library)
{
    const char *args[] = {"--ignore-config", library ? "--media-library" : "--no-media-library", "--quiet",
        "--no-ml-network-discovery", "--no-metadata-network-access",
        "--no-auto-preparse", "--extraintf=", "--control=", "--aout=dummy", "--vout=dummy"};
    return libvlc_new(ARRAY_SIZE(args), args);
}
static void disabled_startup_checks(const char *fixture)
{
    libvlc_instance_t *vlc = new_instance(false); assert(vlc);
    libvlc_int_t *root = vlc->p_libvlc_int;
    char *path;
    if (asprintf(&path, "%s/ml/ml.db", getenv("VLC_USERDATA_PATH")) < 0) abort();
    struct stat st;
    check(!vlc_ml_instance_get(root) && !var_InheritBool(root,"media-library"),
          "Library off exposes no browsing backend and retains its preference");
    check(vlc_ml_history_available(root) && vlc_stat(path,&st) != 0,
          "history capability does not create a database");
    vlc_player_t *player=vlc_player_New(VLC_OBJECT(root),VLC_PLAYER_LOCK_NORMAL); assert(player);
    vlc_player_Lock(player);
    struct vlc_player_history_policy next={false,true,strdup("")};assert(next.exclusion_pattern);
    vlc_player_SwapHistoryPolicy(player,&next);free(next.exclusion_pattern);
    char *uri=vlc_path2uri(fixture,NULL);
    input_item_t *item=input_item_New(uri,"Owned private startup");free(uri);assert(item);
    assert(vlc_player_SetCurrentMedia(player,item)==VLC_SUCCESS);input_item_Release(item);
    vlc_player_Unlock(player);vlc_player_Delete(player);
    check(vlc_stat(path,&st) != 0,"history opt-out does not create the lazy database");
    vlc_object_t *secondary_owner=vlc_object_create(root,sizeof(*secondary_owner));assert(secondary_owner);
    player=vlc_player_New(secondary_owner,VLC_PLAYER_LOCK_NORMAL);assert(player);
    vlc_player_Lock(player);uri=vlc_path2uri(fixture,NULL);
    item=input_item_New(uri,"Owned secondary input");free(uri);assert(item);
    assert(vlc_player_SetCurrentMedia(player,item)==VLC_SUCCESS);input_item_Release(item);
    player->input->position=.5;
    vlc_player_Unlock(player);vlc_player_Delete(player);vlc_object_delete(secondary_owner);
    check(vlc_stat(path,&st) != 0,"secondary playback never opens the lazy history backend");
    player=vlc_player_New(VLC_OBJECT(root),VLC_PLAYER_LOCK_NORMAL);assert(player);
    vlc_player_Lock(player);
    check(vlc_player_ClearHistory(player)==VLC_SUCCESS,
          "explicit clear initializes a fresh history backend with Library off");
    vlc_player_Unlock(player);vlc_player_Delete(player);
    vlc_medialibrary_t *ml=vlc_ml_history_get(root);
    check(ml && ml==vlc_ml_history_get(root) && !vlc_ml_instance_get(root)
          && !var_InheritBool(root,"media-library"),"lazy history backend is shared without enabling Library");
    check(vlc_ml_control(ml,VLC_ML_RESUME_BACKGROUND)!=VLC_SUCCESS
          && vlc_ml_control(ml,VLC_ML_RELOAD_FOLDER,"file:///owned-never-scanned/")!=VLC_SUCCESS
          && vlc_ml_control(ml,VLC_ML_ADD_FOLDER,"file:///owned-never-scanned/")!=VLC_SUCCESS,
          "history backend rejects discovery, reload and background resume");
    check(vlc_ml_count_folders(ml,NULL)==0,"history-only startup and clear add no Library folders");
    free(path);libvlc_release(vlc);
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
static void policy(vlc_player_t *player, bool save, bool remember, const char *pattern)
{
    struct vlc_player_history_policy next = {save, remember, strdup(pattern)};
    assert(next.exclusion_pattern);
    vlc_player_SwapHistoryPolicy(player, &next);
    free(next.exclusion_pattern);
}
static void matching_checks(void)
{
    check(!vlc_player_HistoryIsExcluded("", "file:///secret.mkv", "secret"), "empty rule excludes nothing");
    check(vlc_player_HistoryIsExcluded("secret", "file:///secret.mkv", NULL), "rule matches URI substring");
    check(!vlc_player_HistoryIsExcluded("SECRET", "file:///secret.mkv", NULL), "matching is case sensitive");
    check(vlc_player_HistoryIsExcluded("My Folder", "file:///E:/My%20Folder/a.mkv", NULL), "rule matches decoded local path");
    check(vlc_player_HistoryIsExcluded("Private*mix?", NULL, "My Private evening mix1"), "wildcards match playlist name substring");
    check(vlc_player_HistoryIsExcluded("?音?", NULL, "x🙂音éy"), "question mark advances Unicode scalars");
    check(!vlc_player_HistoryIsExcluded("a??c", NULL, "a🙂c"), "question mark does not split UTF-8 bytes");
    check(vlc_player_HistoryIsExcluded("a*b*c", NULL, "xxaaabbbccczz"), "multiple wildcard backtracking");
    check(!vlc_player_HistoryIsExcluded("a*b*z", NULL, "xxaaabbbccc"), "nonmatching wildcard retains permitted item");
    check(vlc_player_HistoryIsExcluded("*", NULL, ""), "all wildcard excludes empty name");
    check(!vlc_player_HistoryIsExcluded("[abc]", NULL, "abc"), "rule is not a regular expression");
    check(vlc_player_HistoryIsExcluded("\xff", "file:///anything", NULL), "invalid UTF-8 rule fails closed");
    check(vlc_player_HistoryIsExcluded("x", "\xff", NULL), "invalid UTF-8 identity fails closed");
    char long_pattern[4098]; memset(long_pattern,'a',sizeof(long_pattern)); long_pattern[4097]=0;
    check(vlc_player_HistoryIsExcluded(long_pattern,"file:///anything",NULL), "oversized rule fails closed");
}
static void storage_checks(libvlc_instance_t *vlc, vlc_medialibrary_t *ml, const char *fixture)
{
    char *base = vlc_path2uri(fixture,NULL);
    for (unsigned i=0;i<7;i++)
    {
        char *uri; if (asprintf(&uri,"%s#policy-%u",base,i)<0) abort();
        vlc_ml_media_t *media=vlc_ml_new_external_media(ml,uri);
        assert(media); int64_t id=media->i_id; vlc_ml_release(media);
        assert(vlc_ml_media_set_type(ml,id,VLC_ML_MEDIA_TYPE_VIDEO)==VLC_SUCCESS);
        vlc_player_t *player=vlc_player_New(VLC_OBJECT(vlc->p_libvlc_int),VLC_PLAYER_LOCK_NORMAL);
        assert(player); vlc_player_Lock(player);
        policy(player,i!=1,i!=2,i==3 ? "policy-3" : "");
        input_item_t *item=input_item_New(uri,"Policy owned fixture"); assert(item);
        assert(vlc_player_SetCurrentMedia(player,item)==VLC_SUCCESS); input_item_Release(item);
        assert(player->input); player->input->position=.625;
        if (i==4) { policy(player,false,true,""); policy(player,true,true,""); }
        if (i==5) { policy(player,true,false,""); policy(player,true,true,""); }
        if (i==6) { policy(player,true,true,"policy"); policy(player,true,true,""); }
        vlc_player_Unlock(player);
        vlc_player_Delete(player); // Mainloop performs the actual native persistence.
        media=vlc_ml_get_media(ml,id); assert(media);
        const bool positive=media->f_progress>.6;
        const bool recent=media->i_last_played_date>0;
        const bool expect_progress=i==0;
        const bool expect_recent=i==0 || i==2 || i==5;
        char name[160]; snprintf(name,sizeof(name),"native persistence policy %u: history=%d resume=%d",i,expect_recent,expect_progress);
        check(positive==expect_progress && recent==expect_recent,name);
        vlc_ml_release(media);free(uri);
    }
    // Unknown automatic media must not be inserted while saving is disabled.
    char *uri; if(asprintf(&uri,"%s#no-row",base)<0) abort();
    vlc_player_t *player=vlc_player_New(VLC_OBJECT(vlc->p_libvlc_int),VLC_PLAYER_LOCK_NORMAL);assert(player);
    vlc_player_Lock(player); policy(player,false,true,"");
    input_item_t *item=input_item_New(uri,"Private new item"); assert(item);
    assert(vlc_player_SetCurrentMedia(player,item)==VLC_SUCCESS);input_item_Release(item);
    vlc_player_Unlock(player);vlc_player_Delete(player);
    vlc_ml_media_t *media=vlc_ml_get_media_by_mrl(ml,uri);
    check(media==NULL,"history off does not insert automatic external media");
    if(media)vlc_ml_release(media);
    free(uri);free(base);
}
static void restoration_checks(libvlc_instance_t *vlc, vlc_medialibrary_t *ml, const char *fixture)
{
    char *base=vlc_path2uri(fixture,NULL),*uri;
    if(asprintf(&uri,"%s#restore-policy",base)<0)abort();
    free(base);
    vlc_ml_media_t *media=vlc_ml_new_external_media(ml,uri);assert(media);
    const int64_t id=media->i_id;vlc_ml_release(media);
    assert(vlc_ml_media_set_type(ml,id,VLC_ML_MEDIA_TYPE_VIDEO)==VLC_SUCCESS);
    assert(vlc_ml_media_set_playback_state(ml,id,VLC_ML_PLAYBACK_STATE_TITLE,"3")==VLC_SUCCESS);
    assert(vlc_ml_media_set_playback_state(ml,id,VLC_ML_PLAYBACK_STATE_ASPECT_RATIO,"16:9")==VLC_SUCCESS);
    assert(vlc_ml_media_update_progress(ml,id,.5)==VLC_SUCCESS);
    const int64_t restore=config_GetInt("restore-playback-pos");
    config_PutInt("restore-playback-pos",VLC_PLAYER_RESTORE_PLAYBACK_POS_ALWAYS);
    vlc_player_t *player=vlc_player_New(VLC_OBJECT(vlc->p_libvlc_int),VLC_PLAYER_LOCK_NORMAL);assert(player);
    vlc_player_Lock(player);
    input_item_t *item=input_item_New(uri,"Owned restore fixture");assert(item);
    assert(vlc_player_SetCurrentMedia(player,item)==VLC_SUCCESS);input_item_Release(item);
    check(player->input->ml.restore==VLC_RESTOREPOINT_TITLE&&player->input->ml.states.current_title==3
          &&player->input->ml.pos==.5,
          "permitted saved position arms title restoration under Always policy");
    vlc_player_RestorePlaybackPos(player);vlc_player_RestorePlaybackPos(player);
    check(player->input->ml.states.aspect_ratio&&!strcmp(player->input->ml.states.aspect_ratio,"16:9")
        &&player->input->ml.states.current_title==3,"repeated restore replaces owned preference strings safely");
    policy(player,true,false,"");vlc_player_RestorePlaybackPos(player);
    check(player->input->ml.restore==VLC_RESTOREPOINT_NONE&&!player->input->ml.delay_restore&&player->input->ml.pos<0,
          "storage permission blocks forced position/title restoration independently of Always policy");
    assert(vlc_player_ClearHistory(player)==VLC_SUCCESS);
    vlc_player_Unlock(player);vlc_player_Delete(player);
    player=vlc_player_New(VLC_OBJECT(vlc->p_libvlc_int),VLC_PLAYER_LOCK_NORMAL);assert(player);
    vlc_player_Lock(player);item=input_item_New(uri,"Owned cleared restore fixture");assert(item);
    assert(vlc_player_SetCurrentMedia(player,item)==VLC_SUCCESS);input_item_Release(item);
    char *title=NULL;const int result=vlc_ml_media_get_playback_state(ml,id,VLC_ML_PLAYBACK_STATE_TITLE,&title);
    check(result==VLC_SUCCESS&&title&&!strcmp(title,"3")&&player->input->ml.restore==VLC_RESTOREPOINT_NONE,
          "cleared progress does not arm retained title preference on a new input");
    free(title);vlc_player_Unlock(player);vlc_player_Delete(player);free(uri);
    config_PutInt("restore-playback-pos",restore);
}
int main(void)
{
    if (getenv("VLC_PLUGIN_PATH") == NULL) test_init();
    const char *fixture = getenv("VLC_HISTORY_FIXTURE");
    if (!fixture || !getenv("VLC_USERDATA_PATH")) return 2;
    const bool library_off = getenv("VLC_HISTORY_LIBRARY_OFF") != NULL;
    if (library_off) disabled_startup_checks(fixture);
    libvlc_instance_t *vlc = new_instance(true);
    if (!vlc) return 2;
    vlc_medialibrary_t *ml = vlc_ml_instance_get(vlc->p_libvlc_int);
    if (!ml) return 2;
    // First database operation: an empty fresh library must initialize before
    // clearHistory opens its SQLite transaction (no preceding Get/List query).
    check(vlc_ml_clear_history(ml,VLC_ML_HISTORY_TYPE_GLOBAL)==VLC_SUCCESS,
          "first operation can clear a fresh uninitialized library");
    if (vlc_ml_count_history(ml, NULL, VLC_ML_HISTORY_TYPE_GLOBAL) != 0) return 2;
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
    if (library_off)
    {
        uri=vlc_path2uri(fixture,NULL);assert(uri);
        char *cached_uri;if(asprintf(&cached_uri,"%s.cached.wav",uri)<0)abort();free(uri);
        vlc_ml_media_t *cached=vlc_ml_new_external_media(ml,cached_uri);assert(cached);
        const int64_t cached_id=cached->i_id;vlc_ml_release(cached);
        assert(vlc_ml_media_set_type(ml,cached_id,VLC_ML_MEDIA_TYPE_VIDEO)==VLC_SUCCESS);
        libvlc_release(vlc);
        // The companion inserts an owned queued task and cached removable path
        // while no native instance has the database open.
        printf("SEEDED cached=%"PRId64"\n",cached_id);fflush(stdout);
        if(!command("offline\n"))return 2;
        vlc=new_instance(false);assert(vlc);
        ml=vlc_ml_history_get(vlc->p_libvlc_int);assert(ml);
        check(!vlc_ml_instance_get(vlc->p_libvlc_int)
              && retained(ml,local_id,playlist_id),
              "Library-off history reuses saved favorites, playlists and playback preferences");
        cached=vlc_ml_get_media(ml,cached_id);
        check(cached && cached->p_files && cached->p_files->i_nb_items==1
              && cached->p_files->p_items[0].psz_mrl
              && !strcmp(cached->p_files->p_items[0].psz_mrl,cached_uri),
              "history-only reads reconstruct removable media from cached mountpoints");
        if(cached)vlc_ml_release(cached);
        cached=vlc_ml_get_media_by_mrl(ml,cached_uri);
        check(cached && cached->i_id==cached_id,"history-only lookup resolves existing removable media without duplicate rows");
        if(cached)vlc_ml_release(cached);
        free(cached_uri);
    }
    const size_t original_folder_count=vlc_ml_count_folders(ml,NULL);
    printf("READY local=%"PRId64" network=%"PRId64"\n", local_id, network_id);
    fflush(stdout);
    if (!command("blocked\n")) return 2;
    vlc_player_t *player=vlc_player_New(VLC_OBJECT(vlc->p_libvlc_int),VLC_PLAYER_LOCK_NORMAL);
    assert(player);vlc_player_Lock(player);
    uri=vlc_path2uri(fixture,NULL);
    input_item_t *item=input_item_New(uri,"Owned current media"); free(uri);assert(item);
    assert(vlc_player_SetCurrentMedia(player,item)==VLC_SUCCESS);input_item_Release(item);
    assert(player->input); player->input->position=.8;
    player->input->ml.delay_restore=true;player->input->ml.pos=.375;
    const uint64_t history_epoch=player->history_epoch, position_epoch=player->position_epoch;
    const int blocked = vlc_player_ClearHistory(player);
    check(player->history_epoch==history_epoch && player->position_epoch==position_epoch
        && player->input->ml.delay_restore && player->input->ml.pos==.375,
        "failed clear preserves active input permissions and pending restore");
    check(blocked != VLC_SUCCESS, "database failure reaches native caller");
    local = vlc_ml_get_media(ml, local_id);network = vlc_ml_get_media(ml, network_id);
    check(local && network && local->f_progress == .375 && network->f_progress == .625
          && vlc_ml_count_history(ml, NULL, VLC_ML_HISTORY_TYPE_GLOBAL) == 2
          && retained(ml, local_id, playlist_id), "failed transaction leaves history and saved data unchanged");
    if (local) vlc_ml_release(local);
    if (network) vlc_ml_release(network);
    printf("RETRY\n");fflush(stdout);
    if (!command("retry\n")) return 2;
    check(vlc_player_ClearHistory(player) == VLC_SUCCESS,
          "clear succeeds after database failure is removed");
    check(player->history_epoch!=history_epoch && player->position_epoch!=position_epoch
        && !player->input->ml.delay_restore && player->input->ml.pos<0
        && vlc_player_GetCurrentMedia(player)!=NULL && player->input->position==.8,
        "successful clear invalidates pending state without changing current media or position");
    vlc_player_Unlock(player);vlc_player_Delete(player);
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
    vlc = new_instance(!library_off);
    ml = vlc ? vlc_ml_history_get(vlc->p_libvlc_int) : NULL;
    check(ml && vlc_ml_count_history(ml, NULL, VLC_ML_HISTORY_TYPE_GLOBAL) == 0
          && retained(ml, local_id, playlist_id), "cleared history and retained data survive native restart");
    if (vlc)
    {
        matching_checks();
        storage_checks(vlc,ml,fixture);
        restoration_checks(vlc,ml,fixture);
        // A real input queued for retirement while locked must also be unable
        // to repopulate history after clear.
        player=vlc_player_New(VLC_OBJECT(vlc->p_libvlc_int),VLC_PLAYER_LOCK_NORMAL);assert(player);
        vlc_player_Lock(player);uri=vlc_path2uri(fixture,NULL);
        item=input_item_New(uri,"Owned retiring media");free(uri);assert(item);
        assert(vlc_player_SetCurrentMedia(player,item)==VLC_SUCCESS);input_item_Release(item);
        player->input->position=.75;player->input->ml.delay_restore=true;
        assert(vlc_player_SetCurrentMedia(player,NULL)==VLC_SUCCESS);
        check(!vlc_list_is_empty(&player->mainloop.joinable_inputs),"real input is pending retirement while player is locked");
        check(vlc_player_ClearHistory(player)==VLC_SUCCESS,"clear succeeds with retiring input");
        struct vlc_player_input *retiring;
        vlc_list_foreach(retiring,&player->mainloop.joinable_inputs,node)
            check(!retiring->ml.delay_restore && retiring->history_epoch!=player->history_epoch,
                "retiring input restore and write generation invalidated");
        vlc_player_Unlock(player);vlc_player_Delete(player);
        check(vlc_ml_count_history(ml,NULL,VLC_ML_HISTORY_TYPE_GLOBAL)==0,
              "retiring input shutdown does not resurrect cleared history");
        if (library_off)
            check(!vlc_ml_instance_get(vlc->p_libvlc_int)
                  && !var_InheritBool(vlc->p_libvlc_int,"media-library")
                  && vlc_ml_count_folders(ml,NULL)==original_folder_count,
                  "save, restore, clear and restart keep Library off and roots unchanged");
        libvlc_release(vlc);
    }
    printf("RESULT: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
