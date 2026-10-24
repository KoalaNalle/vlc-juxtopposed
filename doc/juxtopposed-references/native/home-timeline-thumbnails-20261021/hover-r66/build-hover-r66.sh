#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-timeline-thumbnails-20261021"
qa="$base/owned-timeline-hover-r66"
test ! -e "$qa"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-hover-r66.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-hover-r66-exit.txt"; exit "$code"' EXIT
make -C "$base/win64/modules/gui/qt" -f Makefile -f "$repo/modules/gui/qt/tests/native_ui/sidebar_pins_journey.mk" -j2 QA_SOURCE="$work/hover-observer-r66.cpp" QA_OUT="$qa" owned-native-ui
mkdir -p "$work/native-hover-r66/.libs"
cp "$qa/.libs/libqt_plugin.dll" "$work/native-hover-r66/.libs/"
sha256sum "$work/native-hover-r66/.libs/libqt_plugin.dll"
