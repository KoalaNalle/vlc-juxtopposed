/*****************************************************************************
 * win32.c: Global-Hotkey _WIN32 handling for vlc
 *****************************************************************************
 * Copyright (C) 2008-2009 the VideoLAN team
 *
 * Authors: Domani Hannes <ssbssa at yahoo dot de>
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

#include <inttypes.h>

#define VLC_MODULE_LICENSE VLC_LICENSE_GPL_2_PLUS
#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_interface.h>
#include <vlc_actions.h>
#include <vlc_memstream.h>
#include <vlc_vector.h>

#include <windows.h>

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
static int Open( vlc_object_t *p_this );
static void Close( vlc_object_t *p_this );
static void *Thread( void *p_data );
LRESULT CALLBACK WMHOTKEYPROC( HWND, UINT, WPARAM, LPARAM );

static void AutoRun(libvlc_int_t *libvlc)
{
    intf_Create(libvlc, MODULE_STRING);
}

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
vlc_module_begin()
    set_shortname( N_("Global Hotkeys") )
    set_subcategory( SUBCAT_INTERFACE_HOTKEYS )
    set_description( N_("Global Hotkeys interface") )
    set_capability( "interface", 0 )
    set_callbacks( Open, Close )
    add_shortcut( "globalhotkeys" )

    add_submodule()
    set_capability("autorun", 10)
    set_callback(AutoRun)
vlc_module_end()

struct intf_sys_t
{
    vlc_thread_t thread;
    HWND hotkeyWindow;
    vlc_mutex_t lock;
    vlc_sem_t wait;
    struct VLC_VECTOR(ATOM) registered;
};

struct hotkey_error
{
    vlc_action_id_t action;
    uint_fast32_t key;
    DWORD code;
    bool unsupported;
};
struct hotkey_report
{
    size_t failed, count;
    struct hotkey_error errors[32];
};

static void RecordFailure(struct hotkey_report *report, vlc_action_id_t action,
                          uint_fast32_t key, DWORD code, bool unsupported)
{
    report->failed++;
    if (report->count < ARRAY_SIZE(report->errors))
        report->errors[report->count++] = (struct hotkey_error){action, key, code, unsupported};
}

/* Transient, per-instance results. They describe the running module, not newly
 * saved mappings. Retain the last result until this libvlc instance is deleted. */
static void PublishStatus(intf_thread_t *intf, const char *state,
                          const struct hotkey_report *report)
{
    libvlc_int_t *root = vlc_object_instance(intf);
    if (var_GetAddress(root, "global-hotkeys-owner") != intf) return;
    intf_sys_t *sys = intf->p_sys;
    struct vlc_memstream stream;
    vlc_memstream_open(&stream);
    vlc_memstream_printf(&stream,
        "{\"version\":1,\"state\":\"%s\",\"registered\":%zu,\"failed\":%zu,\"omitted\":%zu,\"errors\":[",
        state, sys->registered.size, report->failed, report->failed - report->count);
    for (size_t i = 0; i < report->count; ++i)
    {
        const struct hotkey_error *error = &report->errors[i];
        vlc_memstream_printf(&stream,
            "%s{\"action\":%u,\"key\":%" PRIuFAST32 ",\"code\":%lu,\"unsupported\":%s}",
            i ? "," : "", (unsigned)error->action, error->key,
            (unsigned long)error->code, error->unsupported ? "true" : "false");
    }
    vlc_memstream_puts(&stream, "]}");
    if (vlc_memstream_close(&stream) == 0)
    {
        var_SetString(root, "global-hotkeys-status", stream.ptr);
        free(stream.ptr);
    }
    else var_SetString(root, "global-hotkeys-status", "{\"version\":1,\"state\":\"unavailable\"}");
}

/*****************************************************************************
 * Open: initialize interface
 *****************************************************************************/
