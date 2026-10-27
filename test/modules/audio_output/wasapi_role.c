/* SPDX-License-Identifier: LGPL-2.1-or-later */
#define INITGUID
#define COBJMACROS
#define CONST_VTABLE
#include <windows.h>
#include <audioclient.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "wasapi_role.h"

struct client_probe {
    IAudioClient base;
    IAudioClient2 properties;
    HRESULT query_result, set_result;
    unsigned queries, sets, releases;
    bool correct_iid;
    AudioClientProperties received;
};

static HRESULT STDMETHODCALLTYPE query(IAudioClient *client, REFIID iid, void **out)
{
    struct client_probe *probe = (void *)client;
    ++probe->queries;
    probe->correct_iid = IsEqualIID(iid, &IID_IAudioClient2);
    *out = SUCCEEDED(probe->query_result) ? &probe->properties : NULL;
    return probe->query_result;
}

static struct client_probe *owner(IAudioClient2 *client)
{
    return (void *)((char *)client - offsetof(struct client_probe, properties));
}

static HRESULT STDMETHODCALLTYPE set_properties(IAudioClient2 *client,
                                                const AudioClientProperties *properties)
{
    struct client_probe *probe = owner(client);
    ++probe->sets;
    probe->received = *properties;
    return probe->set_result;
}

static ULONG STDMETHODCALLTYPE release_properties(IAudioClient2 *client)
{
    ++owner(client)->releases;
    return 1;
}

static const IAudioClientVtbl base_vtable = { .QueryInterface = query };
static const IAudioClient2Vtbl properties_vtable = {
    .Release = release_properties,
    .SetClientProperties = set_properties,
};

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "FAIL line%d: %s\n", __LINE__, #condition); return 1; \
} } while (0)

int main(void)
{
    static const struct { const char *role; AUDIO_STREAM_CATEGORY category; } cases[] = {
        {"video", AudioCategory_Movie}, {"music", AudioCategory_Media},
        {"communication", AudioCategory_Communications}, {"game", AudioCategory_GameMedia},
        {"notification", AudioCategory_Alerts}, {"animation", AudioCategory_SoundEffects},
        {"production", AudioCategory_Other}, {"accessibility", AudioCategory_Speech},
        {"test", AudioCategory_Other}, {"legacy/unmanaged", AudioCategory_Other},
        {"", AudioCategory_Other}, {NULL, AudioCategory_Other},
    };
    unsigned passed = 0;
    for (unsigned i = 0; i < sizeof(cases)/sizeof(cases[0]); ++i)
    {
        struct client_probe probe = {.base = {&base_vtable}, .properties = {&properties_vtable}};
        CHECK(vlc_wasapi_SetMediaRole(&probe.base, cases[i].role) == S_OK);
        CHECK(probe.queries == 1 && probe.correct_iid);
        CHECK(probe.sets == 1 && probe.releases == 1);
        CHECK(probe.received.cbSize == sizeof(AudioClientProperties));
        CHECK(!probe.received.bIsOffload);
        CHECK(probe.received.eCategory == cases[i].category);
        CHECK(probe.received.Options == AUDCLNT_STREAMOPTIONS_NONE);
        printf("PASS role %s\n", cases[i].role != NULL ? cases[i].role : "(null)");
        ++passed;
    }
    for (unsigned i = 0; i < 2; ++i)
    {
        const HRESULT expected = i == 0 ? E_NOINTERFACE : E_FAIL;
        struct client_probe probe = {.base = {&base_vtable}, .query_result = expected};
        CHECK(vlc_wasapi_SetMediaRole(&probe.base, "music") == expected);
        CHECK(probe.queries == 1 && probe.correct_iid && probe.sets == 0 && probe.releases == 0);
        printf("PASS query failure %u\n", i);
        ++passed;
    }
    struct client_probe probe = {.base = {&base_vtable}, .properties = {&properties_vtable},
                                 .set_result = E_INVALIDARG};
    CHECK(vlc_wasapi_SetMediaRole(&probe.base, "video") == E_INVALIDARG);
    CHECK(probe.queries == 1 && probe.sets == 1 && probe.releases == 1);
    puts("PASS rejected property releases acquired interface");
    printf("%u cases passed\n", ++passed);
    return 0;
}
