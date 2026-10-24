// SPDX-License-Identifier: LGPL-2.1-or-later
/* Private desired-fixture plugin ONLY. Highest priority and no network code.
 * Both scopes succeed so lower-priority real providers are never consulted.
 * Install solely in Root's owned native test plugin tree, never the package. */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_input_item.h>
#include <vlc_meta.h>
#include <vlc_meta_fetcher.h>
#include <vlc_variables.h>
#include <vlc_fs.h>
#include <vlc_interrupt.h>
#ifdef _WIN32
# include <windows.h>
#else
# include <unistd.h>
#endif
static bool Active(vlc_object_t *root)
{
    return (var_Type(root, "library-fixture-active") == VLC_VAR_BOOL
            && var_GetBool(root, "library-fixture-active"))
        || getenv("VLC_LIBRARY_FIXTURE_LOG") != NULL;
}
static void Trace(const char *kind)
{
    const char *prefix = getenv("VLC_LIBRARY_FIXTURE_LOG");
    char *path;
    if (!prefix || asprintf(&path, "%s-%s.log", prefix, kind) < 0)
        return;
    FILE *file = vlc_fopen(path, "ab");
    free(path);
    if (file) {
#ifdef _WIN32
        fprintf(file,"%lu\n",(unsigned long)GetCurrentProcessId());
#else
        fprintf(file,"%lu\n",(unsigned long)getpid());
#endif
        fclose(file);
    }
}
static int Fetch(vlc_object_t *object)
{
    meta_fetcher_t *fetcher = (meta_fetcher_t *)object;
    vlc_object_t *root = VLC_OBJECT(vlc_object_instance(object));
    if (!Active(root)) return VLC_EGENERIC;
    Trace(fetcher->e_scope == FETCHER_SCOPE_LOCAL ? "local" : "network");
    const char *delay = getenv("VLC_LIBRARY_FIXTURE_DELAY_MS");
    if (delay && fetcher->e_scope == FETCHER_SCOPE_NETWORK)
    {
        input_item_SetTitle(fetcher->p_item, "Uncommitted delayed metadata");
        vlc_msleep_i11e(VLC_TICK_FROM_MS(atoi(delay)));
        if (vlc_killed())
        {
            Trace("interrupted");
            return VLC_SUCCESS;
        }
    }
    if (fetcher->e_scope == FETCHER_SCOPE_LOCAL)
    {
        if (var_Type(root, "library-fixture-local"))
            var_IncInteger(root, "library-fixture-local");
    }
    else
    {
        if (var_Type(root, "library-fixture-network"))
            var_IncInteger(root, "library-fixture-network");
        input_item_SetTitle(fetcher->p_item, "Owned network metadata fixture");
        input_item_SetArtist(fetcher->p_item, "Owned fixture artist");
        input_item_SetAlbum(fetcher->p_item, "Owned fixture album");
    }
    return VLC_SUCCESS;
}
static int Art(vlc_object_t *object)
{
    meta_fetcher_t *fetcher = (meta_fetcher_t *)object;
    vlc_object_t *root = VLC_OBJECT(vlc_object_instance(object));
    if (!Active(root)) return VLC_EGENERIC;
    if (fetcher->e_scope != FETCHER_SCOPE_LOCAL)
    {
        char *uri = var_Type(root, "library-fixture-art") == VLC_VAR_STRING
            ? var_GetString(root, "library-fixture-art") : NULL;
        if (uri == NULL)
        {
            const char *test_art = getenv("VLC_LIBRARY_FIXTURE_ART");
            uri = test_art ? strdup(test_art) : NULL;
        }
        if (!uri || strncmp(uri, "file://", 7)) { free(uri); return VLC_EGENERIC; }
        input_item_SetArtURL(fetcher->p_item, uri); free(uri);
    }
    return VLC_SUCCESS;
}
vlc_module_begin()
    set_shortname("Owned Library fixture")
    set_description("Private local-only Library desired fixture")
    set_capability("meta fetcher", 1000000)
    set_callback(Fetch)
    add_submodule()
    add_shortcut("library_fixture_fetcher")
    set_capability("art finder", 1000000)
    set_callback(Art)
vlc_module_end()
