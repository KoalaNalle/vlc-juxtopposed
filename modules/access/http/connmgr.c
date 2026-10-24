/*****************************************************************************
 * connmgr.c: HTTP/TLS VLC connection manager
 *****************************************************************************
 * Copyright © 2015 Rémi Denis-Courmont
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

#include <assert.h>
#include <vlc_common.h>
#include <vlc_network.h>
#include <vlc_tls.h>
#include <vlc_url.h>
#include "transport.h"
#include "conn.h"
#include "connmgr.h"
#include "message.h"

#pragma GCC visibility push(default)

void vlc_http_err(void *ctx, const char *fmt, ...)
{
    struct vlc_logger *logger = ctx;
    va_list ap;

    va_start(ap, fmt);
    vlc_vaLog(&logger, VLC_MSG_ERR, "generic", "http",
              __FILE__, __LINE__, __func__, fmt, ap);
    va_end(ap);
}

void vlc_http_dbg(void *ctx, const char *fmt, ...)
{
    struct vlc_logger *logger = ctx;
    va_list ap;

    va_start(ap, fmt);
    vlc_vaLog(&logger, VLC_MSG_DBG, "generic", "http",
              __FILE__, __LINE__, __func__, fmt, ap);
    va_end(ap);
}

vlc_tls_t *vlc_https_connect(vlc_tls_client_t *creds, const char *name,
                             unsigned port, bool *restrict two)
{
    if (port == 0)
        port = 443;

    /* TLS with ALPN */
    const char *alpn[] = { "h2", "http/1.1", NULL };
    char *alp;

    vlc_tls_t *tls = vlc_tls_SocketOpenTLS(creds, name, port, "https",
                                           alpn + !*two, &alp);
    if (tls != NULL)
    {
        *two = (alp != NULL) && !strcmp(alp, "h2");
        free(alp);
    }
    return tls;
}

static char *vlc_http_request_url(const char *host, unsigned port, bool secure,
                                   const struct vlc_http_msg *request)
{
    char *authority = vlc_http_authority(host, port), *url = NULL;
    const char *path = vlc_http_msg_get_path(request);
    if (authority == NULL) return NULL;
    if (path == NULL || *path != '/') path = "/";
    if (asprintf(&url, "%s://%s%s", secure ? "https" : "http", authority, path) < 0)
        url = NULL;
    free(authority);
    return url;
}


struct vlc_http_mgr
{
    struct vlc_logger *logger;
    vlc_object_t *obj;
    vlc_tls_client_t *creds;
    struct vlc_http_cookie_jar_t *jar;
    struct vlc_http_conn *conn;
    char *uri;
    char *proxy;
};

static struct vlc_http_conn *vlc_http_mgr_find(struct vlc_http_mgr *mgr,
                                               const char *host, unsigned port)
{
    (void) host; (void) port;
    return mgr->conn;
}

static void vlc_http_mgr_release(struct vlc_http_mgr *mgr,
                                 struct vlc_http_conn *conn)
{
    assert(mgr->conn == conn);
    mgr->conn = NULL;

    vlc_http_conn_release(conn);
}

static
struct vlc_http_msg *vlc_http_mgr_reuse(struct vlc_http_mgr *mgr,
                                        const char *host, unsigned port,
                                        const struct vlc_http_msg *req,
                                        bool payload)
{
    struct vlc_http_conn *conn = vlc_http_mgr_find(mgr, host, port);
    if (conn == NULL)
        return NULL;

    struct vlc_http_stream *stream = vlc_http_stream_open(conn, req, payload);
    if (stream != NULL)
    {
        struct vlc_http_msg *m = vlc_http_msg_get_initial(stream);
        if (m != NULL)
            return m;
    }
    /* Get rid of closing or reset connection */
    vlc_http_mgr_release(mgr, conn);
    return NULL;
}

static struct vlc_http_msg *vlc_https_request(struct vlc_http_mgr *mgr,
                                              const char *host, unsigned port,
                                              const struct vlc_http_msg *req,
                                              bool idempotent, bool payload)
{
    vlc_tls_t *tls;
    bool http2 = true;

    if (mgr->creds == NULL)
    {   /* First TLS connection: load x509 credentials */
        mgr->creds = vlc_tls_ClientCreate(mgr->obj);
        if (mgr->creds == NULL)
            return NULL;
    }

    if (idempotent)
    {   /* If the request is idempotent, try to reuse an existing connection.
         * Otherwise, it is possible but unadvisable as we would not know if
         * the nonidempotent request was processed if the connection fails
         * before the response is received.
         */
        struct vlc_http_msg *resp = vlc_http_mgr_reuse(mgr, host, port, req,
                                                       payload);
        if (resp != NULL)
            return resp; /* existing connection reused */
    }

    const char *proxy = mgr->proxy;
    if (proxy != NULL)
    {
        tls = vlc_https_connect_proxy(mgr->logger, mgr->creds,
                                      host, port, &http2, proxy);
    }
    else
        tls = vlc_https_connect(mgr->creds, host, port, &http2);

    if (tls == NULL)
        return NULL;

    struct vlc_http_conn *conn;

    /* For HTTPS, TLS-ALPN determines whether HTTP version 2.0 ("h2") or 1.1
     * ("http/1.1") is used.
     * NOTE: If the negotiated protocol is explicitly "http/1.1", HTTP 1.0
     * should not be used. HTTP 1.0 should only be used if ALPN is not
     * supported by the server.
     * NOTE: We do not enforce TLS version 1.2 for HTTP 2.0 explicitly.
     */
    if (http2)
        conn = vlc_h2_conn_create(mgr->logger, tls);
    else
        conn = vlc_h1_conn_create(mgr->logger, tls, false);

    if (unlikely(conn == NULL))
    {
        vlc_tls_Close(tls);
        return NULL;
    }

    if (mgr->conn != NULL)
        vlc_http_mgr_release(mgr, mgr->conn);

    mgr->conn = conn;
    return vlc_http_mgr_reuse(mgr, host, port, req, payload);
}

