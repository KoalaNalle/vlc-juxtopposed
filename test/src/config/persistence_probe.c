/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Opt-in console peer for real Core persistence/concurrency tests. No GUI. */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc/vlc.h>
#include <vlc_common.h>
#include <vlc_config_transaction.h>
#include "lib/libvlc_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *unhex(const char *text)
{
    if (!strcmp(text, "-")) return strdup("");
    size_t size = strlen(text);
    if (size % 2) return NULL;
    char *value = malloc(size / 2 + 1);
    if (!value) return NULL;
    for (size_t i = 0; i < size; i += 2)
    {
        unsigned byte;
        if (sscanf(text + i, "%2x", &byte) != 1 || !byte)
        { free(value); return NULL; }
        value[i / 2] = byte;
    }
    value[size / 2] = 0;
    return value;
}

int main(int argc, char **argv)
{
    if (argc < 2 || argc > 4) return 2;
    char *option;
    if (asprintf(&option, "--config=%s", argv[1]) == -1) return 2;
    const char *args[] = {"--no-ignore-config", "--no-media-library", "--verbose=2",
        "--no-metadata-network-access", "--no-ml-network-discovery", option,
        argc > 2 ? argv[2] : "--no-qt-privacy-ask",
        argc > 3 ? argv[3] : "--no-qt-updates-notif"};
    libvlc_instance_t *instance = libvlc_new(ARRAY_SIZE(args), args);
    free(option);
    if (!instance) return 3;
    libvlc_int_t *obj = instance->p_libvlc_int;
    puts("READY"); fflush(stdout);
    char line[4096], command[32], key[128], value[3500];
    while (fgets(line, sizeof(line), stdin))
    {
        command[0] = key[0] = value[0] = 0;
        int fields = sscanf(line, "%31s %127s %3499s", command, key, value);
        if (fields == 1 && !strcmp(command, "quit")) break;
        if (fields == 1 && !strcmp(command, "save"))
            printf("RESULT %d\n", config_SaveConfigFile(obj));
        else if (fields == 1 && !strcmp(command, "reset"))
        { config_ResetAll(); printf("RESULT %d\n", config_SaveConfigFile(obj)); }
        else if (fields == 2 && !strcmp(command, "geti"))
            printf("INT %"PRId64"\n", config_GetInt(key));
        else if (fields == 2 && !strcmp(command, "inherit"))
            printf("INT %"PRId64"\n", var_InheritInteger(obj, key));
        else if (fields == 2 && !strcmp(command, "getf"))
            printf("FLOAT %a\n", (double)config_GetFloat(key));
        else if (fields == 2 && !strcmp(command, "gets"))
        {
            char *text = config_GetPsz(key);
            printf("STRING ");
            if (text) for (const unsigned char *p = (void *)text; *p; ++p) printf("%02x", *p);
            puts(""); free(text);
        }
        else if (fields == 3 && !strcmp(command, "stagei"))
        { config_PutInt(key, strtoll(value, NULL, 10)); puts("STAGED"); }
        else if (fields == 3 && !strcmp(command, "stages"))
        {
            char *text = unhex(value);
            if (text) { config_PutPsz(key, text); free(text); puts("STAGED"); }
            else puts("INVALID");
        }
        else if (fields == 1 && (!strcmp(command, "triplet") || !strcmp(command, "badbatch")))
        {
            vlc_config_change changes[] = {
                {.name = "audio-language", .type = VLC_VAR_STRING, .value.str = "Suomi"},
                {.name = "rate", .type = VLC_VAR_FLOAT, .value.f = 1.25f},
                {.name = "qt-notification-duration", .type = VLC_VAR_INTEGER,
                 .value.i = !strcmp(command, "badbatch") ? -1 : 5000}};
            printf("RESULT %d\n", config_SaveChanges(obj, changes, ARRAY_SIZE(changes)));
        }
        else if (fields == 3 && (!strcmp(command, "seti") || !strcmp(command, "setf") || !strcmp(command, "sets")))
        {
            vlc_config_change change = {.name = key, .type = config_GetType(key)};
            char *text = NULL;
            if (!strcmp(command, "sets"))
            {
                change.type = VLC_VAR_STRING;
                text = unhex(value);
                if (!text) { puts("INVALID"); fflush(stdout); continue; }
                change.value.str = text;
            }
            else if (!strcmp(command, "setf"))
            { change.type = VLC_VAR_FLOAT; change.value.f = strtof(value, NULL); }
            else change.value.i = strtoll(value, NULL, 10);
            printf("RESULT %d\n", config_SaveChanges(obj, &change, 1));
            free(text);
        }
        else if (fields == 1 && !strcmp(command, "duplicate"))
        {
            vlc_config_change changes[] = {
                {.name = "qt-notification-duration", .type = VLC_VAR_INTEGER, .value.i = 5000},
                {.name = "qt-notification-duration", .type = VLC_VAR_INTEGER, .value.i = 6000}};
            printf("RESULT %d\n", config_SaveChanges(obj, changes, ARRAY_SIZE(changes)));
        }
        else puts("INVALID");
        fflush(stdout);
    }
    libvlc_release(instance);
    puts("CLOSED"); fflush(stdout);
    return 0;
}
