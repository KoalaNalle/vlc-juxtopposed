// SPDX-License-Identifier: LGPL-2.1-or-later
/* Optional native diagnostic: use only generated slice fixtures and fresh
 * VLC_USERDATA_PATH. This is backend evidence, not a native Qt journey. */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#undef NDEBUG
#include <assert.h>
#include <vlc_common.h>
#include <vlc_media_library.h>
#include <vlc_url.h>
#include <vlc/vlc.h>
#include "../../lib/libvlc_internal.h"

static libvlc_instance_t *create_instance(const char *logfile)
{
    const char *args[] = {
        "--ignore-config", "--media-library", "--no-auto-preparse",
        "--verbose=2", "--file-logging", logfile,
    };
    libvlc_instance_t *vlc = libvlc_new(ARRAY_SIZE(args), args);
    assert(vlc != NULL);
    assert(vlc_ml_instance_get(vlc->p_libvlc_int) != NULL);
    return vlc;
}

static void library_changed(void *opaque, const vlc_ml_event_t *event)
{
    switch (event->i_type)
    {
        case VLC_ML_EVENT_MEDIA_ADDED:
        case VLC_ML_EVENT_MEDIA_UPDATED:
        case VLC_ML_EVENT_DISCOVERY_COMPLETED:
        case VLC_ML_EVENT_BACKGROUND_IDLE_CHANGED:
            vlc_sem_post(opaque);
            break;
        default:
            break;
    }
}

int main(int argc, char **argv)
{
    const char *userdata = getenv("VLC_USERDATA_PATH");
    if (argc != 2 || userdata == NULL || userdata[0] == '\0')
    {
        fprintf(stderr, "Usage: native_library_slice FIXTURE_FOLDER\n"
                "Set VLC_USERDATA_PATH to a fresh isolated directory.\n");
        return 2;
    }
    libvlc_instance_t *vlc = create_instance("--logfile=native-library-cold.log");
    vlc_medialibrary_t *ml = vlc_ml_instance_get(vlc->p_libvlc_int);
    /* Refuse to modify a database containing existing media or playlists. */
    assert(vlc_ml_count_media(ml, NULL) == 0);
    assert(vlc_ml_count_playlists(ml, NULL, VLC_ML_PLAYLIST_TYPE_ALL) == 0);

    vlc_ml_folder_list_t *folders = vlc_ml_list_entry_points(ml, NULL);
    assert(folders && folders->i_nb_items == 0);
    vlc_ml_folder_list_release(folders);
    vlc_sem_t changed;
    vlc_sem_init(&changed, 0);
    vlc_ml_event_callback_t *callback = vlc_ml_event_register_callback(ml, library_changed, &changed);
    assert(callback != NULL);

    char *uri = vlc_path2uri(argv[1], NULL);
    assert(uri != NULL);
    assert(vlc_ml_add_folder(ml, uri) == VLC_SUCCESS);
    free(uri);
    const vlc_tick_t deadline = vlc_tick_now() + VLC_TICK_FROM_SEC(45);
    size_t video_count = 0, audio_count = 0;
    while (true)
    {
        video_count = vlc_ml_count_video_media(ml, NULL);
        audio_count = vlc_ml_count_audio_media(ml, NULL);
        if ((video_count >= 2 && audio_count >= 1) || vlc_sem_timedwait(&changed, deadline) != 0)
            break;
    }
    vlc_ml_event_unregister_callback(ml, callback);
    fprintf(stderr, "Indexed videos=%zu audio=%zu\n", video_count, audio_count);
    assert(video_count == 2 && audio_count == 1);

    vlc_ml_query_params_t search = vlc_ml_query_params_create();
    search.psz_pattern = "Waveform";
    assert(vlc_ml_count_audio_media(ml, &search) == 1);
    assert(vlc_ml_count_video_media(ml, &search) == 0);
    search.psz_pattern = "not-a-real-title";
    assert(vlc_ml_count_media(ml, &search) == 0);
    search.psz_pattern = NULL;
    assert(vlc_ml_count_media(ml, &search) == 3);

    vlc_ml_media_list_t *videos = vlc_ml_list_video_media(ml, NULL);
    vlc_ml_media_list_t *audios = vlc_ml_list_audio_media(ml, NULL);
    assert(videos && videos->i_nb_items == 2);
    assert(audios && audios->i_nb_items == 1);
    int64_t media_ids[] = {videos->p_items[0].i_id, audios->p_items[0].i_id};
    vlc_ml_media_list_release(videos);
    vlc_ml_media_list_release(audios);

    vlc_ml_playlist_t *playlist = vlc_ml_playlist_create(ml, "Slice Mixed Playlist");
    assert(playlist != NULL);
    const int64_t playlist_id = playlist->i_id;
    vlc_ml_playlist_release(playlist);
    assert(vlc_ml_playlist_append(ml, playlist_id, media_ids, ARRAY_SIZE(media_ids)) == VLC_SUCCESS);
    assert(vlc_ml_count_playlists(ml, NULL, VLC_ML_PLAYLIST_TYPE_ALL) == 1);
    vlc_ml_media_list_t *items = vlc_ml_list_playlist_media(ml, NULL, playlist_id);
    assert(items && items->i_nb_items == 2);
    assert(items->p_items[0].i_id == media_ids[0]);
    assert(items->p_items[1].i_id == media_ids[1]);
    vlc_ml_media_list_release(items);
    libvlc_release(vlc);

    /* Warm restart must retain indexed media and mixed playlist contents. */
    vlc = create_instance("--logfile=native-library-warm.log");
    ml = vlc_ml_instance_get(vlc->p_libvlc_int);
    assert(vlc_ml_count_video_media(ml, NULL) == 2);
    assert(vlc_ml_count_audio_media(ml, NULL) == 1);
    assert(vlc_ml_count_playlist_media(ml, NULL, playlist_id) == 2);
    libvlc_release(vlc);
    printf("Native library passes real folder indexing, search, mixed playlist order and warm persistence.\n");
    return 0;
}
