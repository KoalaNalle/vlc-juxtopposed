// SPDX-License-Identifier: LGPL-2.1-or-later
// Exact production resolver with an owned Windows-settings boundary. PAC uses
// the real WinHTTP resolver, restricted to an explicitly supplied loopback URL.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include <vlc_configuration.h>
#include <vlc_charset.h>
#include <vlc_stream.h>
#include "../../../lib/libvlc_internal.h"
#include <vlc/vlc.h>
#include <windows.h>
#include <winhttp.h>

static const wchar_t *owned_proxy, *owned_bypass, *owned_pac;
static bool owned_available = true;
static unsigned settings_calls;
BOOL WINAPI own_proxy_config(WINHTTP_CURRENT_USER_IE_PROXY_CONFIG *);
VLC_API char *vlc_getProxyUrl(const char *);
static char *(*resolver)(const char *);
static ULONG_PTR *settings_slot, settings_original;
static wchar_t *owned_copy(const wchar_t *text)
{
    if (!text) return NULL;
    size_t bytes = (wcslen(text) + 1) * sizeof(*text);
    wchar_t *copy = GlobalAlloc(GPTR, bytes);
    if (copy) memcpy(copy, text, bytes);
    return copy;
}
BOOL WINAPI own_proxy_config(WINHTTP_CURRENT_USER_IE_PROXY_CONFIG *config)
{
    settings_calls++;
    memset(config, 0, sizeof(*config));
    if (!owned_available) return FALSE;
    config->lpszProxy = owned_copy(owned_proxy);
    config->lpszProxyBypass = owned_copy(owned_bypass);
    config->lpszAutoConfigUrl = owned_copy(owned_pac);
    return TRUE;
}
#define WinHttpGetIEProxyConfigForCurrentUser own_proxy_config
#define vlc_getProxyUrl owned_getProxyUrl
#include "../../../src/win32/netconf.c"
#undef vlc_getProxyUrl
#undef WinHttpGetIEProxyConfigForCurrentUser

// Change only this owned process's private Core import slot. This exercises the
// actual built DLL while supplying synthetic settings; no host proxy settings
// or personal PAC URL are read, changed or reset.
static bool install_owned_settings(void)
{
    BYTE *base = (BYTE *)GetModuleHandleW(L"libvlccore.dll");
    if (!base) return false;
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)base;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + dos->e_lfanew);
    DWORD rva = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    if (!rva) return false;
    IMAGE_IMPORT_DESCRIPTOR *dll = (IMAGE_IMPORT_DESCRIPTOR *)(base + rva);
    for (; dll->Name; dll++) {
        if (strcasecmp((char *)base + dll->Name, "winhttp.dll") || !dll->OriginalFirstThunk) continue;
        IMAGE_THUNK_DATA *name = (IMAGE_THUNK_DATA *)(base + dll->OriginalFirstThunk);
        IMAGE_THUNK_DATA *address = (IMAGE_THUNK_DATA *)(base + dll->FirstThunk);
        for (; name->u1.AddressOfData; name++, address++) {
            if (IMAGE_SNAP_BY_ORDINAL(name->u1.Ordinal)) continue;
            IMAGE_IMPORT_BY_NAME *symbol = (IMAGE_IMPORT_BY_NAME *)(base + name->u1.AddressOfData);
            if (strcmp((char *)symbol->Name, "WinHttpGetIEProxyConfigForCurrentUser")) continue;
            settings_slot = &address->u1.Function;
            DWORD previous, ignored;
            if (!VirtualProtect(settings_slot, sizeof(*settings_slot), PAGE_READWRITE, &previous)) return false;
            settings_original = *settings_slot;
            InterlockedExchangePointer((void *volatile *)settings_slot, (void *)own_proxy_config);
            VirtualProtect(settings_slot, sizeof(*settings_slot), previous, &ignored);
            return true;
        }
    }
    return false;
}
static void restore_owned_settings(void)
{
    if (!settings_slot || !settings_original) return;
    DWORD previous, ignored;
    if (!VirtualProtect(settings_slot, sizeof(*settings_slot), PAGE_READWRITE, &previous)) return;
    InterlockedExchangePointer((void *volatile *)settings_slot, (void *)settings_original);
    VirtualProtect(settings_slot, sizeof(*settings_slot), previous, &ignored);
}

