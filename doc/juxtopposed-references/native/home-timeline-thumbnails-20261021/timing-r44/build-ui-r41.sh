#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-timeline-thumbnails-20261021"
qa="$base/owned-timeline-ui-r41"
test ! -e "$qa"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-ui-r41.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-ui-r41-exit.txt"; exit "$code"' EXIT
make -C "$base/win64/modules/gui/qt" -f Makefile -f "$repo/modules/gui/qt/tests/native_ui/sidebar_pins_journey.mk" -j2 QA_SOURCE="$work/timing-r41.cpp" QA_OUT="$qa" owned-native-ui
mkdir -p "$work/native-ui-r41/.libs"
cp "$qa/.libs/libqt_plugin.dll" "$work/native-ui-r41/.libs/"
sha256sum "$work/native-ui-r41/.libs/libqt_plugin.dll"

