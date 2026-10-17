#!/usr/bin/env bash
set -euo pipefail
work=/mnt/e/Modding/vlc-juxtopposed/build-windows/beta-home-content-20261017
ffmpeg -nostdin -hide_banner -loglevel error -f lavfi -i anullsrc=r=8000:cl=mono -t 60 -c:a flac -threads 1 -metadata title="First owned track" -metadata artist="Travel Artist Ää" -metadata album_artist="Travel Artist Ää" -metadata album="Owned Album" -metadata track=1 "$work/fixtures/first.flac"
ffmpeg -nostdin -hide_banner -loglevel error -f lavfi -i anullsrc=r=8000:cl=mono -t 60 -c:a flac -threads 1 -metadata title="Second owned track" -metadata artist="Travel Artist Ää" -metadata album_artist="Travel Artist Ää" -metadata album="Owned Album" -metadata track=2 "$work/fixtures/second.flac"
ffprobe -v error -show_entries format=duration:format_tags -of json "$work/fixtures/first.flac" > "$work/fixtures/tagged-audio.json"
