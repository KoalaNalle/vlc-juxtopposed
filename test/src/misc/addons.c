// SPDX-License-Identifier: LGPL-2.1-or-later
// Actual Core manager and filesystem/repository modules, in an owned runtime.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include <vlc_addons.h>
#include <vlc/vlc.h>
#include <vlc_fs.h>
#include <vlc_url.h>
#include <vlc_configuration.h>
#ifdef _WIN32
# include <vlc_charset.h>
#endif
#include "../../../lib/libvlc_internal.h"

static unsigned checks, failures;
static void check(bool ok, const char *name)
{
    printf("%s %u %s\n", ok ? "ok" : "not ok", ++checks, name);
    fflush(stdout); failures += !ok;
}
struct context {
    vlc_mutex_t lock;
    vlc_cond_t changed;
    addon_entry_t *entry;
    unsigned discoveries, terminal_changes;
    bool remove_after_install, inspect_pending_catalog, pending_catalog_preserved;
};
static void found(addons_manager_t *manager, addon_entry_t *entry)
{
    struct context *ctx=manager->owner.sys;
    vlc_mutex_lock(&ctx->lock);
    if (!ctx->entry) ctx->entry=addon_entry_Hold(entry);
    vlc_mutex_unlock(&ctx->lock);
}
static void discovered(addons_manager_t *manager)
{
    struct context *ctx=manager->owner.sys;
    vlc_mutex_lock(&ctx->lock);ctx->discoveries++;
    vlc_cond_signal(&ctx->changed);vlc_mutex_unlock(&ctx->lock);
}
static void changed(addons_manager_t *manager, addon_entry_t *entry)
{
    struct context *ctx=manager->owner.sys;
    vlc_mutex_lock(&entry->lock);
    addon_state_t current=entry->e_state;
    bool terminal=current==ADDON_INSTALLED || current==ADDON_NOTINSTALLED;
    vlc_mutex_unlock(&entry->lock);
    if (current==ADDON_INSTALLED && ctx->remove_after_install) {
        ctx->remove_after_install=false;
        /* Real reentrant admission: this queues Remove before the worker
         * publishes the just-completed installation's catalog. */
        ctx->inspect_pending_catalog=addons_manager_Remove(manager,entry->uuid)==VLC_SUCCESS;
    } else if (current==ADDON_UNINSTALLING && ctx->inspect_pending_catalog) {
        char *dir=config_GetUserDir(VLC_USERDATA_DIR),*path=NULL;
        if (dir && asprintf(&path,"%s/catalog.xml",dir)>=0) {
            FILE *file=vlc_fopen(path,"rb");
            if(file) {
                char contents[8192];size_t size=fread(contents,1,sizeof(contents)-1,file);
                contents[size]='\0';
                ctx->pending_catalog_preserved=strstr(contents,"owned-beta-fixture.lua")!=NULL;
                fclose(file);
            }
        }
        free(path);free(dir);
    }
    if (terminal) {
        vlc_mutex_lock(&ctx->lock);ctx->terminal_changes++;
        vlc_cond_signal(&ctx->changed);vlc_mutex_unlock(&ctx->lock);
    }
}
static unsigned count(struct context *ctx, bool discovery)
{
    vlc_mutex_lock(&ctx->lock);
    unsigned value=discovery ? ctx->discoveries : ctx->terminal_changes;
    vlc_mutex_unlock(&ctx->lock);return value;
}
static bool wait_count(struct context *ctx, bool discovery, unsigned wanted)
{
    vlc_tick_t end=vlc_tick_now()+VLC_TICK_FROM_SEC(8);
    vlc_mutex_lock(&ctx->lock);
    unsigned *value=discovery ? &ctx->discoveries : &ctx->terminal_changes;
    while(*value<wanted)
        if(vlc_cond_timedwait(&ctx->changed,&ctx->lock,end)) break;
    bool ready=*value>=wanted;
    vlc_mutex_unlock(&ctx->lock);return ready;
}
static addon_state_t state(addon_entry_t *entry)
{
    vlc_mutex_lock(&entry->lock);addon_state_t result=entry->e_state;
    vlc_mutex_unlock(&entry->lock);return result;
}
int main(int argc, char **argv)
{
    if (argc!=3) return 2;
    const char *args[]={"--quiet","--no-media-library","--no-metadata-network-access",
        "--no-auto-preparse","--extraintf=","--control="};
    libvlc_instance_t *instance=libvlc_new(ARRAY_SIZE(args),args);
    if (!instance) return 3;
#ifdef _WIN32
    const wchar_t *expected=_wgetenv(L"VLC_USERDATA_PATH");
    char *expected_utf8=expected ? FromWide(expected) : NULL;
    char *actual=config_GetUserDir(VLC_USERDATA_DIR);
    check(expected_utf8 && actual && !strcmp(expected_utf8,actual),
        "user-data override preserves the exact Windows Unicode path");
    free(actual);free(expected_utf8);
#endif
    struct context ctx={0};vlc_mutex_init(&ctx.lock);vlc_cond_init(&ctx.changed);
    ctx.remove_after_install=!strcmp(argv[1],"queued-remove-catalog");
    struct addons_manager_owner owner={&ctx,found,discovered,changed};
    addons_manager_t *manager=addons_manager_New(VLC_OBJECT(instance->p_libvlc_int),&owner);
    check(manager!=NULL,"actual addons manager created");
    if (!manager) {libvlc_release(instance);return 1;}
    bool from_catalog=!strncmp(argv[1],"catalog-",8);
    bool ready;
    if (from_catalog) {
        ready=addons_manager_LoadCatalog(manager)==VLC_SUCCESS;
        check(ready,"actual native catalog loader finishes");
    } else {
        char *uri=vlc_path2uri(argv[2],NULL);
        check(uri!=NULL,"owned package converted to native URI");
        if (uri) {addons_manager_Gather(manager,uri);free(uri);}
        ready=wait_count(&ctx,true,1);
        check(ready,"actual repository discovery finishes within deadline");
    }
    if (!strcmp(argv[1],"catalog-empty")) {
        check(ctx.entry==NULL,"successful removal leaves no managed or unmanaged script");
        goto done;
    }
    check(ctx.entry!=NULL,"actual package or catalog produces an add-on");
    if (!ready || !ctx.entry) goto done;
    addon_uuid_t uuid;memcpy(uuid,ctx.entry->uuid,sizeof(uuid));
    check((ctx.entry->e_flags & ADDON_MANAGEABLE)!=0,"entry remains managed by native catalog");
    addon_uuid_t unknown={0};
    check(addons_manager_Install(manager,unknown)!=VLC_SUCCESS,"unknown Install is rejected");
    check(addons_manager_Remove(manager,unknown)!=VLC_SUCCESS,"unknown Remove is rejected");
    check(state(ctx.entry)==(from_catalog ? ADDON_INSTALLED : ADDON_NOTINSTALLED),
        "entry has expected initial installed state");
    if (from_catalog) {
        if (!strcmp(argv[1],"catalog-remove") || !strcmp(argv[1],"catalog-locked-remove")) {
            unsigned before=count(&ctx,false);
            check(addons_manager_Remove(manager,uuid)==VLC_SUCCESS,"catalog removal request admitted");
            check(wait_count(&ctx,false,before+1),"catalog removal finishes within deadline");
            bool locked=!strcmp(argv[1],"catalog-locked-remove");
            check(state(ctx.entry)==(locked ? ADDON_INSTALLED : ADDON_NOTINSTALLED),
                locked ? "failed removal retains installed state" : "catalog removal reaches not-installed state");
        }
        goto done;
    }
    if (!strcmp(argv[1],"remove-first")) {
        unsigned before=count(&ctx,false);
        int result=addons_manager_Remove(manager,uuid);
        check(result!=VLC_SUCCESS,"Remove never installs a not-installed add-on");
        if (result==VLC_SUCCESS) wait_count(&ctx,false,before+1);
        check(state(ctx.entry)==ADDON_NOTINSTALLED,"rejected Remove preserves not-installed state");
    } else {
        unsigned before=count(&ctx,false);
        check(addons_manager_Install(manager,uuid)==VLC_SUCCESS,"native install request admitted");
        if (!strcmp(argv[1],"queued-install")) {
            check(addons_manager_Install(manager,uuid)!=VLC_SUCCESS,"repeated queued Install is rejected");
        }
        if (!strcmp(argv[1],"delete-pending")) goto done;
        if (!strcmp(argv[1],"queued-remove-catalog")) {
            check(wait_count(&ctx,false,before+2),"reentrant install/remove finishes within deadline");
            check(ctx.pending_catalog_preserved,"catalog retains files until queued removal runs");
            check(state(ctx.entry)==ADDON_NOTINSTALLED,"reentrant removal finishes not installed");
            goto done;
        }
        check(wait_count(&ctx,false,before+1),"native installer finishes within deadline");
        check(state(ctx.entry)==ADDON_INSTALLED,"actual package installs successfully");
        if (state(ctx.entry)!=ADDON_INSTALLED) goto done;
        if (!strcmp(argv[1],"install-only")) goto done;
        if (!strcmp(argv[1],"install-twice")) {
            before=count(&ctx,false);
            int result=addons_manager_Install(manager,uuid);
            check(result!=VLC_SUCCESS,"Install never uninstalls an installed add-on");
            if (result==VLC_SUCCESS) wait_count(&ctx,false,before+1);
            check(state(ctx.entry)==ADDON_INSTALLED,"repeated Install preserves installed state");
        } else {
            before=count(&ctx,false);
            check(addons_manager_Remove(manager,uuid)==VLC_SUCCESS,"native removal request admitted");
            if (!strcmp(argv[1],"queued-install"))
                check(addons_manager_Remove(manager,uuid)!=VLC_SUCCESS,"repeated queued Remove is rejected");
            check(wait_count(&ctx,false,before+1),"native removal finishes within deadline");
            check(state(ctx.entry)==ADDON_NOTINSTALLED,"native removal reaches not-installed state");
        }
    }
done:
    int final_state=-1;
    if (ctx.entry) final_state=state(ctx.entry);
    printf("observed final state %d\n",final_state);
    addons_manager_Delete(manager);
    if (ctx.entry) addon_entry_Release(ctx.entry);
    libvlc_release(instance);
    printf("checks %u failures %u\n",checks,failures);fflush(stdout);
    return failures ? 1 : 0;
}
