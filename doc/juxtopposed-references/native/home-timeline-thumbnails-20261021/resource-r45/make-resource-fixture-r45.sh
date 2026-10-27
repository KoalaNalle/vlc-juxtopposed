#!/usr/bin/env bash
set -euo pipefail
work=/mnt/c/Modding/vlc-juxtopposed/build-windows/beta-timeline-thumbnails-20261021
out="$work/resource-fixtures-r45"
test ! -e "$out"
mkdir "$out"
exec > "$work/resource-fixture-r45.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/resource-fixture-r45-exit.txt"; exit "$code"' EXIT
ffmpeg -nostdin -hide_banner -loglevel error -f lavfi -i testsrc2=size=1920x1080:rate=50:duration=120 -f lavfi -i anullsrc=r=48000:cl=stereo -t 120 -c:v libx264 -threads 4 -preset veryfast -crf 28 -g 100 -keyint_min 100 -sc_threshold 0 -bf 3 -pix_fmt yuv420p -c:a aac -b:a 128k -movflags +faststart "$out/motion-1080p50-silent.mp4"
ffprobe -v error -show_streams -show_format -of json "$out/motion-1080p50-silent.mp4" > "$out/media.json"
sha256sum "$out/motion-1080p50-silent.mp4"
