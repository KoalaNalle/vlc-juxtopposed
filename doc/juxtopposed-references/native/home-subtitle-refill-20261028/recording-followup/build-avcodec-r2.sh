#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
repo=/mnt/c/Modding/vlc-juxtopposed
out="$repo/build-windows/beta-fps-20261028/avcodec-plugin-r2"
test ! -e "$out"
mkdir "$out"
exec > "$out/build.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$out/exit.txt"; exit "$code"' EXIT
exec 9> "$base/.vlc-build.lock"
flock -n 9
export PATH="$base/extras/tools/build/bin:$base/contrib/bin:$PATH"
unset CC CXX
cd "$repo"
git diff --binary -- modules/codec/avcodec modules/demux/avformat modules/access/avio.c modules/packetizer/avparser.c > "$out/source.patch"
git ls-files -z src include modules/codec/avcodec modules/demux/avformat modules/access/avio.c modules/packetizer/avparser.c | xargs -0 sha256sum > "$out/inputs.sha256"
sha256sum -c "$out/inputs.sha256" > "$out/inputs-before.log"
test "$(stat -c %h "$base/win64/modules/.libs/libavcodec_plugin.dll")" = 1
sha256sum "$repo/build-windows/beta-follow-20261027/runtime-r7/plugins/codec/libavcodec_plugin.dll" > "$out/baseline.sha256"
make -C "$base/win64/modules" -j2 V=0 libavcodec_plugin.la
sha256sum -c "$out/inputs.sha256" > "$out/inputs-after.log"
cp "$base/win64/modules/.libs/libavcodec_plugin.dll" "$out/"
sha256sum "$out/libavcodec_plugin.dll" > "$out/binary.sha256"
sha256sum "$repo/build-windows/beta-fps-20261028/build-avcodec-r2.sh" > "$out/producer.sha256"
sha256sum -c "$out/baseline.sha256" > "$out/baseline-after.log"
