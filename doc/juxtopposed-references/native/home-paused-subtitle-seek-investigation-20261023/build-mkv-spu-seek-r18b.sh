#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-tracks-subtitles-20261023"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-mkv-spu-seek-r18b.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-mkv-spu-seek-r18b-exit.txt"; exit "$code"' EXIT
printf 'Ordinary MKV before: '
sha256sum "$base/win64/modules/.libs/libmkv_plugin.dll"
sha256sum "$base/win64/modules/Makefile" "$base/win64/config.h"
for phase in before after; do
    out="$base/owned-mkv-spu-seek-$phase-r18b"
    test ! -e "$out"
    make -C "$base/win64/modules" -f Makefile -f "$work/mkv-spu-seek-r18b.mk" -j2 MKV_PRIVATE_OUT="$out" MKV_PRIVATE_SOURCE="$work/mkv-spu-seek-r18b-source/matroska_segment-$phase.cpp" owned-mkv-spu-seek-r18b
    mkdir "$work/mkv-spu-seek-$phase-r18b-bin"
    cp "$out/.libs/libmkv_plugin.dll" "$out/matroska_segment.d" "$work/mkv-spu-seek-$phase-r18b-bin/"
    sha256sum "$work/mkv-spu-seek-$phase-r18b-bin/libmkv_plugin.dll"
done
printf 'Ordinary MKV after: '
sha256sum "$base/win64/modules/.libs/libmkv_plugin.dll"
