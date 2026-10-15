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
static int Fetch(vlc_object_t *object)
{
    meta_fetcher_t *fetcher = (meta_fetcher_t *)object;
    vlc_object_t *root = VLC_OBJECT(vlc_object_instance(object));
    if (!var_InheritBool(root, "library-fixture-active")) return VLC_EGENERIC;
    if (fetcher->e_scope == FETCHER_SCOPE_LOCAL)
        var_IncInteger(root, "library-fixture-local");
    else
    {
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
    if (!var_InheritBool(root, "library-fixture-active")) return VLC_EGENERIC;
    if (fetcher->e_scope != FETCHER_SCOPE_LOCAL)
    {
        char *uri = var_InheritString(root, "library-fixture-art");
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