static int Open( vlc_object_t *p_this )
{
    intf_thread_t *p_intf = (intf_thread_t *)p_this;
    intf_sys_t *p_sys = vlc_obj_malloc( p_this, sizeof (intf_sys_t) );

    if( p_sys == NULL )
        return VLC_ENOMEM;

    p_intf->p_sys = p_sys;
    libvlc_int_t *root = vlc_object_instance(p_intf);
    if (var_Type(root, "global-hotkeys-owner") == 0
        && var_Create(root, "global-hotkeys-owner", VLC_VAR_ADDRESS) != VLC_SUCCESS)
        return VLC_ENOMEM;
    if (var_Type(root, "global-hotkeys-status") == 0
        && var_Create(root, "global-hotkeys-status", VLC_VAR_STRING) != VLC_SUCCESS)
        return VLC_ENOMEM;
    if (var_Type(root, "global-hotkeys-owner") != VLC_VAR_ADDRESS
        || var_Type(root, "global-hotkeys-status") != VLC_VAR_STRING
        || var_GetAddress(root, "global-hotkeys-owner") != NULL)
        return VLC_EGENERIC;
    var_SetAddress(root, "global-hotkeys-owner", p_intf);
    p_sys->hotkeyWindow = NULL;
    vlc_vector_init(&p_sys->registered);
    PublishStatus(p_intf, "starting", &(struct hotkey_report){0});
    vlc_mutex_init( &p_sys->lock );
    vlc_sem_init( &p_sys->wait, 0 );

    if( vlc_clone( &p_sys->thread, Thread, p_intf ) )
    {
        PublishStatus(p_intf, "failed", &(struct hotkey_report){0});
        var_SetAddress(root, "global-hotkeys-owner", NULL);
        vlc_vector_destroy(&p_sys->registered);
        return VLC_ENOMEM;
    }

    vlc_sem_wait( &p_sys->wait );
    vlc_mutex_lock( &p_sys->lock );
    bool fail = p_sys->hotkeyWindow == NULL;
    vlc_mutex_unlock( &p_sys->lock );

    if( fail )
    {
        vlc_join( p_sys->thread, NULL );
        var_SetAddress(root, "global-hotkeys-owner", NULL);
    }

    return fail ? VLC_ENOMEM : VLC_SUCCESS;
}

/*****************************************************************************
 * Close: destroy interface
 *****************************************************************************/
static void Close( vlc_object_t *p_this )
{
    intf_thread_t *p_intf = (intf_thread_t *)p_this;
    intf_sys_t *p_sys = p_intf->p_sys;

    /* stop hotkey window */
    vlc_mutex_lock( &p_sys->lock );
    if( p_sys->hotkeyWindow != NULL )
        PostMessage( p_sys->hotkeyWindow, WM_CLOSE, 0, 0 );
    vlc_mutex_unlock( &p_sys->lock );

    vlc_join( p_sys->thread, NULL );
    var_SetAddress(vlc_object_instance(p_intf), "global-hotkeys-owner", NULL);
}

/*****************************************************************************
 * Thread: main loop
 *****************************************************************************/
