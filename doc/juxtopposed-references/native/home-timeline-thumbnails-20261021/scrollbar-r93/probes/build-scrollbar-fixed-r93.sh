#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-timeline-thumbnails-20261021"
qa="$base/owned-timeline-scrollbar-fixed-r93"
test ! -e "$qa"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-scrollbar-fixed-r93.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-scrollbar-fixed-r93-exit.txt"; exit "$code"' EXIT
make -C "$base/win64/modules/gui/qt" -f Makefile -f "$work/object-observer-r88.mk" -j2 QA_SOURCE="$repo/modules/gui/qt/tests/native_ui/scrollbar_accessibility_journey.cpp" QA_OUT="$qa" owned-object-diagnostic
mkdir -p "$work/native-scrollbar-fixed-r93"
cp "$qa/.libs/libqt_plugin.dll" "$work/native-scrollbar-fixed-r93/"
sha256sum "$work/native-scrollbar-fixed-r93/libqt_plugin.dll"
