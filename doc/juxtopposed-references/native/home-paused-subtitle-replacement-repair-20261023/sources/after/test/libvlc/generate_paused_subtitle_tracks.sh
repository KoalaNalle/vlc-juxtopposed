#!/usr/bin/env bash
set -euo pipefail
if [ "$#" -ne 2 ]; then
    printf "Usage: %s COLORED_LONG_MKV NEW_OUTPUT_DIRECTORY\n" "$0" >&2
    exit 2
fi
source="$1"
out="$2"
test -f "$source"
test ! -e "$out"
mkdir "$out"
ffmpeg -hide_banner -loglevel error -nostdin -copyts -i "$source" -map 0 \
    -vf "format=gbrp,geq=r='if(lt(Y,40),if(lt(X,704),255*mod(floor(N/pow(2,floor(X/64))),2),0),r(X,Y))':g='if(lt(Y,40),0,g(X,Y))':b='if(lt(Y,40),0,b(X,Y))',format=yuv420p" \
    -c:v libx264 -preset veryfast -crf 18 -threads 2 -c:a copy -c:s copy \
    -fps_mode passthrough -cluster_time_limit 1000 -avoid_negative_ts disabled "$out/frame-marker.mkv"
ffprobe -v error -show_streams -show_format -of json "$out/frame-marker.mkv" > "$out/probe.json"
ffprobe -v error -select_streams v -show_frames -show_entries frame=best_effort_timestamp_time,key_frame -of json "$out/frame-marker.mkv" > "$out/frames.json"
for frame in 0 125 126 625; do
    ffmpeg -hide_banner -loglevel error -nostdin -i "$out/frame-marker.mkv" \
        -vf "select=eq(n\,$frame)" -frames:v 1 "$out/frame-$frame.png"
done
sha256sum "$source" "$out/frame-marker.mkv"

cat > "$out/external.srt" <<'EOF'
1
00:00:00,000 --> 00:00:19,999
EXTERNAL FIRST CUE

2
00:00:20,000 --> 00:00:39,999
EXTERNAL MIDDLE CUE

3
00:00:40,000 --> 00:00:59,900
EXTERNAL LAST CUE
EOF
ffmpeg -hide_banner -loglevel error -nostdin -copyts -i "$out/frame-marker.mkv" \
    -map 0 -c:v copy -c:a copy -c:s ass -avoid_negative_ts disabled "$out/frame-marker-ass.mkv"
ffmpeg -hide_banner -loglevel error -nostdin -copyts -i "$out/frame-marker.mkv" \
    -map 0 -c:v copy -c:a copy -c:s mov_text -avoid_negative_ts disabled "$out/frame-marker.mp4"
for name in frame-marker-ass.mkv frame-marker.mp4; do
    ffprobe -v error -show_streams -show_format -of json "$out/$name" > "$out/$name-probe.json"
    sha256sum "$out/$name"
done
