#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-tracks-subtitles-20261023"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-mkv-interval-r23b.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-mkv-interval-r23b-exit.txt"; exit "$code"' EXIT
printf 'Ordinary MKV before: '
sha256sum "$base/win64/modules/.libs/libmkv_plugin.dll"
sha256sum "$base/win64/modules/Makefile" "$base/win64/config.h"
for phase in before after; do
    out="$base/owned-mkv-interval-$phase-r23b"
    test ! -e "$out"
    make -C "$base/win64/modules" -f Makefile -f "$work/mkv-interval-r23b.mk" -j2 MKV_INTERVAL_OUT="$out" MKV_INTERVAL_SOURCE="$work/mkv-interval-r23b-source/$phase" owned-mkv-interval-r23b
    mkdir "$work/mkv-interval-$phase-r23b-bin"
    cp "$out/.libs/libmkv_plugin.dll" "$out/"*.d "$work/mkv-interval-$phase-r23b-bin/"
    sha256sum "$work/mkv-interval-$phase-r23b-bin/libmkv_plugin.dll"
done
printf 'Ordinary MKV after: '
sha256sum "$base/win64/modules/.libs/libmkv_plugin.dll"
