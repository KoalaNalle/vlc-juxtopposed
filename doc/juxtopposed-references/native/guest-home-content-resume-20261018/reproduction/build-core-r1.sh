#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/e/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-home-content-20261017"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
export JOBS=1 MESON_BUILD='-j 1'
unset CC CXX
exec > >(tee "$work/build-core-r1.log") 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-core-r1-exit.txt"; exit "$code"' EXIT
make -C "$base/win64/src" -j1 libvlccore.la
mkdir "$work/core-native-r1"
cp "$base/win64/src/.libs/libvlccore.dll" "$work/core-native-r1/"
