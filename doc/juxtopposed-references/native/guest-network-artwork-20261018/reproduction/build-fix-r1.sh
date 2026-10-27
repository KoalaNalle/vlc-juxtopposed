#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/e/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-network-artwork-20261018"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
export JOBS=1 MESON_BUILD='-j 1'
unset CC CXX
exec > "$work/build-fix-r1.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-fix-r1-exit.txt"; exit "$code"' EXIT
make -C "$base/win64/modules/gui/qt" -f Makefile -f "$work/ordinary-strip-debug.mk" -j1 libqt_plugin.la
mkdir "$work/ordinary-native-r1"
cp "$base/win64/modules/gui/qt/.libs/libqt_plugin.dll" "$work/ordinary-native-r1/"
mkdir "$work/native-ui-r1"
cp "$repo/build-windows/beta-sidebar-pins-20261018/native-ui-r7/native-ui-journey.o" "$work/native-ui-r1/"
make -C "$base/win64/modules/gui/qt" -f Makefile -f "$repo/modules/gui/qt/tests/native_ui/sidebar_pins_journey.mk" -j1 QA_OUT="$work/native-ui-r1" owned-native-ui