static void *Thread( void *p_data )
{
    MSG message;

    vlc_thread_set_name("vlc-hotkeys-win");

    intf_thread_t *p_intf = p_data;
    intf_sys_t *p_sys = p_intf->p_sys;
    struct hotkey_report report = {0};

    /* Window which receives Hotkeys */
    p_sys->hotkeyWindow =
        (void*)CreateWindow( TEXT("STATIC"),         /* name of window class */
                TEXT("VLC ghk ") TEXT(VERSION),     /* window title bar text */
                0,                                           /* window style */
                0,                                   /* default X coordinate */
                0,                                   /* default Y coordinate */
                0,                                           /* window width */
                0,                                          /* window height */
                NULL,                                    /* no parent window */
                NULL,                              /* no menu in this window */
                GetModuleHandle(NULL),    /* handle of this program instance */
                NULL );                                 /* sent to WM_CREATE */
    if( p_sys->hotkeyWindow == NULL )
    {
        PublishStatus(p_intf, "failed", &report);
        vlc_vector_destroy(&p_sys->registered);
        vlc_sem_post(&p_sys->wait);
        return NULL;
    }
    vlc_sem_post( &p_sys->wait );

    SetWindowLongPtr( p_sys->hotkeyWindow, GWLP_WNDPROC,
            (LONG_PTR)WMHOTKEYPROC );
    SetWindowLongPtr( p_sys->hotkeyWindow, GWLP_USERDATA,
            (LONG_PTR)p_intf );

    /* Bind characters using this registration thread's startup keyboard layout. */
    const HKL keyboard_layout = GetKeyboardLayout(0);
    /* Registering of Hotkeys */
    for( const char* const* ppsz_keys = vlc_actions_get_key_names( p_intf );
         *ppsz_keys != NULL; ppsz_keys++ )
    {
        char action_name[44];
        const int length = snprintf(action_name, sizeof(action_name), "key-%s", *ppsz_keys);
        const vlc_action_id_t action = length >= 0 && (size_t)length < sizeof(action_name)
            ? vlc_actions_get_id(action_name) : ACTIONID_NONE;
        uint_fast32_t *p_keys;
        size_t i_nb_keys = vlc_actions_get_keycodes( p_intf, *ppsz_keys, true,
                                                     &p_keys );
        for( size_t i = 0; i < i_nb_keys; ++i )
        {
            uint_fast32_t i_key = p_keys[i];
            /* The legacy mapping does not represent these modifiers. Never
             * register their unmodified counterpart as if it were equivalent. */
            if (i_key & (KEY_MODIFIER_META | KEY_MODIFIER_COMMAND))
            {
                RecordFailure(&report, action, i_key, ERROR_NOT_SUPPORTED, true);
                continue;
            }
            UINT i_keyMod = 0;
            if( i_key & KEY_MODIFIER_SHIFT ) i_keyMod |= MOD_SHIFT;
            if( i_key & KEY_MODIFIER_ALT ) i_keyMod |= MOD_ALT;
            if( i_key & KEY_MODIFIER_CTRL ) i_keyMod |= MOD_CONTROL;

#define HANDLE( key ) case KEY_##key: i_vk = VK_##key; break
#define HANDLE2( key, key2 ) case KEY_##key: i_vk = VK_##key2; break

#define KEY_SPACE ' '

#ifndef VK_VOLUME_DOWN
#define VK_VOLUME_DOWN          0xAE
#define VK_VOLUME_UP            0xAF
#endif

#ifndef VK_MEDIA_NEXT_TRACK
#define VK_MEDIA_NEXT_TRACK     0xB0
#define VK_MEDIA_PREV_TRACK     0xB1
#define VK_MEDIA_STOP           0xB2
#define VK_MEDIA_PLAY_PAUSE     0xB3
#endif

#ifndef VK_PAGEUP
#define VK_PAGEUP               0x21
#define VK_PAGEDOWN             0x22
#endif

            UINT i_vk = 0;
            switch( i_key & ~KEY_MODIFIER )
            {
                HANDLE( LEFT );
                HANDLE( RIGHT );
                HANDLE( UP );
                HANDLE( DOWN );
                HANDLE( SPACE );
                HANDLE( TAB );
                HANDLE2( BACKSPACE, BACK );
                HANDLE2( ESC, ESCAPE );
                HANDLE2( ENTER, RETURN );
                HANDLE( F1 );
                HANDLE( F2 );
                HANDLE( F3 );
                HANDLE( F4 );
                HANDLE( F5 );
                HANDLE( F6 );
                HANDLE( F7 );
                HANDLE( F8 );
                HANDLE( F9 );
                HANDLE( F10 );
                HANDLE( F11 );
                HANDLE( F12 );
                HANDLE( PAGEUP );
                HANDLE( PAGEDOWN );
                HANDLE( HOME );
                HANDLE( END );
                HANDLE( INSERT );
                HANDLE( DELETE );
                HANDLE2( MENU, APPS );
                HANDLE2( PRINT, SNAPSHOT );
                HANDLE( PAUSE );
                HANDLE( BROWSER_BACK );
                HANDLE( BROWSER_FORWARD );
                HANDLE( BROWSER_REFRESH );
                HANDLE( BROWSER_STOP );
                HANDLE( BROWSER_SEARCH );
                HANDLE( BROWSER_FAVORITES );
                HANDLE( BROWSER_HOME );
                HANDLE( VOLUME_MUTE );
                HANDLE( VOLUME_DOWN );
                HANDLE( VOLUME_UP );
                HANDLE( MEDIA_PLAY_PAUSE );
                HANDLE( MEDIA_STOP );
                HANDLE( MEDIA_PREV_TRACK );
                HANDLE( MEDIA_NEXT_TRACK );

                default:
                {
                    uint_fast32_t key = i_key & ~KEY_MODIFIER;
                    if (key >= KEY_F(13) && key <= KEY_F(24)
                        && (key - KEY_F(13)) % 0x10000 == 0)
                        i_vk = VK_F13 + (key - KEY_F(13)) / 0x10000;
                    else if (key >= 0x20 && key <= 0xFFFF
                             && (key < 0xD800 || key > 0xDFFF))
                    {
                        /* VLC's key converter uses lowercase letters with an
                         * independent Shift modifier. Preserve the old ASCII
                         * case-insensitive binding while translating punctuation
                         * and other representable characters without truncation. */
                        if (key >= 'A' && key <= 'Z') key += 'a' - 'A';
                        const SHORT mapping = VkKeyScanExW((WCHAR)key, keyboard_layout);
                        if (mapping != -1 && !(HIBYTE(mapping) & ~7))
                        {
                            i_vk = LOBYTE(mapping);
                            if (HIBYTE(mapping) & 1) i_keyMod |= MOD_SHIFT;
                            if (HIBYTE(mapping) & 2) i_keyMod |= MOD_CONTROL;
                            if (HIBYTE(mapping) & 4) i_keyMod |= MOD_ALT;
                        }
                    }
                    break;
                }
            }
            if( !i_vk || (i_key & ~KEY_MODIFIER) == KEY_F12 )
            {
                RecordFailure(&report, action, i_key, ERROR_NOT_SUPPORTED, true);
                continue;
            }

#undef HANDLE
#undef HANDLE2

            ATOM atom = GlobalAddAtomA( *ppsz_keys );
            if( !atom )
            {
                RecordFailure(&report, action, i_key, GetLastError(), false);
                continue;
            }
            if (!vlc_vector_push(&p_sys->registered, atom))
            {
                GlobalDeleteAtom(atom);
                RecordFailure(&report, action, i_key, ERROR_NOT_ENOUGH_MEMORY, false);
                continue;
            }

            if( !RegisterHotKey( p_sys->hotkeyWindow, atom, i_keyMod, i_vk ) )
            {
                const DWORD error = GetLastError();
                p_sys->registered.size--;
                GlobalDeleteAtom( atom );
                RecordFailure(&report, action, i_key, error, false);
            }
        }
        free( p_keys );
    }
    PublishStatus(p_intf, report.failed ? p_sys->registered.size ? "partial" : "failed"
                  : p_sys->registered.size ? "active" : "inactive", &report);

    /* Main message loop */
    while( GetMessage( &message, NULL, 0, 0 ) )
        DispatchMessage( &message );

    /* Unregistering of Hotkeys */
    for (size_t i = 0; i < p_sys->registered.size; ++i)
    {
        const ATOM atom = p_sys->registered.data[i];
        UnregisterHotKey(p_sys->hotkeyWindow, atom);
        // Balance every successful GlobalAddAtom, including multiple bindings
        // of one action. Never release a different instance's atom reference.
        GlobalDeleteAtom(atom);
    }
    p_sys->registered.size = 0;
    PublishStatus(p_intf, "stopped", &report);
    vlc_vector_destroy(&p_sys->registered);

    /* close window */
    vlc_mutex_lock( &p_sys->lock );
    DestroyWindow( p_sys->hotkeyWindow );
    p_sys->hotkeyWindow = NULL;
    vlc_mutex_unlock( &p_sys->lock );

    return NULL;
}

/*****************************************************************************
 * WMHOTKEYPROC: event callback
 *****************************************************************************/
LRESULT CALLBACK WMHOTKEYPROC( HWND hwnd, UINT uMsg, WPARAM wParam,
        LPARAM lParam )
{
    switch( uMsg )
    {
        case WM_HOTKEY:
            {
                char psz_atomName[44];

                LONG_PTR ret = GetWindowLongPtr( hwnd, GWLP_USERDATA );
                intf_thread_t *p_intf = (intf_thread_t*)ret;
                strcpy( psz_atomName, "key-" );

                if( !GlobalGetAtomNameA(
                        wParam, psz_atomName + 4,
                        sizeof( psz_atomName ) - 4 ) )
                    return 0;

                /* search for key associated with VLC */
                vlc_action_id_t action = vlc_actions_get_id( psz_atomName );
                if( action != ACTIONID_NONE )
                {
                    var_SetInteger( vlc_object_instance(p_intf),
                            "key-action", action );
                    return 1;
                }
            }
            break;

        case WM_DESTROY:
            PostQuitMessage( 0 );
            break;

        default:
            return DefWindowProc( hwnd, uMsg, wParam, lParam );
    }

    return 0;
}
