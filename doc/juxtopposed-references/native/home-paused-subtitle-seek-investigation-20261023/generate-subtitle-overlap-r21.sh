#!/usr/bin/env bash
set -euo pipefail
work=/mnt/c/Modding/vlc-juxtopposed/build-windows/beta-tracks-subtitles-20261023
out="$work/media-r21"
test ! -e "$out"
mkdir "$out"
cat > "$out/overlap-en.srt" <<'EOF'
1
00:00:00,000 --> 00:00:59,900
LONG OVERLAPPING CUE

2
00:00:10,000 --> 00:00:12,000
SHORT OVERLAY CUE

3
00:00:30,000 --> 00:00:32,000
SECOND SHORT OVERLAY
EOF
ffmpeg -hide_banner -loglevel error -nostdin -copyts -i "$work/media-r20/colored-long.mkv" -i "$out/overlap-en.srt" \
    -map 0:v -map 0:a -map 1:s -c copy -cluster_time_limit 1000 -avoid_negative_ts disabled \
    -metadata:s:s:0 language=eng -disposition:s:0 default "$out/colored-overlap.mkv"
ffprobe -v error -show_streams -show_format -of json "$out/colored-overlap.mkv" > "$out/probe.json"
ffprobe -v error -select_streams s -show_packets -show_entries packet=pts_time,duration_time,pos -of json "$out/colored-overlap.mkv" > "$out/subtitle-packets.json"
sha256sum "$out/colored-overlap.mkv"
