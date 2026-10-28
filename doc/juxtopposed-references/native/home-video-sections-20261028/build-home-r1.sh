#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-home-sections-20261028"
out="$work/native-r1"
test ! -e "$out"
mkdir "$out"
exec > "$out/build.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$out/exit.txt"; exit "$code"' EXIT
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
cd "$repo"
sha256sum "$work/build-home-r1.sh" > "$out/producer.sha256"
sha256sum modules/gui/qt/medialibrary/mlvideohistory.hpp modules/gui/qt/medialibrary/mlrecentvideomodel.{hpp,cpp} modules/gui/qt/tests/test_home_activity.cpp modules/gui/qt/tests/network_stream_probe.c modules/gui/qt/Makefile.am > "$out/inputs.sha256"
make -C "$base/win64/modules/gui/qt" -j2 V=0 test_qt_home_activity.exe
sha256sum -c "$out/inputs.sha256"
sha256sum -c "$out/producer.sha256"
cp "$base/win64/modules/gui/qt/.libs/test_qt_home_activity.exe" "$out/"
sha256sum "$out/test_qt_home_activity.exe" > "$out/binary.sha256"
