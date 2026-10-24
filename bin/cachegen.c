/*****************************************************************************
 * cachegen.c: LibVLC plugins cache generator
 *****************************************************************************
 * Copyright (C) 2010 Rémi Denis-Courmont
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

#include <vlc/vlc.h>
#include <vlc_common.h>
#include <vlc_modules.h>
#include "../lib/libvlc_internal.h"
#include "../src/modules/cache_output.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#ifdef _WIN32
# include <windows.h>
# include <shellapi.h>
# include <vlc_charset.h>
#endif
#ifdef HAVE_GETOPT_H
# include <getopt.h>
#endif

static void version (void)
{
    puts ("LibVLC plugins cache generation version "VERSION);
}

static void usage (const char *path)
{
    printf (
"Usage: %s [--output <directory>] <path>\n"
"Generate the LibVLC plugins cache for the specified plugins directory.\n"
"With --output, scan only one source and leave its existing cache unchanged.\n",
            path);
}

static int cachegen_main (int argc, char *argv[])
{
    const char *output = NULL;
#ifdef HAVE_GETOPT_H
    static const struct option opts[] =
    {
        { "help",       no_argument,       NULL, 'h' },
        { "version",    no_argument,       NULL, 'V' },
        { "output",     required_argument, NULL, 'o' },
        { NULL,         no_argument,       NULL, '\0'}
    };

    int c;

    while ((c = getopt_long (argc, argv, "hVo:", opts, NULL)) != -1)
        switch (c)
        {
            case 'h':
                usage (argv[0]);
                return 0;
            case 'V':
                version ();
                return 0;
            case 'o':
                output = optarg;
                break;
            default:
                usage (argv[0]);
                return 1;
        }
#else
    int optind = 1;
    while (optind < argc && argv[optind][0] == '-')
    {
        const char *arg = argv[optind++];
        if (!strcmp(arg, "--")) break;
        if (!strcmp(arg, "-h") || !strcmp(arg, "--help")) { usage(argv[0]); return 0; }
        if (!strcmp(arg, "-V") || !strcmp(arg, "--version")) { version(); return 0; }
        if ((!strcmp(arg, "-o") || !strcmp(arg, "--output")) && optind < argc)
            output = argv[optind++];
        else if (!strncmp(arg, "--output=", 9))
            output = arg + 9;
        else { usage(argv[0]); return 1; }
    }
#endif
    if (output != NULL && (!*output || argc - optind != 1 || !*argv[optind]))
    {
        usage(argv[0]);
        return 1;
    }
    if (output != NULL && vlc_plugin_cache_output_protocol() != VLC_CACHE_OUTPUT_PROTOCOL)
        return 1;
    unsetenv(VLC_CACHE_SOURCE_ENV);
    unsetenv(VLC_CACHE_OUTPUT_ENV);

    for (int i = optind; i < argc; i++)
    {
        const char *path = argv[i];

        if (output != NULL)
        {
            if (setenv(VLC_CACHE_SOURCE_ENV, path, 1)
                || setenv(VLC_CACHE_OUTPUT_ENV, output, 1))
                return 1;
        }
        else if (setenv ("VLC_PLUGIN_PATH", path, 1))
            abort ();

        const char *vlc_argv[4];
        int vlc_argc = 0;

        vlc_argv[vlc_argc++] = "--quiet";
        vlc_argv[vlc_argc++] = "--reset-plugins-cache";
        vlc_argv[vlc_argc++] = "--"; /* end of options */
        vlc_argv[vlc_argc] = NULL;

        libvlc_instance_t *vlc = libvlc_new (vlc_argc, vlc_argv);
        if (vlc == NULL)
            return 1;
        const bool valid = output == NULL
            || vlc_module_cache_validate(VLC_OBJECT(vlc->p_libvlc_int), output);
        libvlc_release(vlc);
        if (!valid) return 1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
#ifdef _WIN32
    /* The CRT's narrow argv uses the system code page. VLC paths are UTF-8. */
    (void)argv;
    wchar_t **wide = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (wide == NULL) return 1;
    char **utf8 = calloc((size_t)argc + 1, sizeof(*utf8));
    if (utf8 == NULL) { LocalFree(wide); return 1; }
    int result = 1;
    for (int i = 0; i < argc; ++i) {
        utf8[i] = FromWide(wide[i]);
        if (utf8[i] == NULL) goto out;
    }
    result = cachegen_main(argc, utf8);
out:
    for (int i = 0; i < argc; ++i) free(utf8[i]);
    free(utf8);
    LocalFree(wide);
    return result;
#else
    return cachegen_main(argc, argv);
#endif
}
