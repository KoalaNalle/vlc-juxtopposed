/*****************************************************************************
 * specific.c: Win32 specific initialization
 *****************************************************************************
 * Copyright (C) 2001-2004, 2010 VLC authors and VideoLAN
 *
 * Authors: Samuel Hocevar <sam@zoy.org>
 *          Gildas Bazin <gbazin@videolan.org>
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

#ifndef UNICODE
# define UNICODE
#endif
#include <vlc_common.h>
#include <vlc_url.h>
#include "../libvlc.h"
#include "../lib/libvlc_internal.h"

#include <winsock2.h>

static int system_InitWSA(int hi, int lo)
{
    WSADATA data;

    if (WSAStartup(MAKEWORD(hi, lo), &data) == 0)
    {
        if (LOBYTE(data.wVersion) == 2 && HIBYTE(data.wVersion) == 2)
            return 0;
        /* Winsock DLL is not usable */
        WSACleanup( );
    }
    return -1;
}

/**
 * Initializes Winsock.
 */
void system_Init(void)
{
    if (system_InitWSA(2, 2) && system_InitWSA(1, 1))
        fputs("Error: cannot initialize Winsocks\n", stderr);
}

/*****************************************************************************
 * system_Configure: check for system specific configuration options.
 *****************************************************************************/

/* Must be same as in modules/control/win_msg.c */
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

/* Query without sending files first: an older receiver treats modes 2/3
 * as legacy enqueue, so its window caption alone is not sufficient. */
static int LibrarySendExplicit(libvlc_int_t *instance, HWND window,
                               const COPYDATASTRUCT *message)
{
    const UINT query = RegisterWindowMessageW(LIBRARY_IPC_QUERY_NAME);
    DWORD_PTR reply = 0;
    const UINT flags = SMTO_BLOCK | SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT;
    if (!query || !SendMessageTimeoutW(window, query, 0, 0, flags, 5000, &reply)
        || reply != LIBRARY_IPC_VERSION)
    {
        msg_Err(instance, "running instance cannot confirm Library IPC v1; "
                          "no files were sent");
        return EXIT_FAILURE;
    }

    reply = LIBRARY_IPC_REJECTED;
    if (!SendMessageTimeoutW(window, WM_COPYDATA, 0, (LPARAM)message,
                             flags, 5000, &reply))
    {
        /* A timeout or disappearing receiver does not prove non-delivery.
         * It may already have inserted the files: never retry automatically. */
        msg_Err(instance, "running instance did not confirm the open request; "
                          "the queue may have changed; request will not be retried");
        return EXIT_FAILURE;
    }

    switch (reply)
    {
        case LIBRARY_IPC_ACCEPTED:
            return EXIT_SUCCESS;
        case LIBRARY_IPC_REJECTED:
            msg_Err(instance, "running instance rejected the open request; "
                              "its queue was not changed by this request");
            return EXIT_FAILURE;
        case LIBRARY_IPC_ACCEPTED_PLAYBACK_FAILED:
            msg_Err(instance, "files were accepted by the running instance, "
                              "but selecting or starting playback failed; "
                              "request will not be retried");
            return EXIT_FAILURE;
        default:
            msg_Err(instance, "running instance returned an unknown open result; "
                              "the queue may have changed; request will not be retried");
            return EXIT_FAILURE;
    }
}

static bool LibrarySingleInstance(libvlc_int_t *p_this)
{
    const int policy = var_InheritInteger(p_this, "one-instance-file-manager-policy");
    if (policy == 0 || policy == 1)
        return var_InheritBool(p_this, "one-instance") &&
            (!var_InheritBool(p_this, "started-from-file") || policy == 1);
    return var_InheritBool( p_this, "one-instance" )
     || ( var_InheritBool( p_this, "one-instance-when-started-from-file" )
       && var_InheritBool( p_this, "started-from-file" ) );
}

static int LibraryOpenMode(libvlc_int_t *p_this)
{
    const int policy = var_InheritInteger(p_this, "playlist-open-mode");
    if (policy == 0 || policy == 1)
        return policy + 2; /* 2 = replace, 3 = append; legacy 0/1 stays intact. */
    return var_InheritBool(p_this, "playlist-enqueue");
}

static void LibraryFreeArguments(char **args, int count)
{
    if (args == NULL)
        return;
    for (int i = 0; i < count; ++i)
        free(args[i]);
    free(args);
}

/* Resolve relative filenames in the launching process. The running instance
 * may have an unrelated working directory. Input options stay with their file. */
static char **LibraryOpenArguments(int count, const char *const argv[])
{
    char **args = calloc(count, sizeof(*args));
    if (args == NULL)
        return NULL;
    for (int i = 0; i < count; ++i)
    {
        args[i] = argv[i][0] == ':' || strstr(argv[i], "://") != NULL
            ? strdup(argv[i]) : vlc_path2uri(argv[i], NULL);
        if (args[i] == NULL)
        {
            LibraryFreeArguments(args, count);
            return NULL;
        }
    }
    return args;
}

