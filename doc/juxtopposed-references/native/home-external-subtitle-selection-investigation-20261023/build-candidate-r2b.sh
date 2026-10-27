#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-external-subtitle-selection-20261023"
out="$base/owned-external-subtitle-core-r2"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-candidate-r2.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-candidate-r2-exit.txt"; exit "$code"' EXIT
test ! -e "$out"
make -C "$base/win64/src" -f Makefile -f "$work/candidate-r1.mk" -j2 EXTERNAL_CORE_OUT="$out" EXTERNAL_SOURCE="$work/candidate-r2" owned-external-subtitle-core-r1
mkdir "$work/candidate-r2-bin"
cp "$out/.libs/libvlccore.dll" "$out/input.d" "$out/es_out.d" "$work/candidate-r2-bin/"
sha256sum "$work/candidate-r2-bin/libvlccore.dll" "$base/win64/src/.libs/libvlccore.dll"
