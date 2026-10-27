#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-subtitle-sync-20261025"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-wasapi-r15.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-wasapi-r15-exit.txt"; exit "$code"' EXIT
test "$(sha256sum "$base/win64/modules/.libs/libwasapi_plugin.dll" | cut -d' ' -f1)" = 4e4e33c04003ab5597b64f3657eff68ed2c583f1c7ba192f660482d16d792408
test ! -e "$work/wasapi-r15-bin"
make -C "$base/win64/modules" -j2 libwasapi_plugin.la
mkdir "$work/wasapi-r15-bin"
cp "$base/win64/modules/.libs/libwasapi_plugin.dll" "$work/wasapi-r15-bin/"
cp "$base/win64/modules/audio_output/.deps/wasapi.Plo" "$work/wasapi-r15-bin/"
sha256sum "$work/wasapi-r15-bin/libwasapi_plugin.dll"
