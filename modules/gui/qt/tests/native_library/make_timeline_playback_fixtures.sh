#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Synthetic sustained playback and thumbnail startup-policy regression media.
set -euo pipefail
fixture_out=${1:?Provide a NEW absolute output directory}
[[ "$fixture_out" = /* ]] || { printf '%s\n' 'Use an absolute output directory'; exit 2; }
test ! -e "$fixture_out"
fixture_ffmpeg=${FFMPEG:-ffmpeg}
fixture_ffprobe=${FFPROBE:-ffprobe}
mkdir -p "$fixture_out"
"$fixture_ffmpeg" -nostdin -hide_banner -loglevel error \
 -f lavfi -i testsrc2=size=1920x1080:rate=50:duration=120 \
 -f lavfi -i anullsrc=r=48000:cl=stereo -t 120 \
 -c:v libx264 -threads 4 -preset veryfast -crf 28 -g 100 -keyint_min 100 \
 -sc_threshold 0 -bf 3 -pix_fmt yuv420p -c:a aac -b:a 128k -movflags +faststart \
 "$fixture_out/motion-1080p50-silent.mp4"
"$fixture_ffprobe" -v error -show_streams -show_format -of json \
 "$fixture_out/motion-1080p50-silent.mp4" > "$fixture_out/media.json"
"$fixture_ffmpeg" -nostdin -hide_banner -loglevel error -ss 40 \
 -i "$fixture_out/motion-1080p50-silent.mp4" -an \
 -vf "scale=iw*sar:ih,setsar=1,crop='min(iw,ih*252/142)':'min(ih,iw*142/252)',scale=252:142:flags=bicubic" \
 -frames:v 1 -update 1 "$fixture_out/reference-40.png"
