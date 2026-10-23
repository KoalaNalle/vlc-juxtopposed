#!/usr/bin/env bash
set -euo pipefail
repo=/mnt/c/Modding/vlc-juxtopposed
before="$repo/build-windows/beta-subtitle-replacement-repair-20261023/media-r1"
out="$repo/build-windows/beta-external-subtitle-selection-20261023/media-extra-r2b"
test ! -e "$out"
mkdir "$out"
for file in frame-marker.mkv frame-marker-ass.mkv frame-marker.mp4 external.srt; do
    cp "$before/$file" "$out/$file"
done
cat > "$out/external-finnish.srt" <<'EOF'
1
00:00:00,000 --> 00:00:19,999
ULKOINEN ENSIMMAINEN

2
00:00:20,000 --> 00:00:39,999
ULKOINEN TOINEN

3
00:00:40,000 --> 00:00:59,900
ULKOINEN KOLMAS
EOF
ffmpeg -nostdin -v error -i "$out/external.srt" -c:s ass "$out/external.ass"
ffmpeg -nostdin -v error -i "$out/external.srt" -i "$out/external-finnish.srt" -map 0:s -map 1:s -c:s srt -metadata:s:s:0 language=eng -metadata:s:s:1 language=fin -f matroska "$out/multi-language.mks"
ffprobe -v error -show_streams -of json "$out/multi-language.mks" > "$out/multi-language.mks-probe.json"
sha256sum "$out/external.ass" "$out/multi-language.mks"
