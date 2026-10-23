#!/usr/bin/env bash
set -euo pipefail
work=/mnt/c/Modding/vlc-juxtopposed/build-windows/beta-tracks-subtitles-20261023
source="$work/media-r1/multi-language.mkv"
out="$work/media-r8"
test ! -e "$out"
mkdir "$out"
for spec in preserve:0 plus1ms:0.001 plus21ms:0.021 minus1ms:-0.001; do
    name=${spec%%:*}
    offset=${spec#*:}
    ffmpeg -hide_banner -loglevel error -nostdin -copyts -itsoffset "$offset" -i "$source" -map 0 -c copy -avoid_negative_ts disabled "$out/$name.mkv"
    ffprobe -v error -show_streams -show_format -of json "$out/$name.mkv" > "$out/$name-probe.json"
    ffprobe -v error -select_streams v:0 -show_packets -show_data_hash sha256 -show_entries packet=pts_time,dts_time,duration_time,data_hash -of json "$out/$name.mkv" > "$out/$name-video.json"
    ffprobe -v error -select_streams a:0 -show_packets -show_data_hash sha256 -show_entries packet=pts_time,dts_time,duration_time,data_hash -of json "$out/$name.mkv" > "$out/$name-audio.json"
done
ffprobe -v error -select_streams a:0 -show_packets -show_data_hash sha256 -show_entries packet=pts_time,dts_time,duration_time,data_hash -of json "$source" > "$out/original-audio.json"
cp "$work/media-r1/manual-external.srt" "$out/"
sha256sum "$out/"*.mkv
