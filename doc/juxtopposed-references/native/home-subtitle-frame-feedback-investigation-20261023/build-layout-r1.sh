#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-subtitle-frame-feedback-20261023"
exec > "$work/build-layout-r1.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-layout-r1-exit.txt"; exit "$code"' EXIT
test ! -e "$work/layout-r1.exe"
x86_64-w64-mingw32-gcc --version
x86_64-w64-mingw32-gcc -std=c11 -Wall -Wextra -I"$base/win64/include" -I"$repo/include" \
    "$repo/test/libvlc/native_player_callback_layout.c" -o "$work/layout-r1.exe"
sha256sum "$repo/test/libvlc/native_player_callback_layout.c" "$repo/include/vlc/libvlc_media_player.h" "$work/layout-r1.exe"
