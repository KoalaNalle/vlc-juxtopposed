/* SPDX-License-Identifier: LGPL-2.1-or-later */
#ifndef VLC_WASAPI_ROLE_H
#define VLC_WASAPI_ROLE_H

#include <audioclient.h>
#include <string.h>

/* Classify the stream, independently of which endpoint MMDevice selected.
 * Production/test have no specific Windows category; keep them Other. */
static inline AUDIO_STREAM_CATEGORY vlc_wasapi_MediaCategory(const char *role)
{
    if (role != NULL)
    {
        if (!strcmp(role, "video"))         return AudioCategory_Movie;
        if (!strcmp(role, "music"))         return AudioCategory_Media;
        if (!strcmp(role, "communication")) return AudioCategory_Communications;
        if (!strcmp(role, "game"))          return AudioCategory_GameMedia;
        if (!strcmp(role, "notification"))  return AudioCategory_Alerts;
        if (!strcmp(role, "animation"))     return AudioCategory_SoundEffects;
        if (!strcmp(role, "accessibility")) return AudioCategory_Speech;
    }
    return AudioCategory_Other;
}

/* Set before format negotiation/Initialize. Query dynamically so an older
 * interface or a rejected preference does not prevent normal audio playback. */
static inline HRESULT vlc_wasapi_SetMediaRole(IAudioClient *client,
                                             const char *role)
{
    IAudioClient2 *properties_client;
    HRESULT hr = IAudioClient_QueryInterface(client, &IID_IAudioClient2,
                                            (void **)&properties_client);
    if (FAILED(hr))
        return hr;

    const AudioClientProperties properties = {
        .cbSize = sizeof(AudioClientProperties),
        .bIsOffload = FALSE,
        .eCategory = vlc_wasapi_MediaCategory(role),
        .Options = AUDCLNT_STREAMOPTIONS_NONE,
    };
    hr = IAudioClient2_SetClientProperties(properties_client, &properties);
    IAudioClient2_Release(properties_client);
    return hr;
}

#endif