void system_Configure( libvlc_int_t *p_this, int i_argc, const char *const ppsz_argv[] )
{
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    if( LibrarySingleInstance(p_this) )
    {
        HANDLE hmutex;

        msg_Info( p_this, "one instance mode ENABLED");

        /* Use a named mutex to check if another instance is already running */
        if( !( hmutex = CreateMutex( 0, TRUE, L"VLC ipc " TEXT(VERSION) ) ) )
        {
            /* Failed for some reason. Just ignore the option and go on as
             * normal. */
            msg_Err( p_this, "one instance mode DISABLED "
                     "(mutex couldn't be created)" );
            return;
        }

        if( GetLastError() != ERROR_ALREADY_EXISTS )
        {
            libvlc_InternalAddIntf( p_this, "win_msg,none" );
            /* Initialization done.
             * Release the mutex to unblock other instances */
            ReleaseMutex( hmutex );
        }
        else
        {
            /* Another instance is running */

            HWND ipcwindow;

            /* Wait until the 1st instance is initialized */
            WaitForSingleObject( hmutex, INFINITE );

            /* Locate the window created by the IPC helper thread of the
             * 1st instance */
            if( !( ipcwindow = FindWindow( 0, L"VLC ipc " TEXT(VERSION) ) ) )
            {
                msg_Err( p_this, "one instance mode DISABLED "
                         "(couldn't find 1st instance of program)" );
                ReleaseMutex( hmutex );
                return;
            }

            int exit_status = EXIT_SUCCESS;
            const int open_mode = LibraryOpenMode(p_this);
            char **explicit_args = NULL;
            const char *const *send_args = ppsz_argv;

            /* We assume that the remaining parameters are filenames
             * and their input options */
            if( i_argc > 0 )
            {
                COPYDATASTRUCT wm_data;
                int i_opt;
                vlc_ipc_data_t *p_data;
                size_t i_data = sizeof (*p_data);

                /* Match the explicit receiver's bounds before allocation. */
                if (open_mode >= 2 && i_argc > 4096)
                {
                    msg_Err(p_this, "too many files/options for the running instance");
                    exit_status = EXIT_FAILURE;
                    goto handoff_done;
                }
                if (open_mode >= 2)
                {
                    explicit_args = LibraryOpenArguments(i_argc, ppsz_argv);
                    if (explicit_args == NULL)
                    {
                        msg_Err(p_this, "cannot resolve files for the running instance");
                        exit_status = EXIT_FAILURE;
                        goto handoff_done;
                    }
                    send_args = (const char *const *)explicit_args;
                }
                for( i_opt = 0; i_opt < i_argc; i_opt++ )
                {
                    const size_t length = strlen(send_args[i_opt]) + 1;
                    if (open_mode >= 2 &&
                        (length > 16 * 1024 * 1024 - sizeof(size_t) ||
                         i_data > 16 * 1024 * 1024 - sizeof(size_t) - length))
                    {
                        msg_Err(p_this, "open request exceeds the running instance's limit");
                        exit_status = EXIT_FAILURE;
                        goto handoff_done;
                    }
                    i_data += sizeof(size_t) + length;
                }

                p_data = malloc( i_data );
                if (!p_data)
                {
                    if (open_mode < 2) { ReleaseMutex(hmutex); return; }
                    msg_Err(p_this, "cannot allocate the open request");
                    exit_status = EXIT_FAILURE;
                    goto handoff_done;
                }
                p_data->argc = i_argc;
                p_data->enqueue = open_mode;
                i_data = 0;
                for( i_opt = 0; i_opt < i_argc; i_opt++ )
                {
                    size_t i_len = strlen( send_args[ i_opt ] ) + 1;
                    memcpy( &p_data->data[i_data], &i_len, sizeof (size_t) );
                    i_data += sizeof (size_t);
                    memcpy( &p_data->data[i_data], send_args[ i_opt ], i_len );
                    i_data += i_len;
                }
                i_data += sizeof (*p_data);

                /* Send our playlist items to the 1st instance */
                wm_data.dwData = 0;
                wm_data.cbData = i_data;
                wm_data.lpData = p_data;
                if (open_mode >= 2)
                    exit_status = LibrarySendExplicit(p_this, ipcwindow, &wm_data);
                else
                    SendMessage(ipcwindow, WM_COPYDATA, 0, (LPARAM)&wm_data);
                free(p_data);
            }

handoff_done:
            LibraryFreeArguments(explicit_args, i_argc);
            /* Initialization done.
             * Release the mutex to unblock other instances */
            ReleaseMutex( hmutex );

            /* Bye bye */
            system_End( );
            exit(exit_status);
        }
    }
#endif
}

/**
 * Cleans up after system_Init() and system_Configure().
 */
void system_End(void)
{
    /* XXX: In theory, we should not call this if WSAStartup() failed. */
    WSACleanup();
}
