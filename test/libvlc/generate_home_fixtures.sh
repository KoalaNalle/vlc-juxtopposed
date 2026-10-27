#!/usr/bin/env bash
# Isolated tagged media for the real Home activity diagnostic. GPLv2 or later.
set -euo pipefail
output=${1:?Usage: generate_home_fixtures.sh FRESH_OUTPUT_DIRECTORY}
mkdir -p "$output"
for artist in alpha beta; do
    if [[ "$artist" == alpha ]]; then display='Home Artist Alpha'; else display='Home Artist Beta'; fi
    for track in one two; do
        ffmpeg -hide_banner -loglevel error -n -f lavfi -i 'sine=frequency=440:sample_rate=44100:duration=2' \
            -c:a flac -metadata artist="$display" -metadata album_artist="$display" \
            -metadata album='Home Native Fixture' -metadata title="$display $track" "$output/$artist-$track.flac"
    done
done
ffmpeg -hide_banner -loglevel error -n -f lavfi -i 'color=c=blue:s=320x180:r=25:d=2' \
    -c:v mpeg4 -g 1 -q:v 2 "$output/home-video.mp4"
