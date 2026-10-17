// SPDX-License-Identifier: LGPL-2.1-or-later
int borrowed_proxy_fixture_main(int, char **);
#define main borrowed_proxy_fixture_main
#include "proxy_windows.c"
#undef main

static ULONG_PTR *pac_slot, pac_original;
static ULONG_PTR *dns_slot, dns_original;
static const char *expected_media[2];
static unsigned pac_calls, dns_calls;
static bool complete_urls = true;
static BOOL WINAPI trace_pac(HINTERNET session, LPCWSTR url,
    WINHTTP_AUTOPROXY_OPTIONS *options, WINHTTP_PROXY_INFO *info)
{
    BOOL (WINAPI *real)(HINTERNET, LPCWSTR, WINHTTP_AUTOPROXY_OPTIONS *, WINHTTP_PROXY_INFO *) = (void *)pac_original;
    BOOL result = real(session, url, options, info);
    DWORD error = GetLastError();
    char *text = FromWide(url), *proxy = FromWide(info->lpszProxy);
    complete_urls &= text && pac_calls < 2 && !strcmp(text, expected_media[pac_calls]);
    pac_calls++;
    printf("# PAC url=%s result=%u error=%lu type=%lu proxy=%s\n", text,
        (unsigned)result, (unsigned long)error, (unsigned long)info->dwAccessType, proxy ? proxy : "(null)");
    free(text); free(proxy); SetLastError(error);
    return result;
}
static int WSAAPI owned_dns(const char *node, const char *service,
    const struct addrinfo *hints, struct addrinfo **result)
{
    int (WSAAPI *real)(const char *, const char *, const struct addrinfo *, struct addrinfo **) = (void *)dns_original;
    if (node && !strcmp(node, "media.owned.invalid")) {
        dns_calls++; node = "127.0.0.1";
    } else if (!node || (strcmp(node, "127.0.0.1") && strcmp(node, "::1"))) return EAI_NONAME;
    return real(node, service, hints, result);
}
static bool install_owned_dns(void)
{
    BYTE *base = (BYTE *)GetModuleHandleW(L"libvlccore.dll");
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)base;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + dos->e_lfanew);
    DWORD rva = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    IMAGE_IMPORT_DESCRIPTOR *dll = (IMAGE_IMPORT_DESCRIPTOR *)(base + rva);
    for (; dll->Name; dll++) {
        if (strcasecmp((char *)base + dll->Name, "ws2_32.dll") || !dll->OriginalFirstThunk) continue;
        IMAGE_THUNK_DATA *name = (IMAGE_THUNK_DATA *)(base + dll->OriginalFirstThunk);
        IMAGE_THUNK_DATA *address = (IMAGE_THUNK_DATA *)(base + dll->FirstThunk);
        for (; name->u1.AddressOfData; name++, address++) {
            if (IMAGE_SNAP_BY_ORDINAL(name->u1.Ordinal)) continue;
            IMAGE_IMPORT_BY_NAME *symbol = (IMAGE_IMPORT_BY_NAME *)(base + name->u1.AddressOfData);
            if (strcmp((char *)symbol->Name, "getaddrinfo")) continue;
            dns_slot = &address->u1.Function;
            DWORD previous, ignored;
            if (!VirtualProtect(dns_slot, sizeof(*dns_slot), PAGE_READWRITE, &previous)) return false;
            dns_original = *dns_slot;
            InterlockedExchangePointer((void *volatile *)dns_slot, (void *)owned_dns);
            VirtualProtect(dns_slot, sizeof(*dns_slot), previous, &ignored);
            return true;
        }
    }
    return false;
}
static bool install_pac_trace(void)
{
    BYTE *base = (BYTE *)GetModuleHandleW(L"libvlccore.dll");
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)base;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + dos->e_lfanew);
    DWORD rva = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    IMAGE_IMPORT_DESCRIPTOR *dll = (IMAGE_IMPORT_DESCRIPTOR *)(base + rva);
    for (; dll->Name; dll++) {
        if (strcasecmp((char *)base + dll->Name, "winhttp.dll") || !dll->OriginalFirstThunk) continue;
        IMAGE_THUNK_DATA *name = (IMAGE_THUNK_DATA *)(base + dll->OriginalFirstThunk);
        IMAGE_THUNK_DATA *address = (IMAGE_THUNK_DATA *)(base + dll->FirstThunk);
        for (; name->u1.AddressOfData; name++, address++) {
            if (IMAGE_SNAP_BY_ORDINAL(name->u1.Ordinal)) continue;
            IMAGE_IMPORT_BY_NAME *symbol = (IMAGE_IMPORT_BY_NAME *)(base + name->u1.AddressOfData);
            if (strcmp((char *)symbol->Name, "WinHttpGetProxyForUrl")) continue;
            pac_slot = &address->u1.Function;
            DWORD previous, ignored;
            if (!VirtualProtect(pac_slot, sizeof(*pac_slot), PAGE_READWRITE, &previous)) return false;
            pac_original = *pac_slot;
            InterlockedExchangePointer((void *volatile *)pac_slot, (void *)trace_pac);
            VirtualProtect(pac_slot, sizeof(*pac_slot), previous, &ignored);
            return true;
        }
    }
    return false;
}

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    unsigned count = 0; int result = 1;
    libvlc_instance_t *instance = NULL;
    wchar_t *pac = NULL;
    CHECK(install_owned_settings() && install_pac_trace() && install_owned_dns(), "actual Core proxy settings and DNS boundaries are owned");
    bool valid = argc == 4;
    for (int i = 1; valid && i < argc; i++) {
        vlc_url_t url;
        valid = vlc_UrlParse(&url, argv[i]) == 0 && url.psz_host
            && !strcmp(url.psz_host, i == 1 ? "127.0.0.1" : "media.owned.invalid")
            && url.psz_protocol && !strcmp(url.psz_protocol, "http")
            && url.i_port > 0 && url.i_port <= UINT16_MAX;
        vlc_UrlClean(&url);
    }
    CHECK(valid, "PAC and media transport are restricted to owned loopback servers");
    expected_media[0] = argv[2]; expected_media[1] = argv[3];
    const char *options[] = {"--ignore-config", "--quiet", "--no-media-library",
        "--no-metadata-network-access", "--no-auto-preparse", "--extraintf=", "--control="};
    instance = libvlc_new(ARRAY_SIZE(options), options);
    CHECK(instance != NULL, "actual native instance initializes");
    pac = ToWide(argv[1]);
    CHECK(pac != NULL, "owned PAC URL converted");
    owned_pac = pac;
    config_PutPsz("http-proxy", "");
    config_PutPsz("http-proxy-pwd", "must-not-reach-system-proxy");
    for (unsigned i = 0; i < 2; i++) {
        stream_t *stream = vlc_stream_NewURL(VLC_OBJECT(instance->p_libvlc_int), argv[i + 2]);
        CHECK(stream != NULL, "actual HTTP media opens after a path-specific PAC lookup");
        char bytes[64] = {0};
        ssize_t length = vlc_stream_Read(stream, bytes, sizeof(bytes));
        vlc_stream_Delete(stream);
        const char *expected = i ? "origin-payload\n" : "proxy-no-auth\n";
        CHECK(length == (ssize_t)strlen(expected) && !memcmp(bytes, expected, length),
              "actual HTTP routing preserves the complete path and query");
    }
    CHECK(settings_calls >= 2 && pac_calls == 2 && complete_urls && dns_calls > 0,
          "both actual inputs consult Windows with complete URLs and owned DNS");
    result = 0;
out:
    if (instance) libvlc_release(instance);
    if (dns_slot && dns_original) {
        DWORD previous, ignored;
        if (VirtualProtect(dns_slot, sizeof(*dns_slot), PAGE_READWRITE, &previous)) {
            InterlockedExchangePointer((void *volatile *)dns_slot, (void *)dns_original);
            VirtualProtect(dns_slot, sizeof(*dns_slot), previous, &ignored);
        }
    }
    if (pac_slot && pac_original) {
        DWORD previous, ignored;
        if (VirtualProtect(pac_slot, sizeof(*pac_slot), PAGE_READWRITE, &previous)) {
            InterlockedExchangePointer((void *volatile *)pac_slot, (void *)pac_original);
            VirtualProtect(pac_slot, sizeof(*pac_slot), previous, &ignored);
        }
    }
    free(pac); restore_owned_settings();
    return result;
}
