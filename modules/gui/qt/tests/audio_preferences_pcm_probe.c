// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include <vlc_variables.h>

int vlc_test_audio_preferences_amem(vlc_object_t *,
    void (*)(void *,const void *,unsigned,int64_t),void *);

int vlc_test_audio_preferences_amem(vlc_object_t *root,
    void (*play)(void *,const void *,unsigned,int64_t),void *opaque)
{
    if(root==NULL||play==NULL||opaque==NULL)return VLC_EGENERIC;
    /* Only the generated fixture's transport changes. In particular no
     * audio-filter, equalizer variable, or native config value is written.
     * Install before vlc_playlist_New so its first aout opens AMEM directly. */
    if(var_Create(root,"amem-play",VLC_VAR_ADDRESS)!=VLC_SUCCESS
       ||var_Create(root,"amem-data",VLC_VAR_ADDRESS)!=VLC_SUCCESS
       ||var_Create(root,"amem-format",VLC_VAR_STRING)!=VLC_SUCCESS
       ||var_Create(root,"amem-rate",VLC_VAR_INTEGER)!=VLC_SUCCESS
       ||var_Create(root,"amem-channels",VLC_VAR_INTEGER)!=VLC_SUCCESS
       ||var_Create(root,"aout",VLC_VAR_STRING)!=VLC_SUCCESS)return VLC_ENOMEM;
    if(var_SetAddress(root,"amem-play",play)!=VLC_SUCCESS
       ||var_SetAddress(root,"amem-data",opaque)!=VLC_SUCCESS
       ||var_SetString(root,"amem-format","FL32")!=VLC_SUCCESS
       ||var_SetInteger(root,"amem-rate",44100)!=VLC_SUCCESS
       ||var_SetInteger(root,"amem-channels",1)!=VLC_SUCCESS
       ||var_SetString(root,"aout","amem,none")!=VLC_SUCCESS)return VLC_EGENERIC;
    return VLC_SUCCESS;
}
