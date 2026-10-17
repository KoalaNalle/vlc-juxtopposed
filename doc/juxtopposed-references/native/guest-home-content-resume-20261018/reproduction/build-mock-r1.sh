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
exec > >(tee "$work/build-mock-r1.log") 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-mock-r1-exit.txt"; exit "$code"' EXIT
make -C "$base/win64/modules" -j1 libdemux_mock_plugin.la
mkdir "$work/player-mock-r1"
cp "$base/win64/modules/.libs/libdemux_mock_plugin.dll" "$work/player-mock-r1/"
