/* SPDX-License-Identifier: LGPL-2.1-or-later
 * Source-level fault regression for the extracted actual sender helper.
 * It cannot establish Windows delivery, mutex lifetime or real process exit.
 */
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
typedef uintptr_t DWORD_PTR;
typedef intptr_t LPARAM;
typedef intptr_t LRESULT;
typedef unsigned UINT;
typedef uintptr_t HWND;
typedef struct { int unused; } libvlc_int_t;
typedef struct { uintptr_t dwData; uint32_t cbData; void *lpData; } COPYDATASTRUCT;
#define SMTO_BLOCK 1
#define SMTO_ABORTIFHUNG 2
#define SMTO_ERRORONEXIT 32
#define WM_COPYDATA 74
static int query_calls, sends, errors, registered;
static int query_ok, send_ok;
static DWORD_PTR version_reply, open_reply;
static const COPYDATASTRUCT *expected_message;
static char error[512];
static UINT RegisterWindowMessageW(const wchar_t *name)
{ assert(!wcscmp(name, L"VLC.LibraryOpen.v1")); return registered ? 0xc123 : 0; }
static LRESULT SendMessageTimeoutW(HWND window, UINT message, uintptr_t word,
                                  LPARAM data, UINT flags, UINT timeout,
                                  DWORD_PTR *reply)
{
    assert(window == 123 && !word && timeout == 5000);
    assert(flags == (SMTO_BLOCK | SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT));
    if (message == 0xc123) {
        ++query_calls; assert(!data); *reply = version_reply; return query_ok;
    }
    assert(message == WM_COPYDATA && data == (LPARAM)expected_message);
    ++sends; *reply = open_reply; return send_ok;
}
static void msg_Err(libvlc_int_t *instance, const char *format, ...)
{
    (void)instance; ++errors;
    va_list ap; va_start(ap, format); vsnprintf(error, sizeof(error), format, ap); va_end(ap);
}

#include "sender-under-test.inc"

static void reset(void)
{
    query_calls = sends = errors = 0; error[0] = '\0';
    registered = query_ok = send_ok = 1;
    version_reply = LIBRARY_IPC_VERSION; open_reply = LIBRARY_IPC_ACCEPTED;
}
int main(void)
{
    libvlc_int_t instance = {0}; COPYDATASTRUCT message = {0}; expected_message = &message;
    reset(); registered = 0;
    assert(LibrarySendExplicit(&instance, 123, &message) == EXIT_FAILURE);
    assert(!query_calls && !sends && errors == 1);
    reset(); version_reply = 0; /* Old receiver; never send a mode it misinterprets. */
    assert(LibrarySendExplicit(&instance, 123, &message) == EXIT_FAILURE);
    assert(query_calls == 1 && !sends && strstr(error, "no files were sent"));
    reset(); query_ok = 0;
    assert(LibrarySendExplicit(&instance, 123, &message) == EXIT_FAILURE);
    assert(query_calls == 1 && !sends);
    reset();
    assert(LibrarySendExplicit(&instance, 123, &message) == EXIT_SUCCESS);
    assert(query_calls == 1 && sends == 1 && !errors);
    reset(); open_reply = LIBRARY_IPC_REJECTED;
    assert(LibrarySendExplicit(&instance, 123, &message) == EXIT_FAILURE);
    assert(sends == 1 && errors == 1 && strstr(error, "rejected"));
    reset(); open_reply = LIBRARY_IPC_ACCEPTED_PLAYBACK_FAILED;
    assert(LibrarySendExplicit(&instance, 123, &message) == EXIT_FAILURE);
    assert(sends == 1 && errors == 1 && strstr(error, "files were accepted"));
    assert(strstr(error, "will not be retried"));
    reset(); open_reply = 999;
    assert(LibrarySendExplicit(&instance, 123, &message) == EXIT_FAILURE);
    assert(sends == 1 && errors == 1 && strstr(error, "unknown"));
    /* Receiver might have committed before the transport failed. One send only. */
    reset(); send_ok = 0; open_reply = LIBRARY_IPC_ACCEPTED;
    assert(LibrarySendExplicit(&instance, 123, &message) == EXIT_FAILURE);
    assert(sends == 1 && errors == 1 && strstr(error, "queue may have changed"));
    assert(strstr(error, "will not be retried"));
    puts("sender branch cases: 8"); return 0;
}
