/* SPDX-License-Identifier: LGPL-2.1-or-later */
#ifndef VLC_HTTP_TEST_TRANSPORT_H
#define VLC_HTTP_TEST_TRANSPORT_H
#include <vlc_network.h>
#include <vlc_tls.h>
#include <vlc_poll.h>

#ifdef _WIN32
#include <afunix.h>
static inline void vlc_http_test_network_cleanup(void) { WSACleanup(); }
static inline int vlc_http_test_network_init(void)
{
    static bool initialized;
    if (!initialized) {
        WSADATA data;
        if (WSAStartup(MAKEWORD(2, 2), &data)) return -1;
        atexit(vlc_http_test_network_cleanup);
        initialized = true;
    }
    return 0;
}
static inline int vlc_http_test_socket_pair(vlc_tls_t *pair[2])
{
    /* Keep the Unix stream/shutdown contract of these protocol tests. Windows
     * supports AF_UNIX, but Core does not provide a socketpair constructor. */
    if (vlc_http_test_network_init()) return -1;
    SOCKET listener = INVALID_SOCKET, first = INVALID_SOCKET, second = INVALID_SOCKET;
    SOCKADDR_UN address = { .sun_family = AF_UNIX };
    char directory[UNIX_PATH_MAX];
    DWORD size = GetTempPathA(sizeof(directory), directory);
    static unsigned serial;
    if (!size || size >= sizeof(directory)) return -1;
    int length = snprintf(address.sun_path, sizeof(address.sun_path), "%svlc-http-%lu-%lu-%u.sock",
        directory, (unsigned long)GetCurrentProcessId(), (unsigned long)GetTickCount(), serial++);
    if (length < 0 || (size_t)length >= sizeof(address.sun_path)) return -1;
    bool bound = false;
    listener = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listener == INVALID_SOCKET) return -1;
    if (bind(listener, (struct sockaddr *)&address, sizeof(address))) goto error;
    bound = true;
    if (listen(listener, 1)) goto error;
    first = socket(AF_UNIX, SOCK_STREAM, 0);
    if (first == INVALID_SOCKET || connect(first, (struct sockaddr *)&address, sizeof(address))) goto error;
    second = accept(listener, NULL, NULL);
    if (second == INVALID_SOCKET) goto error;
    closesocket(listener); listener = INVALID_SOCKET;
    DeleteFileA(address.sun_path); bound = false;
    for (unsigned i = 0; i < 2; i++) {
        SOCKET socket = i ? second : first;
        unsigned long nonblocking = 1;
        if (ioctlsocket(socket, FIONBIO, &nonblocking)) goto error;
    }
    pair[0] = vlc_tls_SocketOpen((int)first);
    pair[1] = vlc_tls_SocketOpen((int)second);
    if (!pair[0] || !pair[1]) {
        if (pair[0]) vlc_tls_SessionDelete(pair[0]); else closesocket(first);
        if (pair[1]) vlc_tls_SessionDelete(pair[1]); else closesocket(second);
        return -1;
    }
    return 0;
error:
    if (listener != INVALID_SOCKET) closesocket(listener);
    if (first != INVALID_SOCKET) closesocket(first);
    if (second != INVALID_SOCKET) closesocket(second);
    if (bound) DeleteFileA(address.sun_path);
    return -1;
}

#else
static inline int vlc_http_test_network_init(void) { return 0; }
static inline int vlc_http_test_socket_pair(vlc_tls_t *pair[2])
{ return vlc_tls_SocketPair(AF_UNIX, 0, pair); }
#endif
#endif
