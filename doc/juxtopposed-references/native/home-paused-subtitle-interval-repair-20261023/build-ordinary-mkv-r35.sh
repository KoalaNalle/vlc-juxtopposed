#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-tracks-subtitles-20261023"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-ordinary-mkv-r35.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-ordinary-mkv-r35-exit.txt"; exit "$code"' EXIT
sha256sum "$base/win64/modules/Makefile" "$base/win64/config.h"
force_sources=()
for name in util virtual_segment matroska_segment matroska_segment_parse matroska_segment_seeker demux events Ebml_parser chapters chapter_command chapter_command_dvd chapter_command_script chapter_command_script_common stream_io_callback lzokay mkv; do
    force_sources+=(--assume-new="$repo/modules/demux/mkv/$name.cpp")
done
# Use the configured ordinary rules and ordinary object locations. Force all
# sixteen C++ sources without touching file contents or rebuilding other plugins.
make -C "$base/win64/modules" -j2 "${force_sources[@]}" libmkv_plugin.la
mkdir "$work/ordinary-mkv-r35-bin"
cp "$base/win64/modules/.libs/libmkv_plugin.dll" "$work/ordinary-mkv-r35-bin/"
cp "$base/win64/modules/demux/mkv/.deps/"libmkv_plugin_la-*.Plo "$work/ordinary-mkv-r35-bin/"
sha256sum "$base/win64/modules/demux/mkv/.libs/"libmkv_plugin_la-*.o > "$work/ordinary-mkv-r35-bin/object-hashes.txt"
sha256sum "$work/ordinary-mkv-r35-bin/libmkv_plugin.dll"
