#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-timeline-thumbnails-20261021"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-startup-r47.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-startup-r47-exit.txt"; exit "$code"' EXIT
make -C "$base/win64/src" -j2 libvlccore.la
mkdir "$work/native-core-r47"
cp "$base/win64/src/.libs/libvlccore.dll" "$work/native-core-r47/"
printf '%s\n' 'test_qt_timeline_thumbnails_LDFLAGS += -Wl,--strip-debug' > "$work/backend-r47.mk"
make -C "$base/win64/modules/gui/qt" -f Makefile -f "$work/backend-r47.mk" -j2 test_qt_timeline_thumbnails.exe
mkdir "$work/native-backend-r47"
cp "$base/win64/modules/gui/qt/.libs/test_qt_timeline_thumbnails.exe" "$work/native-backend-r47/"
sha256sum "$work/native-core-r47/libvlccore.dll" "$work/native-backend-r47/test_qt_timeline_thumbnails.exe"
