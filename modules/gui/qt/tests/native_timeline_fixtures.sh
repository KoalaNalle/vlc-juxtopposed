#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Generate small silent fixtures. Never write into an existing output directory.
set -euo pipefail
output=${1:?Usage: native_timeline_fixtures.sh NEW_OUTPUT_DIRECTORY}
mkdir "$output"
cp "$(dirname "$0")/native_timeline_server.py" "$output/"
ffmpeg -hide_banner -loglevel error -nostdin -threads 1 \
    -f lavfi -i anullsrc=r=8000:cl=mono -t 24 -c:a pcm_s16le \
    "$output/finite.wav"
ffmpeg -hide_banner -loglevel error -nostdin -threads 1 \
    -f lavfi -i anullsrc=r=8000:cl=mono -t 28 -c:a aac -b:a 16k -threads 1 \
    -hls_time 1 -hls_list_size 0 -hls_segment_filename "$output/segment-%03d.ts" \
    -f hls "$output/generated.m3u8"
