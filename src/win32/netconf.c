/*****************************************************************************
 * netconf.c : Network configuration
 *****************************************************************************
 * Copyright (C) 2001-2008 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <string.h>

#include <vlc_common.h>
#include <vlc_configuration.h>
#include <vlc_network.h>
#include <vlc_url.h>
#include <vlc_charset.h>
#ifndef VLC_WINSTORE_APP
# include <windows.h>
# include <winhttp.h>

/* Anchored, case-insensitive Windows bypass patterns. Keep wildcard matching
 * iterative so a long pattern does not cause recursive backtracking. */
static bool bypass_match(const wchar_t *pattern, const wchar_t *host)
{
    const wchar_t *star = NULL, *retry = NULL;
    while (*host) {
        if (*pattern == L'?' || CompareStringOrdinal(pattern, 1, host, 1, TRUE) == CSTR_EQUAL) {
            pattern++; host++;
        } else if (*pattern == L'*') {
            star = pattern++; retry = host;
        } else if (star) {
            pattern = star + 1; host = ++retry;
        } else return false;
    }
    while (*pattern == L'*') pattern++;
    return *pattern == L'\0';
}

static bool proxy_bypassed(const vlc_url_t *target, const wchar_t *wide)
{
    if (!wide) return false;
    wchar_t *list = _wcsdup(wide);
    if (!list) return false;
    wchar_t *hosts[2] = {ToWide(target->psz_host), NULL};
    if (hosts[0]) {
        int size = IdnToUnicode(0, hosts[0], -1, NULL, 0);
        if (size > 0) {
            hosts[1] = malloc(size * sizeof(wchar_t));
            if (hosts[1] && !IdnToUnicode(0, hosts[0], -1, hosts[1], size)) {
                free(hosts[1]); hosts[1] = NULL;
            }
        }
    }
    unsigned port = target->i_port ? target->i_port
        : !strcasecmp(target->psz_protocol, "https") ? 443 : 80;
    bool bypass = false;
    for (wchar_t *cursor = list; *cursor && !bypass;) {
        cursor += wcsspn(cursor, L"; \t\r\n");
        wchar_t *item = cursor;
        cursor += wcscspn(cursor, L"; \t\r\n");
        if (*cursor) *cursor++ = L'\0';
        if (!*item) break;
        if (!_wcsicmp(item, L"<local>") && !strchr(target->psz_host, '.')
            && !strchr(target->psz_host, ':')
            && strspn(target->psz_host, "0123456789") != strlen(target->psz_host)) {
            bypass = true; break;
        }
        for (unsigned i = 0; i < ARRAY_SIZE(hosts) && !bypass; i++) {
            wchar_t *host = hosts[i];
            if (!host) continue;
            size_t size = wcslen(host) + 32;
            wchar_t *authority = malloc(size * sizeof(wchar_t));
            if (bypass_match(item, host)) bypass = true;
            if (authority) {
                const bool ipv6 = wcschr(host, L':') != NULL;
                swprintf(authority, size, ipv6 ? L"[%ls]" : L"%ls", host);
                if (bypass_match(item, authority)) bypass = true;
                swprintf(authority, size, ipv6 ? L"[%ls]:%u" : L"%ls:%u", host, port);
                if (bypass_match(item, authority)) bypass = true;
                free(authority);
            }
        }
    }
    free(hosts[1]); free(hosts[0]); free(list);
    return bypass;
}

static char *proxy_endpoint(const char *entry)
{
    char *value = NULL;
    if (strstr(entry, "://")) value = strdup(entry);
    else if (asprintf(&value, "http://%s", entry) < 0) return NULL;
    if (!value) return NULL;
    vlc_url_t endpoint;
    bool valid = vlc_UrlParse(&endpoint, value) == 0
        && endpoint.psz_host && *endpoint.psz_host && endpoint.psz_protocol
        && endpoint.i_port <= UINT16_MAX
        && (!strcasecmp(endpoint.psz_protocol, "http") || !strcasecmp(endpoint.psz_protocol, "https"))
        && (!endpoint.psz_path || !*endpoint.psz_path || !strcmp(endpoint.psz_path, "/"))
        && !endpoint.psz_option;
    vlc_UrlClean(&endpoint);
    if (!valid) { free(value); return NULL; }
    return value;
}

