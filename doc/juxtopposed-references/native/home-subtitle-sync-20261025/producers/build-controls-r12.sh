#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-subtitle-sync-20261025"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-controls-r12.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-controls-r12-exit.txt"; exit "$code"' EXIT
test "$(sha256sum "$base/win64/src/.libs/libvlccore.dll" | cut -d' ' -f1)" = 9edcbb104e359ff4a0b7ff98b1a2643f2beb3a2f86a784a7115fdbbbb7689586
test ! -e "$work/controls-r12-bin"
make -C "$base/win64/test" -f Makefile -f "$repo/test/controls-native.mk" -j2 CONTROLS_OUT="$work/controls-r12-test" controls-input-native
make -C "$base/win64/modules" -f Makefile -f "$repo/modules/gui/qt/tests/native_library/controls-modules.mk" -j2 CONTROLS_OUT="$work/controls-r12-module" controls-input-module
mkdir "$work/controls-r12-bin"
cp "$work/controls-r12-test/test_controls_input.exe" "$work/controls-r12-bin/"
cp "$work/controls-r12-module/.libs/libcontrols_fixture_plugin.dll" "$work/controls-r12-bin/"
sha256sum "$work/controls-r12-bin/"*
