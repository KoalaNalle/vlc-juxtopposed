#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-timeline-thumbnails-20261021"
qa="$base/owned-timeline-scrollbar-r91"
test ! -e "$qa"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-scrollbar-r91.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-scrollbar-r91-exit.txt"; exit "$code"' EXIT
make -C "$base/win64/modules/gui/qt" -f Makefile -f "$work/object-observer-r88.mk" -j2 QA_SOURCE="$work/scrollbar-regression-r91.cpp" QA_OUT="$qa" owned-object-diagnostic
mkdir -p "$work/native-scrollbar-r91"
cp "$qa/.libs/libqt_plugin.dll" "$work/native-scrollbar-r91/"
sha256sum "$work/native-scrollbar-r91/libqt_plugin.dll"
