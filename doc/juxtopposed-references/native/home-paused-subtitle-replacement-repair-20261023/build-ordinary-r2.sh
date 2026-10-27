#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-subtitle-replacement-repair-20261023"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-ordinary-r2.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-ordinary-r2-exit.txt"; exit "$code"' EXIT
test ! -e "$work/ordinary-core-r2"
make -C "$base/win64/src" -j2 libvlccore.la
mkdir "$work/ordinary-core-r2"
cp "$base/win64/src/.libs/libvlccore.dll" "$base/win64/src/player/.deps/libvlccore_la-input.Plo" "$work/ordinary-core-r2/"
sha256sum "$work/ordinary-core-r2/libvlccore.dll" "$base/win64/src/player/.libs/libvlccore_la-input.o"
