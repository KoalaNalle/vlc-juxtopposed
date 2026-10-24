#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-subtitle-frame-feedback-20261023"
out="$base/owned-frame-feedback-core-r2"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-candidate-r2.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-candidate-r2-exit.txt"; exit "$code"' EXIT
test "$(sha256sum "$base/win64/src/.libs/libvlccore.dll" | cut -d' ' -f1)" = ea14300c8c8366a99d17eab124216f543287822c18a85ed305eea0cc557b23d4
test ! -e "$out"
make -C "$base/win64/src" -f Makefile -f "$work/candidate-r2.mk" -j2 \
    FEEDBACK_CORE_OUT="$out" FEEDBACK_SOURCE="$work/candidate-r2" owned-frame-feedback-core-r2
mkdir "$work/candidate-r2-bin"
cp "$out/.libs/libvlccore.dll" "$out/decoder.d" "$work/candidate-r2-bin/"
sha256sum "$work/candidate-r2-bin/libvlccore.dll" "$base/win64/src/.libs/libvlccore.dll"