static struct vlc_http_msg *vlc_http_request(struct vlc_http_mgr *mgr,
                                             const char *host, unsigned port,
                                             const struct vlc_http_msg *req,
                                             bool idempotent, bool payload)
{
    if (idempotent)
    {
        struct vlc_http_msg *resp = vlc_http_mgr_reuse(mgr, host, port, req,
                                                       payload);
        if (resp != NULL)
            return resp;
    }

    struct vlc_http_conn *conn;
    struct vlc_http_stream *stream;

    const char *proxy = mgr->proxy;
    if (proxy != NULL)
    {
        vlc_url_t endpoint;
        bool valid = vlc_UrlParse(&endpoint, proxy) == 0
            && endpoint.psz_host && endpoint.psz_protocol
            && (!strcasecmp(endpoint.psz_protocol, "http") || !strcasecmp(endpoint.psz_protocol, "https"));
        stream = NULL;
        if (valid && !strcasecmp(endpoint.psz_protocol, "https")) {
            if (mgr->creds == NULL) mgr->creds = vlc_tls_ClientCreate(mgr->obj);
            bool http2 = false; /* The forward proxy request uses HTTP/1.1. */
            vlc_tls_t *tls = mgr->creds ? vlc_https_connect(mgr->creds, endpoint.psz_host,
                endpoint.i_port ? endpoint.i_port : 443, &http2) : NULL;
            if (tls != NULL) {
                conn = vlc_h1_conn_create(mgr->logger, tls, true);
                if (conn == NULL) vlc_tls_Close(tls);
                else {
                    stream = vlc_http_stream_open(conn, req, payload);
                    if (stream == NULL) vlc_http_conn_release(conn);
                }
            }
        } else if (valid) {
            stream = vlc_h1_request(mgr->logger, endpoint.psz_host,
                endpoint.i_port ? endpoint.i_port : 80, true, req,
                idempotent, payload, &conn);
        }
        vlc_UrlClean(&endpoint);
    }
    else
        stream = vlc_h1_request(mgr->logger, host, port ? port : 80, false,
                                req, idempotent, payload, &conn);

    if (stream == NULL)
        return NULL;

    struct vlc_http_msg *resp = vlc_http_msg_get_initial(stream);
    if (resp == NULL)
    {
        vlc_http_conn_release(conn);
        return NULL;
    }

    if (mgr->conn != NULL)
        vlc_http_mgr_release(mgr, mgr->conn);

    mgr->conn = conn;
    return resp;
}

struct vlc_http_msg *vlc_http_mgr_request(struct vlc_http_mgr *mgr, bool https,
                                          const char *host, unsigned port,
                                          const struct vlc_http_msg *m,
                                          bool idempotent, bool payload)
{
    if (port && vlc_http_port_blocked(port))
        return NULL;

    char *uri = vlc_http_request_url(host, port, https, m);
    if (uri == NULL) return NULL;
    if (!idempotent || mgr->conn == NULL || mgr->uri == NULL || strcmp(mgr->uri, uri)) {
        /* PAC sees the complete request path/query. Never reuse a connection to
         * a different origin or a route chosen for a different request URL. */
        char *proxy = vlc_getProxyUrlForObject(mgr->obj, uri);
        if (mgr->conn != NULL) vlc_http_mgr_release(mgr, mgr->conn);
        free(mgr->uri); free(mgr->proxy);
        mgr->uri = uri; mgr->proxy = proxy;
    } else free(uri);

    struct vlc_http_msg *request = vlc_http_req_copy(m);
    if (request == NULL) return NULL;
    if (!https && mgr->proxy != NULL) {
        vlc_url_t endpoint;
        bool valid = vlc_UrlParse(&endpoint, mgr->proxy) == 0;
        if (valid && endpoint.psz_username != NULL)
            valid = vlc_http_msg_add_creds_basic(request, true, endpoint.psz_username,
                endpoint.psz_password ? endpoint.psz_password : "") == 0;
        vlc_UrlClean(&endpoint);
        if (!valid) { vlc_http_msg_destroy(request); return NULL; }
    }
    struct vlc_http_msg *response = (https ? vlc_https_request : vlc_http_request)(
        mgr, host, port, request, idempotent, payload);
    vlc_http_msg_destroy(request);
    return response;
}

struct vlc_http_cookie_jar_t *vlc_http_mgr_get_jar(struct vlc_http_mgr *mgr)
{
    return mgr->jar;
}

struct vlc_http_mgr *vlc_http_mgr_create(vlc_object_t *obj,
                                         struct vlc_http_cookie_jar_t *jar)
{
    struct vlc_http_mgr *mgr = malloc(sizeof (*mgr));
    if (unlikely(mgr == NULL))
        return NULL;

    mgr->logger = obj->logger;
    mgr->obj = obj;
    mgr->creds = NULL;
    mgr->jar = jar;
    mgr->conn = NULL;
    mgr->uri = NULL;
    mgr->proxy = NULL;
    return mgr;
}

void vlc_http_mgr_destroy(struct vlc_http_mgr *mgr)
{
    if (mgr->conn != NULL)
        vlc_http_mgr_release(mgr, mgr->conn);
    if (mgr->creds != NULL)
        vlc_tls_ClientDelete(mgr->creds);
    free(mgr->uri);
    free(mgr->proxy);
    free(mgr);
}
