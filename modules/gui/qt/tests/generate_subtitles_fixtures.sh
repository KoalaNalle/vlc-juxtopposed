#!/bin/sh
# Author does not execute. Root supplies the existing GENERATED slice fixture.
set -eu
test "$#" = 2
source_video=$1
out=$2
test -f "$source_video"
test ! -e "$out"
mkdir -p "$out"
cp "$source_video" "$out/plain.mp4"
printf '1\n00:00:00,000 --> 00:00:59,900\niiiiiiii\n' > "$out/plain.srt"
printf '1\n00:00:00,000 --> 00:00:59,900\nEnglish owned fixture\n' > "$out/english.srt"
printf '1\n00:00:00,000 --> 00:00:59,900\nSuomi oma koe\n' > "$out/finnish.srt"
ffmpeg -hide_banner -loglevel error -y -i "$source_video" -i "$out/english.srt" -i "$out/finnish.srt" \
    -map 0:v:0 -map 1:s:0 -map 2:s:0 -c:v copy -c:s srt \
    -metadata:s:s:0 language=eng -metadata:s:s:1 language=fin \
    -disposition:s:0 default -disposition:s:1 0 "$out/languages.mkv"