static bool matches(const char *target, const char *expected)
{
    char *actual = resolver(target);
    bool same = expected ? actual && !strcmp(actual, expected) : actual == NULL;
    free(actual);
    return same;
}
#define CHECK(condition, description) do { \
    count++; bool passed = (condition); \
    printf("%s %u %s\n", passed ? "ok" : "not ok", count, description); \
    if (!passed) goto out; \
} while (0)

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    bool native = false, allow_missing = false;
    for (int i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "--native-core")) native = true;
        else if (!strcmp(argv[i], "--allow-missing-core-hook")) allow_missing = true;
        else return 2;
    }
    resolver = native ? vlc_getProxyUrl : owned_getProxyUrl;
    bool hooked = native && install_owned_settings();
    printf("# native_core=%d owned_settings_boundary=%d\n", native, hooked);
    if (native && !hooked && !allow_missing) return 2;
    const char *options[] = {"--ignore-config", "--quiet", "--no-media-library",
        "--no-metadata-network-access", "--no-auto-preparse", "--extraintf=", "--control="};
    libvlc_instance_t *instance = libvlc_new(ARRAY_SIZE(options), options);
    if (!instance) return 2;
    unsigned count = 0; int result = 1;
    wchar_t *pac = NULL;
    config_PutPsz("http-proxy", "http://proxy.owned.invalid:8080");
    config_PutPsz("http-proxy-pwd", "");
    CHECK(matches("http://target.owned.invalid/", "http://proxy.owned.invalid:8080"), "explicit proxy preserved");
    CHECK(settings_calls == 0, "explicit proxy never queries Windows settings");
    config_PutPsz("http-proxy", "http://owned-user@proxy.owned.invalid:8080");
    config_PutPsz("http-proxy-pwd", "owned-password");
    CHECK(matches("http://target.owned.invalid/", "http://owned-user:owned-password@proxy.owned.invalid:8080"), "separate explicit password composed");
    config_PutPsz("http-proxy", "http://owned-user:embedded@proxy.owned.invalid:8080");
    CHECK(matches("http://target.owned.invalid/", "http://owned-user:embedded@proxy.owned.invalid:8080"), "embedded explicit password retained");
    CHECK(settings_calls == 0, "credential composition does not inspect Windows settings");

    config_PutPsz("http-proxy", "");
    owned_proxy = L"proxy.owned.invalid:8080";
    CHECK(matches("http://target.owned.invalid/", "http://proxy.owned.invalid:8080"), "empty explicit value uses owned Windows proxy");
    CHECK(settings_calls > 0, "Windows resolver was actually queried");
    CHECK(matches("https://target.owned.invalid/", "http://proxy.owned.invalid:8080"), "system proxy does not receive old explicit password");
    owned_proxy = L"https=secure.owned.invalid:8443;http=plain.owned.invalid:8080";
    CHECK(matches("http://target.owned.invalid/", "http://plain.owned.invalid:8080"), "HTTP selector chooses its own endpoint");
    CHECK(matches("https://target.owned.invalid/", "http://secure.owned.invalid:8443"), "HTTPS selector chooses its own endpoint");
    owned_proxy = L"fallback.owned.invalid:8000 https=https://secure.owned.invalid:8443";
    CHECK(matches("https://target.owned.invalid/", "https://secure.owned.invalid:8443"), "scheme-specific endpoint precedes generic fallback");
    CHECK(matches("http://target.owned.invalid/", "http://fallback.owned.invalid:8000"), "generic endpoint used without matching selector");
    owned_proxy = L"socks=socks.owned.invalid:1080";
    CHECK(matches("http://target.owned.invalid/", NULL), "unsupported proxy scheme is not treated as an HTTP endpoint");
    owned_proxy = L"http=proxy.owned.invalid:8080";
    owned_bypass = L"exact.owned.invalid; *.internal.owned.invalid\t<local>";
    CHECK(matches("http://EXACT.owned.invalid/", NULL), "exact bypass is case insensitive");
    CHECK(matches("http://a.internal.owned.invalid/", NULL), "wildcard bypass matches subdomain");
    CHECK(matches("http://a.internal.owned.invalid.evil.invalid/", "http://proxy.owned.invalid:8080"), "bypass does not match a hostname suffix attack");
    CHECK(matches("http://intranet/", NULL), "local bypass handles dotless intranet host");
    CHECK(matches("http://external.owned.invalid/", "http://proxy.owned.invalid:8080"), "local bypass does not bypass a dotted external host");
    CHECK(matches("http://[2001:db8::1]/", "http://proxy.owned.invalid:8080"), "local bypass does not bypass an IPv6 literal");
    CHECK(matches("http://2130706433/", "http://proxy.owned.invalid:8080"), "local bypass does not treat a numeric IP literal as an intranet name");
    owned_bypass = L"target.owned.invalid:8081; [::1]";
    CHECK(matches("http://target.owned.invalid:8081/", NULL), "port-specific bypass matches exact port");
    CHECK(matches("http://target.owned.invalid:8082/", "http://proxy.owned.invalid:8080"), "port-specific bypass preserves other ports");
    CHECK(matches("http://[::1]:8082/", NULL), "IPv6 bypass matches address without losing its brackets");
    owned_bypass = L"?.owned.invalid";
    CHECK(matches("http://a.owned.invalid/", NULL), "single-character bypass wildcard");
    CHECK(matches("http://ab.owned.invalid/", "http://proxy.owned.invalid:8080"), "single-character wildcard is bounded");
    owned_bypass = L"m\u00fcnchen.owned.invalid";
    CHECK(matches("http://xn--mnchen-3ya.owned.invalid/", NULL), "Unicode bypass matches an IDNA hostname");
    owned_bypass = L"M\u00dcNCHEN.owned.invalid:8081";
    CHECK(matches("http://xn--mnchen-3ya.owned.invalid:8081/", NULL), "Unicode case and port-specific bypass are preserved");
    owned_bypass = L"xn--mnchen-3ya.owned.invalid";
    CHECK(matches("http://xn--mnchen-3ya.owned.invalid/", NULL), "ASCII IDNA bypass remains supported");
    owned_bypass = NULL; owned_proxy = L"proxy.owned.invalid:70000";
    CHECK(matches("http://target.owned.invalid/", NULL), "out-of-range system proxy port is rejected");
    unsigned calls_before = settings_calls;
    config_PutPsz("http-proxy", "direct://");
    CHECK(matches("http://target.owned.invalid/", NULL), "explicit direct mode bypasses a configured system proxy");
    CHECK(settings_calls == calls_before, "explicit direct mode does not read system settings");
    config_PutPsz("http-proxy", "");
    CHECK(matches("file:///owned-media", NULL) && settings_calls == calls_before, "non-HTTP target does not query system proxy settings");
    owned_bypass = NULL; owned_proxy = NULL;
    CHECK(matches("http://target.owned.invalid/", NULL), "empty system configuration is direct");
    owned_available = false;
    CHECK(matches("http://target.owned.invalid/", NULL), "unavailable Windows settings safely return direct");
    owned_available = true;

    if (native && argc >= 2) {
        vlc_url_t endpoint;
        bool valid = vlc_UrlParse(&endpoint, argv[1]) == 0
            && endpoint.psz_host && !strcmp(endpoint.psz_host, "127.0.0.1")
            && endpoint.i_port > 0 && endpoint.i_port <= UINT16_MAX;
        unsigned port = endpoint.i_port;
        vlc_UrlClean(&endpoint);
        CHECK(valid, "actual proxy transport is restricted to the owned loopback server");
        wchar_t transport_proxy[64];
        swprintf(transport_proxy, ARRAY_SIZE(transport_proxy), L"127.0.0.1:%u", port);
        owned_proxy = transport_proxy;
        const unsigned before = settings_calls;
        stream_t *stream = vlc_stream_NewURL(VLC_OBJECT(instance->p_libvlc_int),
            "http://media.owned.invalid/owned-vlc-proxy-transport");
        CHECK(stream != NULL, "actual VLC HTTP input opens through the owned system proxy");
        char bytes[64] = {0};
        ssize_t length = vlc_stream_Read(stream, bytes, sizeof(bytes));
        vlc_stream_Delete(stream);
        static const char expected[] = "owned-vlc-proxy-transport\r\n";
        CHECK(length == sizeof(expected) - 1 && !memcmp(bytes, expected, sizeof(expected) - 1),
            "actual VLC HTTP input receives exact bytes from its proxy response");
        CHECK(settings_calls > before, "HTTP input invokes the actual Core resolver with owned settings");
        owned_proxy = NULL;
    }

    if (argc >= 2) {
        pac = ToWide(argv[1]);
        CHECK(pac != NULL, "owned loopback PAC URL converted");
        owned_pac = pac;
        CHECK(matches("https://target.owned.invalid/proxy", "http://pac.owned.invalid:18080"), "real WinHTTP evaluates owned PAC proxy result");
        CHECK(matches("https://direct.owned.invalid/direct", NULL), "real WinHTTP preserves owned HTTPS PAC DIRECT result");
        CHECK(matches("http://target.owned.invalid/proxy", "http://pac.owned.invalid:18080"), "real WinHTTP evaluates owned HTTP path proxy result");
        CHECK(matches("http://target.owned.invalid/direct", NULL), "real WinHTTP evaluates a different HTTP path without reusing the host result");
        owned_proxy = L"fallback.owned.invalid:8000";
        wchar_t *failed_pac = malloc((wcslen(pac) + 16) * sizeof(wchar_t));
        CHECK(failed_pac != NULL, "owned rejected-PAC URL allocated");
        swprintf(failed_pac, wcslen(pac) + 16, L"%ls/rejected", pac);
        owned_pac = failed_pac;
        bool fallback = matches("https://target.owned.invalid/proxy", "http://fallback.owned.invalid:8000");
        owned_pac = NULL; free(failed_pac);
        CHECK(fallback, "PAC authentication rejection falls back to the configured static endpoint");
        owned_pac = NULL;
    }
    result = 0;
out:
    free(pac);
    libvlc_release(instance);
    restore_owned_settings();
    return result;
}
