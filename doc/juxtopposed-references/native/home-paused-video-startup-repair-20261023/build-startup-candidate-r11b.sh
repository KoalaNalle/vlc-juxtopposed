#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-tracks-subtitles-20261023"
out="$base/owned-startup-candidate-core-r11b"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-startup-candidate-r11b.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-startup-candidate-r11b-exit.txt"; exit "$code"' EXIT
test ! -e "$out"
printf 'Ordinary Core before: '
sha256sum "$base/win64/src/.libs/libvlccore.dll"
make -C "$base/win64/src" -f Makefile -f "$work/startup-candidate-r11b.mk" -j2 CANDIDATE_CORE_OUT="$out" CANDIDATE_DECODER_SOURCE="$work/startup-candidate-r11b-source/decoder.c" owned-startup-candidate-core-r11b
mkdir "$work/startup-candidate-r11b-bin"
cp "$out/.libs/libvlccore.dll" "$out/decoder.d" "$work/startup-candidate-r11b-bin/"
printf 'Private candidate Core: '
sha256sum "$work/startup-candidate-r11b-bin/libvlccore.dll"
printf 'Ordinary Core after: '
sha256sum "$base/win64/src/.libs/libvlccore.dll"
