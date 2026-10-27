/* SPDX-License-Identifier: LGPL-2.1-or-later */
#ifndef VLC_NETWORK_PROXY_INTERNAL_H
#define VLC_NETWORK_PROXY_INTERNAL_H
/* Windows platform policy only; explicit VLC settings are handled separately. */
char *vlc_getSystemProxyUrl(const char *url);
#endif
