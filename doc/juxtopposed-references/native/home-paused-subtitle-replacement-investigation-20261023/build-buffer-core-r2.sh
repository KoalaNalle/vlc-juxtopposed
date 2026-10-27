#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-subtitle-workflows-20261023"
out="$base/owned-subtitle-buffer-core-r2"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-buffer-core-r2.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-buffer-core-r2-exit.txt"; exit "$code"' EXIT
test ! -e "$out"
printf 'Ordinary Core before: '
sha256sum "$base/win64/src/.libs/libvlccore.dll"
make -C "$base/win64/src" -f Makefile -f "$work/buffer-candidate-r2.mk" -j2 CANDIDATE_CORE_OUT="$out" CANDIDATE_DECODER_SOURCE="$work/buffer-candidate-r2/decoder.c" owned-subtitle-buffer-core-r2
mkdir "$work/buffer-candidate-r2-bin"
cp "$out/.libs/libvlccore.dll" "$out/decoder.d" "$work/buffer-candidate-r2-bin/"
printf 'Candidate buffered-subtitle Core: '
sha256sum "$work/buffer-candidate-r2-bin/libvlccore.dll"
printf 'Ordinary Core after: '
sha256sum "$base/win64/src/.libs/libvlccore.dll"
