#!/usr/bin/env bash
set -euo pipefail
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-external-subtitle-acceptance-20261023"
exec > "$work/generate-fixtures-r1.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/generate-fixtures-r1-exit.txt"; exit "$code"' EXIT
bash "$repo/test/libvlc/generate_paused_subtitle_tracks.sh" "$repo/build-windows/beta-tracks-subtitles-20261023/media-generator-smoke-r35/colored-long.mkv" "$work/media-r1"
