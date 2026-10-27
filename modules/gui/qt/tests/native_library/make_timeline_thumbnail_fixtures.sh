#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Owned synthetic media only. Requires existing ffmpeg/ffprobe and Python3.
# Run in the native/WSL build environment with a NEW absolute output directory:
#   bash make_timeline_thumbnail_fixtures.sh /absolute/path/to/new-fixtures
# Then export VLC_THUMBNAIL_FIXTURE=<output>/preview-colors.mp4 before the native
# Qt test. Missing fixtures skip ordinary make check; qualification rejects skips.
set -euo pipefail
fixture_out=${1:?Provide a new absolute output directory}
[[ "$fixture_out" = /* ]] || { printf '%s\n' 'Use an absolute output directory' >&2; exit 2; }
test ! -e "$fixture_out"
fixture_ffmpeg=${FFMPEG:-ffmpeg}
fixture_ffprobe=${FFPROBE:-ffprobe}
fixture_python=${PYTHON:-python3}
command -v "$fixture_ffmpeg" >/dev/null
command -v "$fixture_ffprobe" >/dev/null
command -v "$fixture_python" >/dev/null
mkdir -p "$fixture_out"
"$fixture_ffmpeg" -nostdin -hide_banner -loglevel error \
 -f lavfi -i color=c=red:s=320x180:r=25:d=6 \
 -f lavfi -i color=c=lime:s=320x180:r=25:d=6 \
 -f lavfi -i color=c=blue:s=320x180:r=25:d=6 \
 -filter_complex '[0:v][1:v][2:v]concat=n=3:v=1:a=0[v]' -map '[v]' -an \
 -c:v libx264 -preset ultrafast -crf 18 -g 25 -keyint_min 25 -sc_threshold 0 -bf 0 \
 -pix_fmt yuv420p -movflags +faststart "$fixture_out/preview-colors.mp4"
"$fixture_ffmpeg" -nostdin -hide_banner -loglevel error \
 -f lavfi -i testsrc2=size=1920x1080:rate=30:duration=18 \
 -vf "drawbox=x=iw/3:y=ih/3:w=iw/3:h=ih/3:c=red:t=fill:enable='lt(t,6)',drawbox=x=iw/3:y=ih/3:w=iw/3:h=ih/3:c=lime:t=fill:enable='between(t,6,12)',drawbox=x=iw/3:y=ih/3:w=iw/3:h=ih/3:c=blue:t=fill:enable='gte(t,12)'" \
 -an -c:v libx264 -threads 4 -preset veryfast -crf 28 -g 180 -keyint_min 180 -sc_threshold 0 -bf 3 -pix_fmt yuv420p -movflags +faststart "$fixture_out/preview-motion-1080p.mp4"
"$fixture_python" - "$fixture_out/preview-silent.wav" <<'PY'
import sys, wave
with wave.open(sys.argv[1], 'wb') as audio:
    audio.setnchannels(2)
    audio.setsampwidth(2)
    audio.setframerate(44100)
    audio.writeframes(bytes(18*44100*4))
PY
for fixture_name in preview-colors preview-motion-1080p; do
 "$fixture_ffprobe" -v error -show_streams -show_format -of json "$fixture_out/$fixture_name.mp4" > "$fixture_out/$fixture_name-ffprobe.json"
done
