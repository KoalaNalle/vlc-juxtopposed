#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-caption-accessibility-20261023"
out="$base/owned-ordinary-accessibility-r3"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-ordinary-r3.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-ordinary-r3-exit.txt"; exit "$code"' EXIT
test ! -e "$out"
make -C "$base/win64/modules/gui/qt" -f Makefile -f "$work/ordinary-accessibility-r3.mk" -j2 QA_OUT="$out" owned-ordinary-accessibility-r3
mkdir "$work/ordinary-r3-bin"
cp "$out/.libs/libqt_plugin.dll" "$work/ordinary-r3-bin/"
sha256sum "$work/ordinary-r3-bin/libqt_plugin.dll"
