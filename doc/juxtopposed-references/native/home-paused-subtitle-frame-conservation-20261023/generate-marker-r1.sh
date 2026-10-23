#!/usr/bin/env bash
set -euo pipefail
repo=/mnt/c/Modding/vlc-juxtopposed
work="$repo/build-windows/beta-subtitle-frame-conservation-20261023"
source="$repo/build-windows/beta-tracks-subtitles-20261023/media-generator-smoke-r35/colored-long.mkv"
out="$work/media-r1"
test ! -e "$out"
mkdir "$out"
exec > "$work/generate-marker-r1.log" 2>&1
trap 'code=$?; printf "%s\n" "$code" > "$work/generate-marker-r1-exit.txt"; exit "$code"' EXIT
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
