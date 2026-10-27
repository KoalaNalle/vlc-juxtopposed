#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-external-subtitle-acceptance-20261023"
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
exec > "$work/build-ordinary-r2.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/build-ordinary-r2-exit.txt"; exit "$code"' EXIT
test ! -e "$work/ordinary-core-r2"
make -C "$base/win64/src" -j2 libvlccore.la
mkdir "$work/ordinary-core-r2"
cp "$base/win64/src/.libs/libvlccore.dll" "$work/ordinary-core-r2/"
cp "$base/win64/src/input/.deps/libvlccore_la-input.Plo" "$work/ordinary-core-r2/input.Plo"
cp "$base/win64/src/input/.deps/libvlccore_la-es_out.Plo" "$work/ordinary-core-r2/es_out.Plo"
cp "$base/win64/src/input/.deps/libvlccore_la-es_out_source.Plo" "$work/ordinary-core-r2/es_out_source.Plo"
sha256sum "$base/win64/src/.libs/libvlccore.dll" "$base/win64/src/input/.libs/libvlccore_la-input.o" "$base/win64/src/input/.libs/libvlccore_la-es_out.o" "$base/win64/src/input/.libs/libvlccore_la-es_out_source.o"
