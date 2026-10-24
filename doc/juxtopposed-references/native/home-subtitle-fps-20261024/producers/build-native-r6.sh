#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-subtitle-fps-20261024"
out="$base/owned-subtitle-fps-native-20261024-r6"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-native-r6.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-native-r6-exit.txt"; exit "$code"' EXIT
test ! -e "$out"
make -C "$base/win64/modules/gui/qt" -f Makefile -f "$repo/build-windows/beta-subtitle-fps-20261023/native-r1.mk" -j2 FPS_NATIVE_OUT="$out" owned-subtitle-fps-native
mkdir "$work/native-r6-bin"
cp "$out/.libs/test_qt_player_subtitle_delay.exe" "$work/native-r6-bin/"
sha256sum "$work/native-r6-bin/test_qt_player_subtitle_delay.exe"
