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
exec > >(tee "$work/build-player-relocated-r1.log") 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-player-relocated-r1-exit.txt"; exit "$code"' EXIT
make -C "$base/win64/test" -j1 test_src_player_lifecycle.exe test_src_player_pause.exe test_src_player_seeks.exe test_src_player_media.exe test_src_player_timers.exe
mkdir "$work/player-native-relocated-r1"
for name in lifecycle pause seeks media timers; do
 cp "$base/win64/test/.libs/test_src_player_$name.exe" "$work/player-native-relocated-r1/"
done
