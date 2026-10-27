#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-timeline-thumbnails-20261021"
qa="$base/owned-ordinary-r94"
test ! -e "$qa"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-ordinary-r94.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-ordinary-r94-exit.txt"; exit "$code"' EXIT
make -C "$base/win64/modules/gui/qt" -f Makefile -f "$work/ordinary-r94.mk" -j2 QA_OUT="$qa" owned-ordinary-r94
mkdir "$work/ordinary-r94"
cp "$qa/.libs/libqt_plugin.dll" "$work/ordinary-r94/"
sha256sum "$work/ordinary-r94/libqt_plugin.dll"
