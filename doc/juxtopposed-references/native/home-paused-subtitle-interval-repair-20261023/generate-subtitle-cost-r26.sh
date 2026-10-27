#!/usr/bin/env bash
set -euo pipefail
work=/mnt/c/Modding/vlc-juxtopposed/build-windows/beta-tracks-subtitles-20261023
out="$work/media-r26"
test ! -e "$out"
mkdir "$out"
ffmpeg -hide_banner -loglevel error -nostdin -copyts -i "$work/media-r21/colored-overlap.mkv" \
    -map 0 -c:v libx264 -threads 2 -preset veryfast -g 25 -pix_fmt yuv420p \
    -b:v 4M -minrate 4M -maxrate 4M -bufsize 8M -x264-params 'nal-hrd=cbr:force-cfr=1' \
    -c:a copy -c:s copy -cluster_time_limit 1000 -avoid_negative_ts disabled "$out/cbr-overlap.mkv"
ffmpeg -hide_banner -loglevel error -nostdin -copyts -i "$out/cbr-overlap.mkv" -i "$work/media-r20/gaps-en.srt" \
    -map 0:v -map 0:a -map 1:s -c copy -metadata:s:s:0 language=eng -disposition:s:0 default \
    -cluster_time_limit 1000 -avoid_negative_ts disabled "$out/cbr-gaps.mkv"
ffmpeg -hide_banner -loglevel error -nostdin -copyts -i "$work/media-r21/colored-overlap.mkv" \
    -map 0 -c:v copy -c:a copy -c:s ass -metadata:s:s:0 language=eng \
    -cluster_time_limit 1000 -avoid_negative_ts disabled "$out/ass-overlap.mkv"
for name in cbr-overlap cbr-gaps ass-overlap; do
    ffprobe -v error -show_streams -show_format -of json "$out/$name.mkv" > "$out/$name-probe.json"
    ffprobe -v error -select_streams s -show_packets -show_entries packet=pts_time,duration_time,pos -of json "$out/$name.mkv" > "$out/$name-subtitle-packets.json"
    sha256sum "$out/$name.mkv"
done
