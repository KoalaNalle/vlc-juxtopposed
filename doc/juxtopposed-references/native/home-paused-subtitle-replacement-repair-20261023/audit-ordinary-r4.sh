#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build/win64/src
work=/mnt/c/Modding/vlc-juxtopposed/build-windows/beta-subtitle-replacement-repair-20261023
test ! -e "$work/ordinary-object-audit-r4"
mkdir "$work/ordinary-object-audit-r4"
cp "$base/input/.deps/libvlccore_la-decoder.Plo" "$work/ordinary-object-audit-r4/decoder.Plo"
cp "$base/input/.deps/libvlccore_la-es_out.Plo" "$work/ordinary-object-audit-r4/es_out.Plo"
cp "$base/player/.deps/libvlccore_la-input.Plo" "$work/ordinary-object-audit-r4/player-input.Plo"
sha256sum "$base/input/.libs/libvlccore_la-decoder.o" "$base/input/.libs/libvlccore_la-es_out.o" "$base/player/.libs/libvlccore_la-input.o" "$base/.libs/libvlccore.dll" > "$work/ordinary-object-audit-r4/sha256.txt"
