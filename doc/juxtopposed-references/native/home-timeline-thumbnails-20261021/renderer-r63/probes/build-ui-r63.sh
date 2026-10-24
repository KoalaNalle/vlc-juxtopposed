#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-timeline-thumbnails-20261021"
qa="$base/owned-timeline-ui-r63"
test ! -e "$qa"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-ui-r63.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-ui-r63-exit.txt"; exit "$code"' EXIT
make -C "$base/win64/modules/gui/qt" -f Makefile -f "$repo/modules/gui/qt/tests/native_ui/sidebar_pins_journey.mk" -j2 QA_SOURCE="$work/timing-r42.cpp" QA_OUT="$qa" owned-native-ui
mkdir -p "$work/native-ui-r63/.libs"
cp "$qa/.libs/libqt_plugin.dll" "$work/native-ui-r63/.libs/"
sha256sum "$work/native-ui-r63/.libs/libqt_plugin.dll"


printf '%s\n' 'libqt_plugin_la_LDFLAGS += -Wl,--strip-debug' > "$work/ordinary-r63.mk"
make -C "$base/win64/modules/gui/qt" -f Makefile -f "$work/ordinary-r63.mk" -j2 libqt_plugin.la
mkdir "$work/ordinary-r63"
cp "$base/win64/modules/gui/qt/.libs/libqt_plugin.dll" "$work/ordinary-r63/"
sha256sum "$work/ordinary-r63/libqt_plugin.dll"
