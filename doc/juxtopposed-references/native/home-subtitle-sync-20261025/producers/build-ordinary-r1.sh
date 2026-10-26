#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-subtitle-sync-20261025"
out="$base/owned-ordinary-subtitle-sync-20261025-r1"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-ordinary-r1.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-ordinary-r1-exit.txt"; exit "$code"' EXIT
test "$(sha256sum "$base/win64/src/.libs/libvlccore.dll" | cut -d' ' -f1)" = b62f414569c494ba866089afeae46f6cdc4c4e260d63b6a554d60c4dcfe54561
test ! -e "$out"
test ! -e "$work/ordinary-r1-bin"
make -C "$base/win64/modules/gui/qt" -f Makefile -f "$work/ordinary.mk" -j2 QA_OUT="$out" owned-ordinary-subtitle-sync
mkdir "$work/ordinary-r1-bin"
cp "$out/.libs/libqt_plugin.dll" "$work/ordinary-r1-bin/"
sha256sum "$base/win64/src/.libs/libvlccore.dll" "$work/ordinary-r1-bin/libqt_plugin.dll"
