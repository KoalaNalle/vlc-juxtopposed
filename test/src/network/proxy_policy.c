// SPDX-License-Identifier: LGPL-2.1-or-later
// Reuse the owned Core import-slot boundary, not its resolver test main.
int borrowed_proxy_fixture_main(int, char **);
#define main borrowed_proxy_fixture_main
#include "proxy_windows.c"
#undef main

static bool object_matches(vlc_object_t *object, const char *expected)
{
    char *actual = vlc_getProxyUrlForObject(object, "http://target.owned.invalid/media?owned=1");
    bool same = expected ? actual && !strcmp(actual, expected) : actual == NULL;
    free(actual);
    return same;
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    unsigned count = 0; int result = 1;
    libvlc_instance_t *instance = NULL;
    vlc_object_t *child = NULL;
    CHECK(install_owned_settings(), "actual Core Windows settings boundary is owned");
    const char *options[] = {"--ignore-config", "--quiet", "--no-media-library",
        "--no-metadata-network-access", "--no-auto-preparse", "--extraintf=", "--control="};
    instance = libvlc_new(ARRAY_SIZE(options), options);
    CHECK(instance != NULL, "actual native instance initializes");
    vlc_object_t *root = VLC_OBJECT(instance->p_libvlc_int);
    child = vlc_object_create(root, sizeof(*child));
    CHECK(child != NULL, "owned per-media object initializes");
    config_PutPsz("http-proxy", "http://owned@proxy.owned.invalid:8080");
    config_PutPsz("http-proxy-pwd", "saved-secret");
    CHECK(object_matches(child, "http://owned:saved-secret@proxy.owned.invalid:8080"), "saved endpoint uses its own separate password");
    var_Create(root, "http-proxy", VLC_VAR_STRING);
    var_SetString(root, "http-proxy", "http://owned@other.owned.invalid:8181");
    CHECK(object_matches(child, "http://owned@other.owned.invalid:8181"), "CLI proxy overrides storage without receiving its old secret");
    var_SetString(root, "http-proxy", "http://owned@proxy.owned.invalid:8080");
    CHECK(object_matches(child, "http://owned:saved-secret@proxy.owned.invalid:8080"), "same CLI authority may retain its saved secret");
    var_Create(root, "http-proxy-pwd", VLC_VAR_STRING);
    var_SetString(root, "http-proxy-pwd", "cli-secret");
    CHECK(object_matches(child, "http://owned:cli-secret@proxy.owned.invalid:8080"), "explicit CLI password overrides storage");
    var_Create(child, "http-proxy", VLC_VAR_STRING);
    var_SetString(child, "http-proxy", "http://owned@media.owned.invalid:8282");
    CHECK(object_matches(child, "http://owned@media.owned.invalid:8282"), "per-media endpoint does not receive outer CLI credentials");
    var_Create(child, "http-proxy-pwd", VLC_VAR_STRING);
    var_SetString(child, "http-proxy-pwd", "media-secret");
    CHECK(object_matches(child, "http://owned:media-secret@media.owned.invalid:8282"), "per-media endpoint receives its own explicit credentials");
    var_SetString(child, "http-proxy", "http://own:embedded@media.owned.invalid:8282");
    CHECK(object_matches(child, "http://own:embedded@media.owned.invalid:8282"), "embedded credentials remain scoped to their explicit URL");
    const unsigned before = settings_calls;
    var_SetString(child, "http-proxy", "DIRECT://");
    CHECK(object_matches(child, NULL) && settings_calls == before, "explicit direct mode bypasses saved and Windows policies");
    owned_proxy = L"system.owned.invalid:8080";
    var_SetString(child, "http-proxy", "");
    CHECK(object_matches(child, "http://system.owned.invalid:8080"), "explicit empty mode uses system policy even with a saved manual proxy");
    CHECK(settings_calls > before, "actual Windows resolver was called through the owned boundary");
    var_Destroy(child, "http-proxy-pwd");
    var_Destroy(child, "http-proxy");
    var_Destroy(root, "http-proxy");
    CHECK(object_matches(child, "http://owned:cli-secret@proxy.owned.invalid:8080"), "CLI password alone remains scoped to the saved endpoint");
    var_Destroy(root, "http-proxy-pwd");
    CHECK(object_matches(child, "http://owned:saved-secret@proxy.owned.invalid:8080"), "removing runtime overrides restores saved policy");
    config_PutPsz("http-proxy", "");
    CHECK(object_matches(child, "http://system.owned.invalid:8080"), "system endpoint never inherits a saved explicit password");
    result = 0;
out:
    if (child) vlc_object_delete(child);
    if (instance) libvlc_release(instance);
    restore_owned_settings();
    return result;
}
