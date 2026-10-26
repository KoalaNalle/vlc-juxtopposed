#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-subtitle-sync-20261025"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-core-r12.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-core-r12-exit.txt"; exit "$code"' EXIT
test "$(sha256sum "$base/win64/src/.libs/libvlccore.dll" | cut -d' ' -f1)" = bebbfed6e38b88d2ef54b85e0f83dd1efc112ac63f9a27c7a3d5d6af31fa3991
test ! -e "$work/core-r12-bin"
make -C "$base/win64/src" -j2 libvlccore.la
mkdir "$work/core-r12-bin"
cp "$base/win64/src/.libs/libvlccore.dll" "$work/core-r12-bin/"
cp "$base/win64/src/input/.deps/libvlccore_la-input.Plo" "$work/core-r12-bin/input.Plo"
cp "$base/win64/src/input/.deps/libvlccore_la-es_out.Plo" "$work/core-r12-bin/es_out.Plo"
cp "$base/win64/src/.deps/libvlccore_la-libvlc.Plo" "$work/core-r12-bin/libvlc.Plo"
sha256sum "$work/core-r12-bin/libvlccore.dll"
