#!/usr/bin/env bash
set -euo pipefail
base=/root/vlc-juxtopposed-build
work=/mnt/c/Modding/vlc-juxtopposed/build-windows/beta-tracks-subtitles-20261023
exec 9> "$base/.vlc-build.lock"
flock -n 9
exec > "$work/finalize-ordinary-build-r35c.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/finalize-ordinary-build-r35c-exit.txt"; exit "$code"' EXIT
# R35b compiled and linked all sixteen units. Its final copy requested the
# optional dependency name, but the configured rules wrote real standard .Plo
# files via their later -MF option. Preserve the packaging failure unchanged.
test "$(cat "$work/build-ordinary-mkv-r35b-exit.txt")" = 1
make -q -C "$base/win64/modules" libmkv_plugin.la
mkdir "$work/ordinary-mkv-r35c-bin"
cp "$base/win64/modules/.libs/libmkv_plugin.dll" "$work/ordinary-mkv-r35c-bin/"
for name in util virtual_segment matroska_segment matroska_segment_parse matroska_segment_seeker demux events Ebml_parser chapters chapter_command chapter_command_dvd chapter_command_script chapter_command_script_common stream_io_callback lzokay mkv; do
    dep="$base/win64/modules/demux/mkv/.deps/libmkv_plugin_la-$name.Plo"
    test "$(wc -c < "$dep")" -gt 20
    cp "$dep" "$work/ordinary-mkv-r35c-bin/"
done
sha256sum "$base/win64/modules/demux/mkv/.libs/"libmkv_plugin_la-*.o "$base/win64/modules/packetizer/.libs/libmkv_plugin_la-dts_header.o" > "$work/ordinary-mkv-r35c-bin/object-hashes.txt"
sha256sum "$work/ordinary-mkv-r35c-bin/libmkv_plugin.dll"
