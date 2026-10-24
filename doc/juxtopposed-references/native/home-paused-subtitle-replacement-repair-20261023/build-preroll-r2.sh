#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-subtitle-replacement-repair-20261023"
out="$base/owned-subtitle-preroll-core-r2"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-preroll-r2.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-preroll-r2-exit.txt"; exit "$code"' EXIT
test ! -e "$out"
make -C "$base/win64/src" -f Makefile -f "$work/candidate-preroll-r2.mk" -j2 REPAIR_CORE_OUT="$out" REPAIR_SOURCE="$work/candidate-preroll-r2" owned-subtitle-preroll-core-r2
mkdir "$work/candidate-preroll-r2-bin"
cp "$out/.libs/libvlccore.dll" "$out/decoder.d" "$out/es_out.d" "$out/player-input.d" "$work/candidate-preroll-r2-bin/"
sha256sum "$work/candidate-preroll-r2-bin/libvlccore.dll" "$base/win64/src/.libs/libvlccore.dll"
