#!/usr/bin/env bash
set -euo pipefail
work=/mnt/c/Modding/vlc-juxtopposed/build-windows/beta-tracks-subtitles-20261023
source="$work/media-r1/multi-language.mkv"
out="$work/media-r7"
test ! -e "$out"
mkdir "$out"
ffmpeg -hide_banner -loglevel error -nostdin -i "$source" -map 0 -c copy "$out/remux-all.mkv"
ffmpeg -hide_banner -loglevel error -nostdin -i "$source" -map 0:v:0 -map 0:a -c copy "$out/two-audio-no-subs.mkv"
ffmpeg -hide_banner -loglevel error -nostdin -i "$source" -map 0:v:0 -map 0:a:0 -map 0:s -c copy "$out/one-audio-two-subs.mkv"
cp "$work/media-r1/manual-external.srt" "$out/"
for name in remux-all two-audio-no-subs one-audio-two-subs; do
    ffprobe -v error -show_streams -show_format -of json "$out/$name.mkv" > "$out/$name-probe.json"
done
sha256sum "$out/"*.mkv
