#!/usr/bin/env bash
set -euo pipefail
work=/mnt/c/Modding/vlc-juxtopposed/build-windows/beta-tracks-subtitles-20261023
out="$work/media-r20"
test ! -e "$out"
mkdir "$out"
cat > "$out/long-en.srt" <<'EOF'
1
00:00:00,000 --> 00:00:19,999
ENGLISH FIRST CUE

2
00:00:20,000 --> 00:00:39,999
ENGLISH MIDDLE CUE

3
00:00:40,000 --> 00:00:59,900
ENGLISH LAST CUE
EOF
cat > "$out/long-fi.srt" <<'EOF'
1
00:00:00,000 --> 00:00:19,999
SUOMI ENSIMMÄINEN

2
00:00:20,000 --> 00:00:39,999
SUOMI KESKIMMÄINEN

3
00:00:40,000 --> 00:00:59,900
SUOMI VIIMEINEN
EOF
sed -e 's/00:00:00,000/00:00:10,000/' -e 's/00:00:20,000/00:00:30,000/' -e 's/00:00:40,000/00:00:50,000/' "$out/long-en.srt" > "$out/delayed-en.srt"
sed -e 's/00:00:19,999/00:00:02,000/' -e 's/00:00:39,999/00:00:22,000/' -e 's/00:00:59,900/00:00:42,000/' "$out/long-en.srt" > "$out/gaps-en.srt"
ffmpeg -hide_banner -loglevel error -nostdin \
    -f lavfi -i 'color=c=red:s=960x540:r=25:d=10' \
    -f lavfi -i 'color=c=lime:s=960x540:r=25:d=10' \
    -f lavfi -i 'color=c=blue:s=960x540:r=25:d=10' \
    -f lavfi -i 'color=c=red:s=960x540:r=25:d=10' \
    -f lavfi -i 'color=c=lime:s=960x540:r=25:d=10' \
    -f lavfi -i 'color=c=blue:s=960x540:r=25:d=10' \
    -f lavfi -i 'sine=frequency=440:sample_rate=48000:duration=60' \
    -i "$out/long-en.srt" -i "$out/long-fi.srt" \
    -filter_complex '[0:v][1:v][2:v][3:v][4:v][5:v]concat=n=6:v=1:a=0[v]' \
    -filter_complex_threads 1 -map '[v]' -map 6:a -map 7:s -map 8:s \
    -c:v libx264 -threads 2 -preset veryfast -crf 28 -g 25 -pix_fmt yuv420p \
    -c:a aac -b:a 96k -c:s srt -t 60 -cluster_time_limit 1000 \
    -metadata:s:a:0 language=eng -metadata:s:s:0 language=eng -metadata:s:s:1 language=fin \
    -disposition:s:0 default -disposition:s:1 0 "$out/colored-long.mkv"
for kind in delayed gaps; do
    ffmpeg -hide_banner -loglevel error -nostdin -copyts -i "$out/colored-long.mkv" -i "$out/$kind-en.srt" \
        -map 0:v -map 0:a -map 1:s -c copy -cluster_time_limit 1000 -avoid_negative_ts disabled \
        -metadata:s:s:0 language=eng -disposition:s:0 default "$out/colored-$kind.mkv"
done
for kind in long delayed gaps; do
    ffprobe -v error -show_streams -show_format -of json "$out/colored-$kind.mkv" > "$out/$kind-probe.json"
    ffprobe -v error -select_streams s -show_packets -show_entries packet=pts_time,duration_time,pos -of json "$out/colored-$kind.mkv" > "$out/$kind-subtitle-packets.json"
    sha256sum "$out/colored-$kind.mkv"
done
