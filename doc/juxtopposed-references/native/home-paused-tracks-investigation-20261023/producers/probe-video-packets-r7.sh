#!/usr/bin/env bash
set -euo pipefail
work=/mnt/c/Modding/vlc-juxtopposed/build-windows/beta-tracks-subtitles-20261023
out="$work/video-packet-probes-r7"
test ! -e "$out"
mkdir "$out"
ffprobe -v error -select_streams v:0 -show_packets -show_data_hash sha256 -show_entries packet=pts_time,dts_time,duration_time,data_hash -of json "$work/media-r1/multi-language.mkv" > "$out/original.json"
for name in remux-all two-audio-no-subs one-audio-two-subs; do
    ffprobe -v error -select_streams v:0 -show_packets -show_data_hash sha256 -show_entries packet=pts_time,dts_time,duration_time,data_hash -of json "$work/media-r7/$name.mkv" > "$out/$name.json"
done
