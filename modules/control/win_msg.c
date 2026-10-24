/*****************************************************************************
 * ntservice.c: Windows NT/2K/XP service interface
 *****************************************************************************
 * Copyright (C) 2001-2004, 2010 VLC authors and VideoLAN
 *
 * Authors: Samuel Hocevar <sam@zoy.org>
 *          Gildas Bazin <gbazin@videolan.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define VLC_MODULE_LICENSE VLC_LICENSE_GPL_2_PLUS
#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_interface.h>
#include <vlc_playlist.h>
#include <vlc_url.h> // FIXME: move URL generation to calling process
#include <vlc_input_item.h>

#include <windows.h>

struct intf_sys_t
{
    HWND window;
    HANDLE ready;
    vlc_thread_t thread;
};

/* Must be same as in src/win32/specific.c */
typedef struct
{
    int argc;
    int enqueue;
    char data[];
} vlc_ipc_data_t;

/* Explicit Library IPC v1. Keep this block identical in both peers.
 * Rejected means no queue insertion. Playback failure means the queue was
 * accepted, so the sender must not replay the request or open it locally.
 * Acceptance covers synchronous operations, not later decoder/input errors. */
#define LIBRARY_IPC_QUERY_NAME L"VLC.LibraryOpen.v1"
#define LIBRARY_IPC_VERSION 0x4c564301
enum
{
    LIBRARY_IPC_REJECTED = 0,
    LIBRARY_IPC_ACCEPTED = 1,
    LIBRARY_IPC_ACCEPTED_PLAYBACK_FAILED = 2,
};

static void add_to_playlist(intf_thread_t *intf, const char *uri,
                            bool play_now, int options_count,
                            const char *const *options)
{
    vlc_playlist_t *playlist = vlc_intf_GetMainPlaylist(intf);

    input_item_t *media = input_item_New(uri, NULL);
    if (!media)
        return;
    input_item_AddOptions(media, options_count, options, VLC_INPUT_OPTION_TRUSTED);

    vlc_playlist_Lock(playlist);
    vlc_playlist_AppendOne(playlist, media);
    if (play_now)
        vlc_playlist_Start(playlist);
    vlc_playlist_Unlock(playlist);
    input_item_Release(media);
}

/* New explicit policies use 2/3. Legacy payloads 0/1 retain their old path.
 * Validate/allocate everything before changing the actual playback queue. */
