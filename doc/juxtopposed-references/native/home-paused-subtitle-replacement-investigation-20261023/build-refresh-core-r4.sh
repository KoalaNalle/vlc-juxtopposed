#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-subtitle-workflows-20261023"
out="$base/owned-subtitle-refresh-core-r4"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-refresh-core-r4.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-refresh-core-r4-exit.txt"; exit "$code"' EXIT
test ! -e "$out"
printf 'Ordinary Core before: '
sha256sum "$base/win64/src/.libs/libvlccore.dll"
make -C "$base/win64/src" -f Makefile -f "$work/refresh-candidate-r4.mk" -j2 CANDIDATE_CORE_OUT="$out" CANDIDATE_INPUT_SOURCE="$work/refresh-candidate-r4/input.c" owned-subtitle-refresh-core-r4
mkdir "$work/refresh-candidate-r4-bin"
cp "$out/.libs/libvlccore.dll" "$out/player-input.d" "$work/refresh-candidate-r4-bin/"
printf 'Candidate paused subtitle refresh Core: '
sha256sum "$work/refresh-candidate-r4-bin/libvlccore.dll"
printf 'Ordinary Core after: '
sha256sum "$base/win64/src/.libs/libvlccore.dll"
