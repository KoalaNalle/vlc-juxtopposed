#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-subtitle-delay-repair-20261023"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-ordinary-r1.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-ordinary-r1-exit.txt"; exit "$code"' EXIT
test "$(sha256sum "$base/win64/src/.libs/libvlccore.dll" | cut -d' ' -f1)" = 0ef9dde2f0a221135cab0a548ac292dd89d7cc9b7b79427eaaba121995b7773f
test ! -e "$work/ordinary-core-r1"
make -C "$base/win64/src" -j2 libvlccore.la
mkdir "$work/ordinary-core-r1"
cp "$base/win64/src/.libs/libvlccore.dll" "$work/ordinary-core-r1/"
for item in input/decoder input/es_out input/input player/input player/player; do
    dir="${item%/*}"
    file="${item##*/}"
    cp "$base/win64/src/$dir/.deps/libvlccore_la-$file.Plo" "$work/ordinary-core-r1/$dir-$file.Plo"
done
sha256sum "$base/win64/src/.libs/libvlccore.dll"
