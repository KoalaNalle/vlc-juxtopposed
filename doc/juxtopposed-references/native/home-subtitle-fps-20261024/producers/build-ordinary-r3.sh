#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-subtitle-fps-20261024"
out="$base/owned-ordinary-subtitle-fps-20261024-r3"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-ordinary-r3.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-ordinary-r3-exit.txt"; exit "$code"' EXIT
test "$(sha256sum "$base/win64/src/.libs/libvlccore.dll" | cut -d' ' -f1)" = 7ab8075014b11a72c5b51e41e0e067fcb8966505b19d23614d0c5ddc1f4ba53f
test ! -e "$out"
test ! -e "$work/ordinary-r3-bin"
make -C "$base/win64/src" -j2 libvlccore.la
make -C "$base/win64/modules/gui/qt" -f Makefile -f "$repo/build-windows/beta-subtitle-fps-20261023/ordinary-r5.mk" -j2 QA_OUT="$out" owned-ordinary-subtitle-fps-r5
mkdir "$work/ordinary-r3-bin"
cp "$base/win64/src/.libs/libvlccore.dll" "$work/ordinary-r3-bin/"
cp "$out/.libs/libqt_plugin.dll" "$work/ordinary-r3-bin/"
cp "$base/win64/src/input/.deps/libvlccore_la-input.Plo" "$work/ordinary-r3-bin/input.Plo"
cp "$base/win64/src/player/.deps/libvlccore_la-player.Plo" "$work/ordinary-r3-bin/player.Plo"
sha256sum "$work/ordinary-r3-bin/libvlccore.dll" "$work/ordinary-r3-bin/libqt_plugin.dll"
