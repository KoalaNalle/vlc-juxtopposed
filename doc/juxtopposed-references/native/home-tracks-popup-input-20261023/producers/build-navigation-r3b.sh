#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-tracks-subtitles-20261023"
out="$base/owned-tracks-navigation-r3b"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-navigation-r3b.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-navigation-r3b-exit.txt"; exit "$code"' EXIT
test ! -e "$out"
make -C "$base/win64/modules/gui/qt" -f Makefile -f "$repo/modules/gui/qt/tests/native_library/tracks-navigation.mk" -j2 TRACKS_NAV_OUT="$out" native-tracks-navigation
mkdir "$work/navigation-r3b-bin"
cp "$out/.libs/test_tracks_cancel_navigation.exe" "$work/navigation-r3b-bin/"
sha256sum "$work/navigation-r3b-bin/test_tracks_cancel_navigation.exe"
