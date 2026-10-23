#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-tracks-subtitles-20261023"
qa="$base/owned-ordinary-tracks-r4"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-r4.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-r4-exit.txt"; exit "$code"' EXIT
test ! -e "$qa"
make -C "$base/win64/modules/gui/qt" -f Makefile -f "$work/ordinary-r4.mk" -j2 QA_OUT="$qa" owned-ordinary-tracks-r4
mkdir "$work/ordinary-r4"
cp "$qa/.libs/libqt_plugin.dll" "$work/ordinary-r4/"
sha256sum "$work/ordinary-r4/libqt_plugin.dll"
