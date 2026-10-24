#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-timeline-thumbnails-20261021"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-formats-r53.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-formats-r53-exit.txt"; exit "$code"' EXIT
printf '%s\n' 'test_qt_timeline_thumbnails_LDFLAGS += -Wl,--strip-debug' > "$work/backend-r53.mk"
make -C "$base/win64/modules/gui/qt" -f Makefile -f "$work/backend-r53.mk" -j2 test_qt_timeline_thumbnails.exe
mkdir "$work/native-backend-r53"
cp "$base/win64/modules/gui/qt/.libs/test_qt_timeline_thumbnails.exe" "$work/native-backend-r53/"
sha256sum "$work/native-core-r49/libvlccore.dll" "$work/native-backend-r53/test_qt_timeline_thumbnails.exe"
