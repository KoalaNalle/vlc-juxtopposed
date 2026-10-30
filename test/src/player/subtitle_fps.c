// SPDX-License-Identifier: LGPL-2.1-or-later
// Actual Core FPS consumers and decoded snapshots; pixel assertions run outside.
#include "../../libvlc/test.h"
#include "../../../lib/libvlc_internal.h"
#include <vlc_player.h>
#include <vlc_vout.h>
#include <vlc_url.h>
#include <sys/stat.h>

struct context { vlc_mutex_t mutex; vlc_cond_t wait; };

static void signal_context(struct context *ctx)
{
    vlc_mutex_lock(&ctx->mutex);
    vlc_cond_signal(&ctx->wait);
    vlc_mutex_unlock(&ctx->mutex);
}
static void on_state(vlc_player_t *p, enum vlc_player_state state, void *data)
{ (void)p; (void)state; signal_context(data); }
static void on_track(vlc_player_t *p, vlc_es_id_t *old, vlc_es_id_t *selected, void *data)
{ (void)p; (void)old; (void)selected; signal_context(data); }
static void on_position(vlc_player_t *p, vlc_tick_t time, double pos, void *data)
{ (void)p; (void)time; (void)pos; signal_context(data); }

/* Called with the player locked. The external mutex prevents missed signals. */
static void wait_event(vlc_player_t *p, struct context *ctx, vlc_tick_t deadline)
{
    vlc_mutex_lock(&ctx->mutex);
    vlc_player_Unlock(p);
    vlc_cond_timedwait(&ctx->wait, &ctx->mutex, deadline);
    vlc_mutex_unlock(&ctx->mutex);
    vlc_player_Lock(p);
}
static void settle(vlc_player_t *p, struct context *ctx, unsigned ms)
{
    vlc_tick_t end=vlc_tick_now()+VLC_TICK_FROM_MS(ms);
    while (vlc_tick_now()<end) wait_event(p,ctx,end);
}
static bool wait_ready(vlc_player_t *p, struct context *ctx, size_t tracks)
{
    vlc_tick_t end=vlc_tick_now()+VLC_TICK_FROM_SEC(12);
    for (;;)
    {
        if (vlc_player_GetState(p)==VLC_PLAYER_STATE_PAUSED
         && vlc_player_GetTrackCount(p,SPU_ES)==tracks
         && vlc_player_GetSelectedTrack(p,SPU_ES)!=NULL) return true;
        if (vlc_tick_now()>=end) return false;
        wait_event(p,ctx,end);
    }
}
static bool advance(vlc_player_t *p, struct context *ctx, vlc_tick_t initial,
                    vlc_tick_t amount)
{
    vlc_tick_t end=vlc_tick_now()+VLC_TICK_FROM_SEC(8);
    while (vlc_player_GetState(p)!=VLC_PLAYER_STATE_PLAYING
        || vlc_player_GetTime(p)<initial+amount)
    {
        if (vlc_tick_now()>=end) return false;
        wait_event(p,ctx,end);
    }
    return true;
}

static bool picture(vlc_player_t *p, const char *directory, const char *name)
{
    char *path=NULL;
    if (asprintf(&path,"%s/%s.png",directory,name)<0) return false;
    struct stat info;
    assert(stat(path,&info)!=0); /* Every producer uses a fresh output directory. */
    const vlc_tick_t time=vlc_player_GetTime(p);
    const enum vlc_player_state state=vlc_player_GetState(p);
    const float fps=vlc_player_GetAssociatedSubsFPS(p);
    const vlc_tick_t delay=vlc_player_GetCategoryDelay(p,SPU_ES);
    vlc_player_Unlock(p);
    vout_thread_t *vout=vlc_player_vout_Hold(p);
    assert(vout);
    var_Create(vout,"snapshot-width",VLC_VAR_INTEGER);var_SetInteger(vout,"snapshot-width",960);
    var_Create(vout,"snapshot-height",VLC_VAR_INTEGER);var_SetInteger(vout,"snapshot-height",540);
    var_Create(vout,"snapshot-path",VLC_VAR_STRING);var_SetString(vout,"snapshot-path",path);
    var_Create(vout,"snapshot-format",VLC_VAR_STRING);var_SetString(vout,"snapshot-format","png");
    var_TriggerCallback(vout,"video-snapshot");vout_Release(vout);
    bool saved=stat(path,&info)==0 && info.st_size>24;
    vlc_player_Lock(p);
    printf("{\"step\":\"%s\",\"timeUs\":%"PRId64",\"state\":%d,\"fps\":%.9g,\"delayUs\":%"PRId64",\"saved\":%s}\n",
           name,time,state,(double)fps,delay,saved?"true":"false");
    fflush(stdout);free(path);return saved;
}

