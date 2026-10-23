#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-tracks-subtitles-20261023"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-core-r14.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-core-r14-exit.txt"; exit "$code"' EXIT
test ! -e "$work/core-r14"
make -C "$base/win64/src" -j2 libvlccore.la
mkdir "$work/core-r14"
cp "$base/win64/src/.libs/libvlccore.dll" "$base/win64/src/input/.deps/libvlccore_la-decoder.Plo" "$work/core-r14/"
sha256sum "$work/core-r14/libvlccore.dll"
