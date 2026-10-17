/* SPDX-License-Identifier: LGPL-2.1-or-later */
#ifndef VLC_MODULES_CACHE_OUTPUT_H
#define VLC_MODULES_CACHE_OUTPUT_H
#define VLC_CACHE_OUTPUT_PROTOCOL 1u
#define VLC_CACHE_SOURCE_ENV "VLC_PLUGIN_CACHE_SOURCE"
#define VLC_CACHE_OUTPUT_ENV "VLC_PLUGIN_CACHE_OUTPUT"

/* Tool/Core protocol marker: the loader must reject an older Core before the
 * output-mode tool can initialize a bank and accidentally write in place. */
VLC_API unsigned vlc_plugin_cache_output_protocol(void);
#endif
