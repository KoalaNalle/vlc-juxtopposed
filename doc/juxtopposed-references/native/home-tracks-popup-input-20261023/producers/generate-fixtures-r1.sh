#!/usr/bin/env bash
set -euo pipefail
work=/mnt/c/Modding/vlc-juxtopposed/build-windows/beta-tracks-subtitles-20261023
out="$work/media-r1"
test ! -e "$out"
mkdir "$out"
cat > "$out/embedded-english.srt" <<'EOF'
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
cat > "$out/embedded-finnish.srt" <<'EOF'
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
cat > "$out/manual-external.srt" <<'EOF'
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
ffmpeg -hide_banner -loglevel error -nostdin \
  -f lavfi -i 'color=c=0x202020:s=960x540:r=25:d=60' \
  -f lavfi -i 'sine=frequency=440:sample_rate=48000:duration=60' \
  -f lavfi -i 'sine=frequency=880:sample_rate=48000:duration=60' \
  -i "$out/embedded-english.srt" -i "$out/embedded-finnish.srt" \
  -map 0:v:0 -map 1:a:0 -map 2:a:0 -map 3:s:0 -map 4:s:0 \
  -c:v libx264 -preset veryfast -crf 28 -g 25 -pix_fmt yuv420p \
  -c:a aac -b:a 96k -c:s srt -t 60 \
  -metadata:s:a:0 language=eng -metadata:s:a:0 title='English - 440 Hz' \
  -metadata:s:a:1 language=fin -metadata:s:a:1 title='Finnish - 880 Hz' \
  -metadata:s:s:0 language=eng -metadata:s:s:0 title='English embedded' \
  -metadata:s:s:1 language=fin -metadata:s:s:1 title='Finnish embedded' \
  -disposition:a:0 default -disposition:a:1 0 -disposition:s:0 default -disposition:s:1 0 \
  "$out/multi-language.mkv"
ffmpeg -hide_banner -loglevel error -nostdin -i "$out/multi-language.mkv" \
  -map 0:v:0 -map 0:a:0 -c copy "$out/plain.mkv"
ffprobe -v error -show_streams -show_format -of json "$out/multi-language.mkv" > "$out/multi-probe.json"
ffprobe -v error -show_streams -show_format -of json "$out/plain.mkv" > "$out/plain-probe.json"
sha256sum "$out/multi-language.mkv" "$out/plain.mkv" "$out/manual-external.srt"