static LRESULT LibraryOpenFiles(intf_thread_t *intf, const COPYDATASTRUCT *message)
{
    if (!message || !message->lpData || message->cbData < sizeof(vlc_ipc_data_t)
        || message->cbData > 16 * 1024 * 1024)
        return LIBRARY_IPC_REJECTED;
    const vlc_ipc_data_t *data = message->lpData;
    if (data->argc <= 0 || data->argc > 4096 || (data->enqueue != 2 && data->enqueue != 3))
        return LIBRARY_IPC_REJECTED;
    const char **args = vlc_alloc(data->argc, sizeof(*args));
    input_item_t **items = vlc_alloc(data->argc, sizeof(*items));
    if (!args || !items) { free(args); free(items); return LIBRARY_IPC_REJECTED; }
    size_t at = 0, available = message->cbData - sizeof(*data), count = 0;
    int result = VLC_EINVAL;
    bool accepted = false;
    for (int i = 0; i < data->argc; ++i)
    {
        size_t length;
        if (at > available || available - at < sizeof(length)) goto out;
        memcpy(&length, data->data + at, sizeof(length)); at += sizeof(length);
        if (!length || length > available - at) goto out;
        const char *arg = data->data + at;
        if (arg[length - 1] != '\0' || memchr(arg, '\0', length - 1) || !arg[0]) goto out;
        args[i] = arg; at += length;
    }
    if (at != available || args[0][0] == ':') goto out;
    for (int i = 0; i < data->argc; )
    {
        int options = 0;
        while (i + options + 1 < data->argc && args[i + options + 1][0] == ':') ++options;
        char *uri = strstr(args[i], "://") ? NULL : vlc_path2uri(args[i], NULL);
        if (!strstr(args[i], "://") && !uri) { result = VLC_ENOMEM; goto out; }
        input_item_t *item = input_item_New(uri ? uri : args[i], NULL); free(uri);
        if (!item) { result = VLC_ENOMEM; goto out; }
        items[count++] = item;
        result = input_item_AddOptions(item, options, options ? &args[i + 1] : NULL, VLC_INPUT_OPTION_TRUSTED);
        if (result != VLC_SUCCESS) goto out;
        i += options + 1;
    }
    vlc_playlist_t *playlist = vlc_intf_GetMainPlaylist(intf);
    vlc_playlist_Lock(playlist);
    const size_t previous = vlc_playlist_Count(playlist);
    const bool replace = data->enqueue == 2;
    /* Insert's own allocation failure rolls back before notifying; insert
     * first so the old queue is not destroyed if an allocation fails. */
    result = vlc_playlist_Insert(playlist, replace ? 0 : previous, items, count);
    if (result == VLC_SUCCESS)
    {
        accepted = true;
        if (replace)
        {
            if (previous) vlc_playlist_Remove(playlist, count, previous);
            /* The new queue is committed. GoTo may change player state even
             * when it fails, so neither this receiver nor the sender retries
             * or claims to restore the previous playback transactionally. */
            result = vlc_playlist_GoTo(playlist, 0);
            if (result != VLC_SUCCESS)
                msg_Err(intf, "files accepted, but playlist selection failed (%d)", result);
            else
            {
                result = vlc_playlist_Start(playlist);
                if (result != VLC_SUCCESS)
                    msg_Err(intf, "files accepted, but starting playback failed (%d)", result);
            }
        }
    }
    vlc_playlist_Unlock(playlist);
out:
    for (size_t i = 0; i < count; ++i) input_item_Release(items[i]);
    free(items); free(args);
    if (!accepted)
        return LIBRARY_IPC_REJECTED;
    return result == VLC_SUCCESS ? LIBRARY_IPC_ACCEPTED
                                 : LIBRARY_IPC_ACCEPTED_PLAYBACK_FAILED;
}

static LRESULT CALLBACK WMCOPYWNDPROC(HWND hwnd, UINT uMsg,
                                      WPARAM wParam, LPARAM lParam)
{
    /* Registered messages are above the ordinary window-message range.
     * An old receiver returns zero from DefWindowProc for this query. */
    if (uMsg >= 0xc000 && uMsg == RegisterWindowMessageW(LIBRARY_IPC_QUERY_NAME))
        return GetWindowLongPtr(hwnd, GWLP_USERDATA) ? LIBRARY_IPC_VERSION : 0;

    if( uMsg == WM_QUIT  )
    {
        PostQuitMessage( 0 );
    }
    else if( uMsg == WM_COPYDATA )
    {
        COPYDATASTRUCT *pwm_data = (COPYDATASTRUCT*)lParam;

        intf_thread_t *intf = (intf_thread_t *)(uintptr_t)
            GetWindowLongPtr( hwnd, GWLP_USERDATA );
        if( intf == NULL )
            return 0; /* XXX: is this even possible? */

        if (pwm_data && pwm_data->lpData && pwm_data->cbData >= sizeof(vlc_ipc_data_t))
        {
            vlc_ipc_data_t *incoming = pwm_data->lpData;
            if (incoming->enqueue == 2 || incoming->enqueue == 3)
                return LibraryOpenFiles(intf, pwm_data);
        }

        /* Add files to the playlist */
        if( pwm_data->lpData )
        {
            char **ppsz_argv;
            vlc_ipc_data_t *p_data = (vlc_ipc_data_t *)pwm_data->lpData;
            size_t i_data = 0;
            size_t i_len;
            int i_argc = p_data->argc, i_opt, i_options;

            ppsz_argv = vlc_alloc( i_argc, sizeof(char *) );
            for( i_opt = 0; i_opt < i_argc; i_opt++ )
            {
                memcpy( &i_len, &p_data->data[i_data], sizeof(size_t) );
                ppsz_argv[i_opt] = &p_data->data[i_data] + sizeof(size_t);
                i_data += sizeof(size_t) + i_len;
            }

            for( i_opt = 0; i_opt < i_argc; i_opt++ )
            {
                i_options = 0;

                /* Count the input options */
                while( i_opt + i_options + 1 < i_argc &&
                        *ppsz_argv[ i_opt + i_options + 1 ] == ':' )
                {
                    i_options++;
                }

#warning URI conversion must be done in calling process instead!
                /* FIXME: This breaks relative paths if calling vlc.exe is
                 * started from a different working directory. */
                char *psz_URI = NULL;
                if( strstr( ppsz_argv[i_opt], "://" ) == NULL )
                    psz_URI = vlc_path2uri( ppsz_argv[i_opt], NULL );
                add_to_playlist(intf, (psz_URI != NULL) ? psz_URI : ppsz_argv[i_opt],
                                (i_opt == 0 && !p_data->enqueue), i_options,
                                (char const **)( i_options ? &ppsz_argv[i_opt+1] : NULL ));
                i_opt += i_options;
                free( psz_URI );
            }

            free( ppsz_argv );
        }
    }

    return DefWindowProc( hwnd, uMsg, wParam, lParam );
}

