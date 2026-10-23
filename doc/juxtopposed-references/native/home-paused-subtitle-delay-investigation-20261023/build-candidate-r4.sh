#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-subtitle-preferences-20261023"
out="$base/owned-subtitle-delay-core-r4"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-candidate-r4.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-candidate-r4-exit.txt"; exit "$code"' EXIT
test "$(sha256sum "$base/win64/src/.libs/libvlccore.dll" | cut -d' ' -f1)" = 0ef9dde2f0a221135cab0a548ac292dd89d7cc9b7b79427eaaba121995b7773f
test ! -e "$out"
make -C "$base/win64/src" -f Makefile -f "$work/candidate-r4.mk" -j2 \
    DELAY_CORE_OUT="$out" DELAY_SOURCE="$work/candidate-r4" owned-subtitle-delay-core-r4
mkdir "$work/candidate-r4-bin"
cp "$out/.libs/libvlccore.dll" "$out/decoder.d" "$out/es_out.d" "$out/input.d" "$out/player_input.d" "$out/player.d" "$work/candidate-r4-bin/"
sha256sum "$work/candidate-r4-bin/libvlccore.dll" "$base/win64/src/.libs/libvlccore.dll"
