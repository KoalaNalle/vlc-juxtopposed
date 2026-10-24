#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-tracks-subtitles-20261023"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-ordinary-mkv-r35b.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-ordinary-mkv-r35b-exit.txt"; exit "$code"' EXIT
sha256sum "$base/win64/modules/Makefile" "$base/win64/config.h"
configured_src=$(sed -n 's/^srcdir = //p' "$base/win64/modules/Makefile")
test "$configured_src" = "$repo/extras/package/win32/../../../modules"
force_sources=()
for name in util virtual_segment matroska_segment matroska_segment_parse matroska_segment_seeker demux events Ebml_parser chapters chapter_command chapter_command_dvd chapter_command_script chapter_command_script_common stream_io_callback lzokay mkv; do
    force_sources+=(--assume-new="$configured_src/demux/mkv/$name.cpp")
done
make -C "$base/win64/modules" -f Makefile -f "$work/ordinary-mkv-dependencies-r35b.mk" V=1 -j2 "${force_sources[@]}" libmkv_plugin.la
mkdir "$work/ordinary-mkv-r35b-bin"
cp "$base/win64/modules/.libs/libmkv_plugin.dll" "$work/ordinary-mkv-r35b-bin/"
cp "$base/win64/modules/demux/mkv/"*.ordinary-r35b.d "$work/ordinary-mkv-r35b-bin/"
sha256sum "$base/win64/modules/demux/mkv/.libs/"libmkv_plugin_la-*.o > "$work/ordinary-mkv-r35b-bin/object-hashes.txt"
sha256sum "$work/ordinary-mkv-r35b-bin/libmkv_plugin.dll"