static void *HelperThread(void *data)
{
    intf_thread_t *intf = data;
    intf_sys_t *sys = intf->p_sys;

    vlc_thread_set_name("vlc-ctrl-win");

    HWND ipcwindow =
        CreateWindow(L"STATIC",                      /* name of window class */
                  L"VLC ipc " TEXT(VERSION),        /* window title bar text */
                  0,                                         /* window style */
                  0,                                 /* default X coordinate */
                  0,                                 /* default Y coordinate */
                  0,                                         /* window width */
                  0,                                        /* window height */
                  NULL,                                  /* no parent window */
                  NULL,                            /* no menu in this window */
                  GetModuleHandle(NULL),  /* handle of this program instance */
                  NULL) ;                               /* sent to WM_CREATE */

    SetWindowLongPtr(ipcwindow, GWLP_WNDPROC, (LRESULT)WMCOPYWNDPROC);
    SetWindowLongPtr(ipcwindow, GWLP_USERDATA, (uintptr_t)data);

    sys->window = ipcwindow;
    /* Signal the creation of the thread and events queue */
    SetEvent(sys->ready);

    MSG message;
    while (GetMessage(&message, NULL, 0, 0))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return NULL;
}

static int Open(vlc_object_t *obj)
{
    intf_thread_t *intf = (intf_thread_t *)obj;
    intf_sys_t *sys = malloc(sizeof (*sys));
    if (unlikely(sys == NULL))
        return VLC_ENOMEM;

    intf->p_sys = sys;

    /* Run the helper thread */
    sys->ready = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (vlc_clone(&sys->thread, HelperThread, intf))
    {
        free(sys);
        msg_Err(intf, "one instance mode DISABLED "
                 "(IPC helper thread couldn't be created)");
        return VLC_ENOMEM;
    }

    WaitForSingleObject(sys->ready, INFINITE);
    CloseHandle(sys->ready);

    return VLC_SUCCESS;
}

static void Close(vlc_object_t *obj)
{
    intf_thread_t *intf = (intf_thread_t *)obj;
    intf_sys_t *sys = intf->p_sys;

    SendMessage(sys->window, WM_QUIT, 0, 0);
    vlc_join(sys->thread, NULL);
    free(sys);
}

vlc_module_begin()
    set_shortname(N_("WinMsg"))
    set_description(N_("Windows messages interface"))
    set_subcategory(SUBCAT_INTERFACE_CONTROL)
    set_capability("interface", 0)
    set_callbacks(Open, Close)
vlc_module_end()
