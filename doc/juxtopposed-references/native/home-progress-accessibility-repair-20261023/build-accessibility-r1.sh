#!/usr/bin/env bash
set -euo pipefail
test "$#" = 1
phase="$1"
case "$phase" in before|after) ;; *) exit 2;; esac
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-caption-accessibility-20261023"
out="$base/owned-accessible-seek-$phase-r1"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-$phase-r1.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-$phase-r1-exit.txt"; exit "$code"' EXIT
test ! -e "$out"
make -C "$base/win64/modules/gui/qt" -f Makefile -f "$work/accessibility-journey-r1.mk" -j2 QA_OUT="$out" QA_SOURCE="$work/seek_accessibility_journey.cpp" owned-native-ui
mkdir "$work/qa-$phase-r1-bin"
cp "$out/.libs/libqt_plugin.dll" "$work/qa-$phase-r1-bin/"
sha256sum "$work/qa-$phase-r1-bin/libqt_plugin.dll"
