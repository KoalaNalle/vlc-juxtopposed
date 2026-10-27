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
exec > >(tee "$work/build-player-modules-r2.log") 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-player-modules-r2-exit.txt"; exit "$code"' EXIT
mkdir "$work/player-modules-r2"
make -C "$base/win64/test" -f Makefile -f "$work/player-modules-dynamic.mk" -j1 owned-player-modules QA_SOURCE="$work/player_modules_dynamic_r2.c" QA_OUT="$work/player-modules-r2"
