#!/usr/bin/env bash
set -euo pipefail
work=/mnt/c/Modding/vlc-juxtopposed/build-windows/beta-tracks-subtitles-20261023
out="$work/media-r12"
test ! -e "$out"
mkdir "$out"
ffmpeg -hide_banner -loglevel error -nostdin -i "$work/media-r1/multi-language.mkv" -map 0:a:0 -c copy "$out/audio-only.mka"
ffmpeg -hide_banner -loglevel error -nostdin -f lavfi -i 'color=c=red:s=960x540:r=25:d=0.04' -frames:v 1 -c:v libx264 -threads 2 -pix_fmt yuv420p "$out/single-picture.mkv"
ffmpeg -hide_banner -loglevel error -nostdin -copyts -i "$work/media-r1/multi-language.mkv" -itsoffset 3 -i "$work/media-r1/multi-language.mkv" -map 1:v:0 -map 0:a:0 -c copy -t 8 -avoid_negative_ts disabled "$out/delayed-video.mkv"
for name in audio-only.mka single-picture.mkv delayed-video.mkv; do
    ffprobe -v error -show_streams -show_format -of json "$out/$name" > "$out/$name.probe.json"
    sha256sum "$out/$name"
done
