#!/usr/bin/env bash
set -euo pipefail
work=/mnt/c/Modding/vlc-juxtopposed/build-windows/beta-tracks-subtitles-20261023
out="$work/subtitle-packet-probes-r17"
test ! -e "$out"
mkdir "$out"
for variant in original preserve; do
    if test "$variant" = original; then media="$work/media-r1/multi-language.mkv"; else media="$work/media-r8/preserve.mkv"; fi
    for track in 0 1; do
        ffprobe -v error -select_streams s:$track -show_packets -show_entries packet=pts_time,dts_time,duration_time,pos,size -of json "$media" > "$out/$variant-subtitle-$track.json"
    done
    ffprobe -v error -select_streams v:0 -show_packets -show_entries packet=pts_time,dts_time,pos,flags -of json "$media" > "$out/$variant-video.json"
done
