/* SPDX-License-Identifier: LGPL-2.1-or-later */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include <vlc_configuration.h>
#include <vlc_plugin.h>
#include <vlc_network.h>
#include <vlc_url.h>
#include "config/configuration.h"
#include "proxy.h"

static bool runtime_string(vlc_object_t *obj, const char *name,
                           char **text, vlc_object_t **owner)
{
    for (; obj != NULL; obj = vlc_object_parent(obj)) {
        vlc_value_t value;
        if (var_GetChecked(obj, name, VLC_VAR_STRING, &value) == VLC_SUCCESS) {
            *text = value.psz_string;
            *owner = obj;
            return true;
        }
    }
    *owner = NULL;
    return false;
}

static bool same_authority(const char *left, const char *right)
{
    if (!left || !right) return false;
    vlc_url_t a, b;
    int ar = vlc_UrlParse(&a, left), br = vlc_UrlParse(&b, right);
    unsigned ap = a.i_port ? a.i_port : a.psz_protocol && !strcasecmp(a.psz_protocol, "https") ? 443 : 80;
    unsigned bp = b.i_port ? b.i_port : b.psz_protocol && !strcasecmp(b.psz_protocol, "https") ? 443 : 80;
    bool same = ar == 0 && br == 0 && a.psz_protocol && b.psz_protocol
        && a.psz_host && b.psz_host && !strcasecmp(a.psz_protocol, b.psz_protocol)
        && !strcasecmp(a.psz_host, b.psz_host) && ap == bp
        && !strcmp(a.psz_username ? a.psz_username : "", b.psz_username ? b.psz_username : "");
    vlc_UrlClean(&a); vlc_UrlClean(&b);
    return same;
}

char *vlc_getProxyUrlForObject(vlc_object_t *obj, const char *url)
{
    /* Read saved endpoint/secret together. The checked preferences writer clears
     * credentials before replacing their authority, including on rollback. */
    config_Lock();
    char *saved_proxy = config_GetPsz("http-proxy");
    char *saved_password = config_GetPsz("http-proxy-pwd");
    config_Unlock();
    char *proxy = NULL, *password = NULL;
    vlc_object_t *proxy_owner, *password_owner;
    if (!runtime_string(obj, "http-proxy", &proxy, &proxy_owner)) {
        proxy = saved_proxy; saved_proxy = NULL;
    }
    if (proxy == NULL || *proxy == '\0') {
        free(proxy); free(saved_proxy); free(saved_password);
#ifdef _WIN32
        return vlc_getSystemProxyUrl(url);
#else
        return vlc_getProxyUrl(url);
#endif
    }
    if (!strcasecmp(proxy, "direct://")) {
        free(proxy); free(saved_proxy); free(saved_password);
        return NULL;
    }

    bool runtime_password = runtime_string(obj, "http-proxy-pwd", &password, &password_owner);
    if (!runtime_password) {
        password = saved_password; saved_password = NULL;
    }
    /* A password inherited from an outer scope belongs to that scope's proxy,
     * not an unrelated per-media/CLI endpoint. Explicit embedded credentials
     * remain attached to their own URL. */
    char *scope_proxy = NULL;
    vlc_object_t *scope_owner;
    bool runtime_scope = runtime_password
        && runtime_string(password_owner, "http-proxy", &scope_proxy, &scope_owner);
    if (!runtime_scope) {
        scope_proxy = saved_proxy;
        saved_proxy = NULL;
        if (!scope_proxy && !proxy_owner) scope_proxy = strdup(proxy);
    }
    if (!same_authority(proxy, scope_proxy)) { free(password); password = NULL; }
    free(scope_proxy); free(saved_proxy); free(saved_password);

    vlc_url_t parsed;
    if (vlc_UrlParse(&parsed, proxy) < 0) {
        vlc_UrlClean(&parsed); free(proxy); free(password); return NULL;
    }
    if (parsed.psz_password == NULL) parsed.psz_password = password;
    char *result = vlc_uri_compose(&parsed);
    vlc_UrlClean(&parsed); free(proxy); free(password);
    return result;
}