int main(int argc,char **argv)
{
    if (argc!=8) return 77; /* video, subtitle, output, FPS, mode, seek seconds, delay us */
    const float fps=strtof(argv[4],NULL);
    const char *mode=argv[5];
    const bool startup=!strcmp(mode,"startup-single") || !strcmp(mode,"startup-batch");
    const vlc_tick_t target=(vlc_tick_t)(strtod(argv[6],NULL)*CLOCK_FREQ);
    const vlc_tick_t delay=strtoll(argv[7],NULL,10);
    assert(fps>=0 && target>0 && target<VLC_TICK_FROM_SEC(50));
    assert(!strcmp(mode,"reference") || !strcmp(mode,"hold")
        || !strcmp(mode,"inherited") || !strcmp(mode,"playing") || startup);
    test_init();
    char *fps_option=NULL,*log_option=NULL,*subtitle_option=NULL,*start_option=NULL;
    assert(asprintf(&fps_option,"--sub-fps=%.9g",!strcmp(mode,"inherited")?(double)fps:0.)>=0);
    assert(asprintf(&log_option,"--logfile=%s/native.log",argv[3])>=0);
    assert(asprintf(&subtitle_option,":sub-file=%s",argv[2])>=0);
    const char *args[]={"--ignore-config","--no-media-library","--no-metadata-network-access",
        "--no-ml-network-discovery","--vout=vdummy","--aout=adummy","--no-hw-dec",
        "--no-osd","--no-video-title-show","--no-sub-autodetect-file",
        "--verbose=2","--file-logging",log_option,fps_option};
    libvlc_instance_t *instance=libvlc_new(ARRAY_SIZE(args),args);assert(instance);
    struct context ctx;vlc_mutex_init(&ctx.mutex);vlc_cond_init(&ctx.wait);
    vlc_player_t *p=vlc_player_New(VLC_OBJECT(instance->p_libvlc_int),VLC_PLAYER_LOCK_NORMAL);assert(p);
    const struct vlc_player_cbs cbs={.on_state_changed=on_state,.on_track_selection_changed=on_track,
                                     .on_position_changed=on_position};
    vlc_player_Lock(p);
    vlc_player_listener_id *listener=vlc_player_AddListener(p,&cbs,&ctx);assert(listener);
    char *uri=vlc_path2uri(argv[1],NULL);assert(uri);
    input_item_t *media=input_item_New(uri,"generated-inline-timing");assert(media);free(uri);
    input_item_AddOption(media,":start-paused",VLC_INPUT_OPTION_TRUSTED);
    if (startup)
    {
        assert(asprintf(&start_option,":start-time=%.9f",(double)target/CLOCK_FREQ)>=0);
        input_item_AddOption(media,start_option,VLC_INPUT_OPTION_TRUSTED);
        vlc_player_SelectSubtitleLanguage(p,"fin");
    }
    else
        input_item_AddOption(media,subtitle_option,VLC_INPUT_OPTION_TRUSTED);
    bool ok=false;
    if (vlc_player_SetCurrentMedia(p,media)!=VLC_SUCCESS || vlc_player_Start(p)!=VLC_SUCCESS
     || !wait_ready(p,&ctx,startup?2:1)) goto done;
    if (!startup)
    {
        if (vlc_player_SetCategoryDelay(p,SPU_ES,delay,VLC_PLAYER_WHENCE_ABSOLUTE)!=VLC_SUCCESS) goto done;
        vlc_player_SetTime(p,target);
    }
    settle(p,&ctx,startup?1000:800);
    if (!picture(p,argv[3],"initial")) goto done;
    if (startup)
    {
        if (vlc_player_SetCategoryDelay(p,SPU_ES,delay,VLC_PLAYER_WHENCE_ABSOLUTE)!=VLC_SUCCESS) goto done;
        settle(p,&ctx,1000);
        if (!picture(p,argv[3],"delayed")) goto done;
        if (vlc_player_SetCategoryDelay(p,SPU_ES,0,VLC_PLAYER_WHENCE_ABSOLUTE)!=VLC_SUCCESS) goto done;
        if (!strcmp(mode,"startup-batch"))
        {
            /* Default preset submits these changes under the same player lock. */
            vlc_player_SetAssociatedSubsFPS(p,0);
            for (size_t i=0;i<vlc_player_GetTrackCount(p,SPU_ES);++i)
            {
                const struct vlc_player_track *track=vlc_player_GetTrackAt(p,SPU_ES,i);
                if (vlc_player_SetEsIdDelay(p,track->es_id,INT64_MAX,
                                          VLC_PLAYER_WHENCE_ABSOLUTE)!=VLC_SUCCESS) goto done;
            }
        }
        settle(p,&ctx,1000);
        if (!picture(p,argv[3],"reset")) goto done;
        /* A real user seek must replace any cached paused-redraw target. */
        vlc_player_SetTime(p,VLC_TICK_FROM_SEC(3));
        settle(p,&ctx,1000);
        if (!picture(p,argv[3],"user-seek")) goto done;
        if (vlc_player_SetCategoryDelay(p,SPU_ES,delay,VLC_PLAYER_WHENCE_ABSOLUTE)!=VLC_SUCCESS) goto done;
        vlc_player_SetAssociatedSubsFPS(p,0);
        settle(p,&ctx,1000);
        if (!picture(p,argv[3],"after-seek-redraw")) goto done;
    }
    else if (!strcmp(mode,"hold"))
    {
        vlc_player_SetAssociatedSubsFPS(p,fps);settle(p,&ctx,800);
        if (!picture(p,argv[3],"changed")) goto done;
        vlc_player_SetAssociatedSubsFPS(p,0);settle(p,&ctx,800);
        if (!picture(p,argv[3],"reset")) goto done;
    }
    else if (!strcmp(mode,"inherited"))
    {
        vlc_player_SetAssociatedSubsFPS(p,0);settle(p,&ctx,800);
        if (!picture(p,argv[3],"reset")) goto done;
    }
    else if (!strcmp(mode,"playing"))
    {
        vlc_tick_t time=vlc_player_GetTime(p);
        vlc_player_Resume(p);if(!advance(p,&ctx,time,VLC_TICK_FROM_MS(500))) goto done;
        if(!picture(p,argv[3],"playing-before")) goto done;
        vlc_player_SetAssociatedSubsFPS(p,fps);
        time=vlc_player_GetTime(p);if(!advance(p,&ctx,time,VLC_TICK_FROM_MS(500))) goto done;
        if(!picture(p,argv[3],"playing-changed")) goto done;
        vlc_player_SetAssociatedSubsFPS(p,0);
        time=vlc_player_GetTime(p);if(!advance(p,&ctx,time,VLC_TICK_FROM_MS(500))) goto done;
        if(!picture(p,argv[3],"playing-reset")) goto done;
    }
    ok=true;
done:
    vlc_player_Stop(p);vlc_player_RemoveListener(p,listener);vlc_player_Unlock(p);
    vlc_player_Delete(p);input_item_Release(media);libvlc_release(instance);
    free(fps_option);free(log_option);free(subtitle_option);free(start_option);
    printf("{\"executionComplete\":%s}\n",ok?"true":"false");
    return ok?0:1;
}
