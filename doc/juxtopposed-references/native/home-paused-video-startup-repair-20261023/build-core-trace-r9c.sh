#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-tracks-subtitles-20261023"
out="$base/owned-decoder-trace-core-r9c"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-core-trace-r9c.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-core-trace-r9c-exit.txt"; exit "$code"' EXIT
test ! -e "$out"
printf 'Ordinary Core before: '
sha256sum "$base/win64/src/.libs/libvlccore.dll"
make -C "$base/win64/src" -f Makefile -f "$work/core-trace-r9c.mk" -j2 TRACE_CORE_OUT="$out" TRACE_DECODER_SOURCE="$work/core-trace-r9c-source/decoder.c" owned-decoder-trace-core-r9c
mkdir "$work/core-trace-r9c-bin"
cp "$out/.libs/libvlccore.dll" "$out/decoder.d" "$work/core-trace-r9c-bin/"
printf 'Private trace Core: '
sha256sum "$work/core-trace-r9c-bin/libvlccore.dll"
printf 'Ordinary Core after: '
sha256sum "$base/win64/src/.libs/libvlccore.dll"
