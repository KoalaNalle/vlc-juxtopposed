#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-home-sections-20261028"
out="$work/history-actions-build-r1"
test ! -e "$out"
mkdir "$out"
exec > "$out/build.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$out/exit.txt"; exit "$code"' EXIT
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
cd "$repo"
sha256sum "$work/build-history-actions-r1.sh" > "$out/producer.sha256"
git rev-parse HEAD > "$out/commit.txt"
git diff --binary -- include/vlc_player.h src/libvlccore.sym src/player src/input modules/gui/qt contrib/src/medialibrary ':!modules/gui/qt/tests/*.py' > "$out/source.patch"
git ls-files -z -- include/vlc_player.h src/libvlccore.sym src/player src/input modules/gui/qt contrib/src/medialibrary ':!modules/gui/qt/tests/*.py' | xargs -0 sha256sum > "$out/inputs.sha256"
sha256sum modules/gui/qt/medialibrary/mlvideohistory.hpp contrib/src/medialibrary/swmr-priority-wake.patch >> "$out/inputs.sha256"
sha256sum test/libvlc/native_home_activity.py >> "$out/inputs.sha256"
make -C "$base/win64/src" -j2 V=0 libvlccore.la
make -C "$base/win64/modules/gui/qt" -j2 V=0 test_qt_home_activity.exe libqt_plugin.la
sha256sum -c "$out/inputs.sha256"
sha256sum -c "$out/producer.sha256"
cp "$base/win64/src/.libs/libvlccore.dll" "$out/"
cp "$base/win64/modules/gui/qt/.libs/test_qt_home_activity.exe" "$out/"
cp "$base/win64/modules/gui/qt/.libs/libqt_plugin.dll" "$out/"
sha256sum "$out/libvlccore.dll" "$out/test_qt_home_activity.exe" "$out/libqt_plugin.dll" > "$out/binaries.sha256"