static char *proxy_select(const vlc_url_t *target, const wchar_t *wide,
                          const wchar_t *bypass)
{
    if (!wide || proxy_bypassed(target, bypass)) return NULL;
    char *list = FromWide(wide);
    if (!list) return NULL;
    char *fallback = NULL, *selected = NULL;
    for (char *state, *item = strtok_r(list, "; \t\r\n", &state); item;
         item = strtok_r(NULL, "; \t\r\n", &state)) {
        char *equal = strchr(item, '=');
        if (equal) {
            *equal = '\0';
            if (!strcasecmp(item, target->psz_protocol)) {
                selected = proxy_endpoint(equal + 1);
                if (selected) break;
            }
        } else if (!fallback) fallback = proxy_endpoint(item);
    }
    free(list);
    if (selected) { free(fallback); return selected; }
    return fallback;
}

static char *system_proxy(const char *url)
{
    vlc_url_t target;
    if (vlc_UrlParse(&target, url) < 0) { vlc_UrlClean(&target); return NULL; }
    if (!target.psz_host || !target.psz_protocol || target.i_port > UINT16_MAX
        || (strcasecmp(target.psz_protocol, "http") && strcasecmp(target.psz_protocol, "https"))) {
        vlc_UrlClean(&target); return NULL;
    }
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG config = {0};
    char *proxy = NULL;
    if (!WinHttpGetIEProxyConfigForCurrentUser(&config)) goto out;
    if (config.lpszAutoConfigUrl || config.fAutoDetect) {
        wchar_t *wide_url = ToWide(url);
        HINTERNET session = WinHttpOpen(L"VLC system proxy", WINHTTP_ACCESS_TYPE_NO_PROXY,
            WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        bool resolved = false;
        if (session && wide_url) {
            WinHttpSetTimeouts(session, 5000, 5000, 5000, 5000);
            WINHTTP_AUTOPROXY_OPTIONS options = {0};
            if (config.lpszAutoConfigUrl) {
                options.dwFlags = WINHTTP_AUTOPROXY_CONFIG_URL;
                options.lpszAutoConfigUrl = config.lpszAutoConfigUrl;
            } else {
                options.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;
                options.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP | WINHTTP_AUTO_DETECT_TYPE_DNS_A;
            }
            // PAC may choose different routes for different paths on one host.
            // Re-evaluate the result per URL while retaining WinHTTP's script cache.
            options.dwFlags |= WINHTTP_AUTOPROXY_NO_CACHE_CLIENT | WINHTTP_AUTOPROXY_NO_CACHE_SVC;
            // Never send the user's domain credentials to a PAC endpoint.
            options.fAutoLogonIfChallenged = FALSE;
            WINHTTP_PROXY_INFO info = {0};
            resolved = WinHttpGetProxyForUrl(session, wide_url, &options, &info);
            if (resolved && info.dwAccessType == WINHTTP_ACCESS_TYPE_NAMED_PROXY)
                proxy = proxy_select(&target, info.lpszProxy, info.lpszProxyBypass);
            GlobalFree(info.lpszProxy); GlobalFree(info.lpszProxyBypass);
        }
        if (session) WinHttpCloseHandle(session);
        free(wide_url);
        // DIRECT from a successfully evaluated PAC is authoritative. A failed
        // PAC falls back to the configured static proxy, including its bypasses.
        if (resolved) goto out;
    }
    proxy = proxy_select(&target, config.lpszProxy, config.lpszProxyBypass);
out:
    GlobalFree(config.lpszAutoConfigUrl); GlobalFree(config.lpszProxy); GlobalFree(config.lpszProxyBypass);
    vlc_UrlClean(&target);
    return proxy;
}
#endif

char *vlc_getProxyUrl(const char *psz_url)
{
    char *proxy = config_GetPsz( "http-proxy" );
    if (proxy && !strcasecmp(proxy, "direct://")) { free(proxy); return NULL; }
    if (proxy == NULL)
#ifndef VLC_WINSTORE_APP
        return system_proxy(psz_url);
#else
    {
        VLC_UNUSED(psz_url);
        return NULL;
    }
#endif

    char *proxy_pwd = config_GetPsz( "http-proxy-pwd" );
    if (proxy_pwd == NULL)
        return proxy;

    vlc_url_t url;
    if (vlc_UrlParse(&url, proxy) < 0) {
        vlc_UrlClean(&url);
        free (proxy);
        free (proxy_pwd);
        return NULL;
    }

    if (url.psz_password == NULL )
        url.psz_password = proxy_pwd;

    char *proxy_url = vlc_uri_compose (&url);
    vlc_UrlClean (&url);

    free (proxy_pwd);
    free (proxy);

    return proxy_url;
}
