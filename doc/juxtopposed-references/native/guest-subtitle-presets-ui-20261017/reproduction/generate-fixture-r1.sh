#!/usr/bin/env bash
set -euo pipefail
out=/mnt/e/Modding/vlc-juxtopposed/build-windows/beta-presets-20261017/fixtures-native-r1
test ! -e "$out/subtitle-style.mp4"
/usr/bin/ffmpeg -nostdin -hide_banner -loglevel error -f lavfi -i 'color=c=black:s=320x180:r=10:d=60' -an -c:v libx264 -threads 1 -filter_threads 1 -preset ultrafast -pix_fmt yuv420p -movflags +faststart "$out/subtitle-style.mp4"
/usr/bin/ffmpeg -version > "$out/generator-version.txt"
/usr/bin/ffprobe -v error -show_entries stream=codec_name,width,height,avg_frame_rate:format=duration -of json "$out/subtitle-style.mp4" > "$out/video.json"
