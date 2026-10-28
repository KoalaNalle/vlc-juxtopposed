#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-home-sections-20261028"
previous="$work/history-actions-build-r1"
out="$work/history-actions-test-build-r2"
test ! -e "$out"
mkdir "$out"
exec > "$out/build.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$out/exit.txt"; exit "$code"' EXIT
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
cd "$repo"
test "$(cat "$previous/exit.txt")" = 0
sha256sum "$work/build-history-actions-tests-r2.sh" > "$out/producer.sha256"
sed '\#  modules/gui/qt/tests/test_home_activity.cpp$#d' "$previous/inputs.sha256" > "$out/unchanged-inputs.sha256"
sha256sum -c "$out/unchanged-inputs.sha256"
cat "$out/unchanged-inputs.sha256" > "$out/inputs.sha256"
sha256sum modules/gui/qt/tests/test_home_activity.cpp >> "$out/inputs.sha256"
make -C "$base/win64/modules/gui/qt" -j2 V=0 test_qt_home_activity.exe
sha256sum -c "$out/inputs.sha256"
sha256sum -c "$out/producer.sha256"
ln "$previous/libvlccore.dll" "$out/libvlccore.dll"
ln "$previous/libqt_plugin.dll" "$out/libqt_plugin.dll"
cp "$base/win64/modules/gui/qt/.libs/test_qt_home_activity.exe" "$out/"
sha256sum "$out/libvlccore.dll" "$out/test_qt_home_activity.exe" "$out/libqt_plugin.dll" > "$out/binaries.sha256"
