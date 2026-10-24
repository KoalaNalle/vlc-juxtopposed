// SPDX-License-Identifier: LGPL-2.1-or-later
// Private headless window/display and interface-chain probe. Never ship it.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_window.h>
#include <vlc_vout_display.h>
#include <vlc_interface.h>
#include <vlc_configuration.h>

static int WindowEnable(vlc_window_t *window, const vlc_window_cfg_t *cfg)
{
    vlc_window_ReportSize(window, cfg->width ? cfg->width : 320,
                         cfg->height ? cfg->height : 180);
    return VLC_SUCCESS;
}
static void WindowDestroy(vlc_window_t *window)
{
    libvlc_int_t *root = vlc_object_instance(window);
    if (var_GetAddress(root, "controls-fixture-window") == window)
        var_SetAddress(root, "controls-fixture-window", NULL);
    var_Destroy(root, "controls-fixture-window");
}
static void WindowFullscreen(vlc_window_t *window, const char *id)
{
    vlc_window_ReportFullscreen(window, id);
}
static const struct vlc_window_operations window_ops = {
    .enable = WindowEnable,
    .resize = vlc_window_ReportSize,
    .destroy = WindowDestroy,
    .set_fullscreen = WindowFullscreen,
    .unset_fullscreen = vlc_window_ReportWindowed,
};
static int OpenWindow(vlc_window_t *window)
{
    window->type = VLC_WINDOW_TYPE_DUMMY;
    window->ops = &window_ops;
    window->info.has_double_click = true;
    libvlc_int_t *root = vlc_object_instance(window);
    var_Create(root, "controls-fixture-window", VLC_VAR_ADDRESS);
    var_SetAddress(root, "controls-fixture-window", window);
    return VLC_SUCCESS;
}
static void Display(vout_display_t *display, picture_t *picture)
{
    (void)picture;
    libvlc_int_t *root = vlc_object_instance(display);
    var_IncInteger(root, "controls-fixture-frames");
}
static const struct vlc_display_operations display_ops = { .display = Display };
static int OpenDisplay(vout_display_t *display, video_format_t *format,
                        vlc_video_context *context)
{
    (void)format; (void)context;
    var_Create(vlc_object_instance(display), "controls-fixture-frames", VLC_VAR_INTEGER);
    display->ops = &display_ops;
    return VLC_SUCCESS;
}
static int OpenInterface(vlc_object_t *object)
{
    intf_thread_t *intf = (intf_thread_t *)object;
    const char *const options[] = { "marker", NULL };
    config_ChainParse(intf, "controls-fixture-", options, intf->p_cfg);
    char *marker = var_GetString(intf, "controls-fixture-marker");
    libvlc_int_t *root = vlc_object_instance(intf);
    var_Create(root, "controls-fixture-markers", VLC_VAR_STRING);
    char *before = var_GetString(root, "controls-fixture-markers");
    char *after;
    if (asprintf(&after, "%s%s%s", before ? before : "",
                 before && *before ? "|" : "", marker ? marker : "") < 0)
    {
        free(before); free(marker);
        return VLC_ENOMEM;
    }
    var_SetString(root, "controls-fixture-markers", after);
    free(before); free(marker); free(after);
    return VLC_SUCCESS;
}

vlc_module_begin()
    set_description("Owned Controls display fixture")
    set_subcategory(SUBCAT_VIDEO_VOUT)
    set_callback_display(OpenDisplay, 0)
    add_shortcut("controls-fixture")
    add_submodule()
    set_description("Owned Controls window fixture")
    set_capability("vout window", 0)
    set_callback(OpenWindow)
    add_shortcut("controls-fixture")
    add_submodule()
    set_description("Owned Controls interface-chain fixture")
    set_subcategory(SUBCAT_INTERFACE_CONTROL)
    set_capability("interface", 0)
    set_callback(OpenInterface)
    add_shortcut("controls-fixture-intf")
    add_string("controls-fixture-marker", NULL, "Owned marker", "Owned test data")
vlc_module_end()
