#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-timeline-thumbnails-20261021"
test ! -e "$work/ordinary-r67"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-ordinary-r67.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-ordinary-r67-exit.txt"; exit "$code"' EXIT
printf '%s\n' 'libqt_plugin_la_LDFLAGS += -Wl,--strip-debug' > "$work/ordinary-r67.mk"
make -C "$base/win64/modules/gui/qt" -f Makefile -f "$work/ordinary-r67.mk" -j2 libqt_plugin.la
mkdir "$work/ordinary-r67"
cp "$base/win64/modules/gui/qt/.libs/libqt_plugin.dll" "$work/ordinary-r67/"
sha256sum "$work/ordinary-r67/libqt_plugin.dll"
