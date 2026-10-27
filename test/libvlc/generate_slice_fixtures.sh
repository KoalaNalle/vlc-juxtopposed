#!/usr/bin/env bash
# Generated local media for native vertical-slice checks. GPLv2 or later.
set -euo pipefail
output=${1:?Usage: generate_slice_fixtures.sh OUTPUT_DIRECTORY}
mkdir -p "$output"
for name in paused-seek-colors.mp4 chapters.ffmetadata chaptered-colors.srt chaptered-colors.mkv waveform-audio.wav quadrants.mp4 subtitle-style.mp4 subtitle-style.srt duration-cues.srt; do
    if [[ -e "$output/$name" ]]; then
        printf 'Fixture already exists: %s\nUse a fresh output directory.\n' "$output/$name" >&2
        exit 1
    fi
done

ffmpeg -hide_banner -loglevel error -n \
    -f lavfi -i color=c=red:s=320x180:r=25:d=6 \
    -f lavfi -i color=c=blue:s=320x180:r=25:d=6 \
    -f lavfi -i color=c=lime:s=320x180:r=25:d=6 \
    -filter_complex '[0:v][1:v][2:v]concat=n=3:v=1:a=0[v]' -map '[v]' \
    -c:v mpeg4 -g 1 -q:v 2 "$output/paused-seek-colors.mp4"

# Asymmetric spatial fixture for flip/rotation checks on rendered pixels.
ffmpeg -hide_banner -loglevel error -n -f lavfi \
    -i 'color=c=red:s=128x128:r=25:d=60,drawbox=x=64:y=0:w=64:h=64:color=lime:t=fill,drawbox=x=0:y=64:w=64:h=64:color=blue:t=fill,drawbox=x=64:y=64:w=64:h=64:color=yellow:t=fill' \
    -c:v mpeg4 -g 1 -q:v 2 "$output/quadrants.mp4"

ffmpeg -hide_banner -loglevel error -n -f lavfi -i color=c=0x404040:s=320x180:r=25:d=60 \
    -c:v mpeg4 -g 1 -q:v 2 "$output/subtitle-style.mp4"
cat > "$output/subtitle-style.srt" <<'EOF'
1
00:00:00,000 --> 00:00:59,900
<font color="#00FF00">iiiiiiii</font>
EOF

cat > "$output/duration-cues.srt" <<'EOF'
1
00:00:01,000 --> 00:00:02,000
<font color="#00FF00">iiiiiiii</font>

2
00:00:04,000 --> 00:00:06,000
<font color="#00FF00">iiiiiiii</font>
EOF

cat > "$output/chapters.ffmetadata" <<'EOF'
;FFMETADATA1
title=Juxtopposed Native Chapter Fixture
[CHAPTER]
TIMEBASE=1/1000
START=0
END=6000
title=Red
[CHAPTER]
TIMEBASE=1/1000
START=6000
END=12000
title=Blue
[CHAPTER]
TIMEBASE=1/1000
START=12000
END=18000
title=Green
EOF

cat > "$output/chaptered-colors.srt" <<'EOF'
1
00:00:00,000 --> 00:00:05,900
Red segment

2
00:00:06,000 --> 00:00:11,900
Blue segment

3
00:00:12,000 --> 00:00:17,900
Green segment
EOF

ffmpeg -hide_banner -loglevel error -n -i "$output/paused-seek-colors.mp4" \
    -f lavfi -i sine=frequency=440:sample_rate=44100:duration=18 \
    -f ffmetadata -i "$output/chapters.ffmetadata" -i "$output/chaptered-colors.srt" \
    -map 0:v -map 1:a -map 3:s -map_metadata 2 -map_chapters 2 \
    -c:v copy -c:a flac -c:s srt "$output/chaptered-colors.mkv"

ffmpeg -hide_banner -loglevel error -n -f lavfi \
    -i 'aevalsrc=0.4*sin(2*PI*220*t)*(0.6+0.4*sin(2*PI*0.2*t)):s=44100:d=30' \
    -c:a pcm_s16le -metadata title='Juxtopposed Waveform Fixture' \
    -metadata artist='Generated test audio' "$output/waveform-audio.wav"

ffprobe -v error -show_chapters -show_streams -show_format -of json \
    "$output/chaptered-colors.mkv" > "$output/chaptered-colors.json"
ffprobe -v error -show_streams -show_format -of json \
    "$output/waveform-audio.wav" > "$output/waveform-audio.json"
