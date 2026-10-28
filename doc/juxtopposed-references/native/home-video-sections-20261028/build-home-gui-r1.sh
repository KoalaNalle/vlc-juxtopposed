#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-home-sections-20261028"
out="$work/gui-r1"
test ! -e "$out"
mkdir "$out"
exec > "$out/build.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$out/exit.txt"; exit "$code"' EXIT
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
cd "$repo"
sha256sum "$work/build-home-gui-r1.sh" > "$out/producer.sha256"
git rev-parse HEAD > "$out/commit.txt"
git diff --binary -- src modules/gui/qt modules/demux modules/codec > "$out/source.patch"
git ls-files -z -- modules/gui/qt src/player src/input | xargs -0 sha256sum > "$out/inputs.sha256"
sha256sum modules/gui/qt/medialibrary/mlvideohistory.hpp >> "$out/inputs.sha256"
make -C "$base/win64/modules/gui/qt" -j2 V=0 libqt_plugin.la
sha256sum -c "$out/inputs.sha256"
sha256sum -c "$out/producer.sha256"
cp "$base/win64/modules/gui/qt/.libs/libqt_plugin.dll" "$out/"
sha256sum "$out/libqt_plugin.dll" > "$out/binary.sha256"
