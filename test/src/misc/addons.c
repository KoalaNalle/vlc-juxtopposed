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
    unsigned found_count;
    int discovery_result, storage_result, catalog_result;
    bool remove_after_install, inspect_pending_catalog, pending_catalog_preserved;
    bool lock_entry_on_found;
};
static void found(addons_manager_t *manager, addon_entry_t *entry)
{
    struct context *ctx=manager->owner.sys;
    if (ctx->lock_entry_on_found) {
        vlc_mutex_lock(&entry->lock);
        vlc_mutex_unlock(&entry->lock);
    }
    vlc_mutex_lock(&ctx->lock);
    ctx->found_count++;
    if (!ctx->entry) ctx->entry=addon_entry_Hold(entry);
    vlc_mutex_unlock(&ctx->lock);
}
static void discovered(addons_manager_t *manager)
{
    struct context *ctx=manager->owner.sys;
    int result=addons_manager_GetDiscoveryResult(manager);
    vlc_mutex_lock(&ctx->lock);ctx->discoveries++;ctx->discovery_result=result;
    vlc_cond_signal(&ctx->changed);vlc_mutex_unlock(&ctx->lock);
}
static void changed(addons_manager_t *manager, addon_entry_t *entry)
{
    struct context *ctx=manager->owner.sys;
    vlc_mutex_lock(&entry->lock);
    addon_state_t current=entry->e_state;
    vlc_mutex_unlock(&entry->lock);
    int storage_result, catalog_result;
    bool terminal=addon_entry_GetOperationResult(entry,&storage_result,&catalog_result);
    if (current==ADDON_INSTALLED && ctx->remove_after_install) {
        ctx->remove_after_install=false;
        /* Re-enter immediately on completion. The installed catalog must
         * already be visible when the queued Remove starts. */
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
        ctx->storage_result=storage_result;ctx->catalog_result=catalog_result;
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
    if (argc!=3 && argc!=4) return 2;
    addon_uuid_t standard, legacy;
    check(addons_uuid_read("8beaeec8-7b16-45f9-83c0-a0ecc57d0401",&standard),"canonical UUID parses");
    check(addons_uuid_read("8beaeec8-7b1645-f983-c0a0-ecc57d0401",&legacy) &&
        !memcmp(standard,legacy,sizeof(standard)),"older VLC catalog UUID remains compatible");
    char *canonical=addons_uuid_to_psz(&legacy);
    check(canonical && !strcmp(canonical,"8beaeec8-7b16-45f9-83c0-a0ecc57d0401"),"new catalog writes canonical UUID separators");
    free(canonical);
    check(!addons_uuid_read("8beaeec8-7b16-45f9-83c0-a0ecc57d040Z",&legacy),"nonhex UUID is rejected");
    check(!addons_uuid_read("8beaeec8-7b16-45f9-83c0-a0ecc57d0401extra",&legacy),"UUID suffix is rejected");
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
    ctx.lock_entry_on_found=!strcmp(argv[1],"reentrant-found");
    struct addons_manager_owner owner={&ctx,found,discovered,changed};
    addons_manager_t *manager=addons_manager_New(VLC_OBJECT(instance->p_libvlc_int),&owner);
    check(manager!=NULL,"actual addons manager created");
    if (!manager) {libvlc_release(instance);return 1;}
    check(addons_manager_GatherChecked(manager,NULL)!=VLC_SUCCESS,"null discovery request is rejected");
    check(addons_manager_GatherChecked(manager,"")!=VLC_SUCCESS,"empty discovery request is rejected");
    bool save_retry=!strcmp(argv[1],"catalog-save-retry");
    bool save_failure=!strcmp(argv[1],"catalog-save-failure") || save_retry;
    bool from_catalog=!strncmp(argv[1],"catalog-",8) && !save_failure;
#ifdef _WIN32
    HANDLE catalog_lock=INVALID_HANDLE_VALUE;
    if (save_retry) {
        char *dir=config_GetUserDir(VLC_USERDATA_DIR),*path=NULL;
        if (dir && asprintf(&path,"%s/catalog.xml",dir)>=0) {
            wchar_t *wide=ToWide(path);
            if (wide) catalog_lock=CreateFileW(wide,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
            free(wide);
        }
        free(path);free(dir);
        check(catalog_lock!=INVALID_HANDLE_VALUE,"owned Windows handle denies catalog publication");
    }
#endif
    bool ready;
    if (from_catalog) {
        int result=addons_manager_LoadCatalog(manager);
        if (!strcmp(argv[1],"catalog-invalid")) {
            check(result!=VLC_SUCCESS,"invalid native catalog reports its failure");
            check(ctx.entry==NULL,"invalid catalog publishes no partial managed entry");
            goto done;
        }
        ready=result==VLC_SUCCESS;
        check(ready,"actual native catalog loader finishes");
    } else {
        char *uri=!strncmp(argv[2],"file://",7) ? strdup(argv[2]) : vlc_path2uri(argv[2],NULL);
        check(uri!=NULL,"owned package converted to native URI");
        if (uri) {
            check(addons_manager_GatherChecked(manager,uri)==VLC_SUCCESS,"actual discovery request admitted");
            free(uri);
        }
        ready=wait_count(&ctx,true,1);
        check(ready,"actual repository discovery finishes within deadline");
        if (!strcmp(argv[1],"discover-invalid")) {
            check(ctx.discovery_result!=VLC_SUCCESS,"actual provider failure reaches its completion callback");
            check(ctx.entry==NULL,"failed discovery publishes no partial add-on");
            goto done;
        }
        check(ctx.discovery_result==VLC_SUCCESS,"actual provider success reaches its completion callback");
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
            check((ctx.storage_result==VLC_SUCCESS)!=locked,"removal completion exposes actual storage result");
            check(ctx.catalog_result==VLC_SUCCESS,"removal completion includes catalog publication");
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
        if (!strcmp(argv[1],"install-failure")) {
            check(ctx.storage_result!=VLC_SUCCESS,"failed installation reports its storage failure");
            check(state(ctx.entry)==ADDON_NOTINSTALLED,"failed installation remains not installed");
            check(ctx.catalog_result==VLC_SUCCESS,"failure completion includes catalog publication");
            goto done;
        }
        check(state(ctx.entry)==ADDON_INSTALLED,"actual package installs successfully");
        check(ctx.storage_result==VLC_SUCCESS,"install completion exposes successful storage");
        bool catalog_locked=save_failure;
        check((ctx.catalog_result==VLC_SUCCESS)!=catalog_locked,
            catalog_locked ? "catalog publication failure is distinct from installed storage" : "catalog publication succeeds before completion");
        if (state(ctx.entry)!=ADDON_INSTALLED) goto done;
        if (!strcmp(argv[1],"replace-source") && argc==4) {
            char *next=!strncmp(argv[3],"file://",7) ? strdup(argv[3]) : vlc_path2uri(argv[3],NULL);
            check(next && addons_manager_GatherChecked(manager,next)==VLC_SUCCESS,"new local version discovery admitted");
            free(next);
            check(wait_count(&ctx,true,2),"new local version discovery finishes");
            check(ctx.found_count==1,"duplicate package keeps one stable native entry");
            vlc_mutex_lock(&ctx.entry->lock);
            bool installed_old=!strcmp(ctx.entry->psz_version,"1.0") && (ctx.entry->e_flags & ADDON_UPDATABLE);
            vlc_mutex_unlock(&ctx.entry->lock);
            check(installed_old,"installed version remains truthful while replacement is available");
            before=count(&ctx,false);
            check(addons_manager_Remove(manager,uuid)==VLC_SUCCESS,"old version removal admitted");
            check(wait_count(&ctx,false,before+1),"old version removal finishes");
            vlc_mutex_lock(&ctx.entry->lock);
            bool next_available=ctx.entry->e_state==ADDON_NOTINSTALLED && !strcmp(ctx.entry->psz_version,"1.1");
            vlc_mutex_unlock(&ctx.entry->lock);
            check(next_available,"removed entry adopts the selected local package descriptor");
            before=count(&ctx,false);
            check(addons_manager_Install(manager,uuid)==VLC_SUCCESS,"replacement installation admitted");
            check(wait_count(&ctx,false,before+1) && state(ctx.entry)==ADDON_INSTALLED,"selected replacement installs through the stable UUID");
            goto done;
        }
        if (save_retry) {
#ifdef _WIN32
            check(CloseHandle(catalog_lock)!=0,"owned catalog lock closes normally");
            catalog_lock=INVALID_HANDLE_VALUE;
#endif
            check(addons_manager_SaveCatalog(manager)==VLC_SUCCESS,"catalog retry publishes existing installed state without reinstalling");
        }
        if (!strcmp(argv[1],"install-only") || catalog_locked) goto done;
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
#ifdef _WIN32
    if (catalog_lock!=INVALID_HANDLE_VALUE) CloseHandle(catalog_lock);
#endif
    int final_state=-1;
    if (ctx.entry) final_state=state(ctx.entry);
    printf("observed final state %d\n",final_state);
    addons_manager_Delete(manager);
    if (ctx.entry) addon_entry_Release(ctx.entry);
    libvlc_release(instance);
    printf("checks %u failures %u\n",checks,failures);fflush(stdout);
    return failures ? 1 : 0;
}
