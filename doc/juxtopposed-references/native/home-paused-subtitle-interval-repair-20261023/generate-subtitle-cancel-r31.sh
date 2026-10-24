#!/usr/bin/env bash
set -euo pipefail
work=/mnt/c/Modding/vlc-juxtopposed/build-windows/beta-tracks-subtitles-20261023
out="$work/media-r31"
test ! -e "$out"
mkdir "$out"
ffmpeg -hide_banner -loglevel error -nostdin -copyts -i "$work/media-r21/colored-overlap.mkv" \
    -map 0 -c:v libx264 -threads 2 -preset veryfast -g 25 -pix_fmt yuv420p \
    -b:v 60M -minrate 60M -maxrate 60M -bufsize 120M -x264-params 'nal-hrd=cbr:force-cfr=1' \
    -c:a copy -c:s copy -cluster_time_limit 1000 -avoid_negative_ts disabled "$out/cbr-overlap.mkv"
ffprobe -v error -show_streams -show_format -of json "$out/cbr-overlap.mkv" > "$out/cbr-overlap-probe.json"
ffprobe -v error -select_streams s -show_packets -show_entries packet=pts_time,duration_time,pos -of json \
    "$out/cbr-overlap.mkv" > "$out/cbr-overlap-subtitle-packets.json"
sha256sum "$out/cbr-overlap.mkv" > "$out/sha256.txt"
