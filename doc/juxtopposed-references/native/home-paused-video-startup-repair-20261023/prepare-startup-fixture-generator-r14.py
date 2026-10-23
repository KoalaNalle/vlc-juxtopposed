from pathlib import Path
work=Path(__file__).resolve().parent
repo=Path('C:/Modding/vlc-juxtopposed')
text=(work/'generate-fixtures-r1.sh').read_text(encoding='utf-8')
before='work=/mnt/c/Modding/vlc-juxtopposed/build-windows/beta-tracks-subtitles-20261023\nout="$work/media-r1"'
assert text.count(before)==1
text=text.replace(before, 'if [ "$#" -ne 1 ]; then\n    printf "Usage: %s OUTPUT_DIRECTORY\\n" "$0" >&2\n    exit 2\nfi\nout="$1"')
text=text.replace('#!/usr/bin/env bash', '#!/usr/bin/env bash\n# SPDX-License-Identifier: GPL-2.0-or-later')
text+='''
ffmpeg -hide_banner -loglevel error -nostdin -i "$out/multi-language.mkv" -map 0:a:0 -c copy "$out/audio-only.mka"
ffmpeg -hide_banner -loglevel error -nostdin -f lavfi -i 'color=c=red:s=960x540:r=25:d=0.04' -frames:v 1 -c:v libx264 -threads 2 -pix_fmt yuv420p "$out/single-picture.mkv"
ffmpeg -hide_banner -loglevel error -nostdin -copyts -i "$out/multi-language.mkv" -itsoffset 3 -i "$out/multi-language.mkv" -map 1:v:0 -map 0:a:0 -c copy -t 8 -avoid_negative_ts disabled "$out/delayed-video.mkv"
ffmpeg -hide_banner -loglevel error -nostdin -copyts -i "$out/multi-language.mkv" -map 0 -c copy -avoid_negative_ts disabled "$out/preserve.mkv"
for name in audio-only.mka single-picture.mkv delayed-video.mkv preserve.mkv; do
    ffprobe -v error -show_streams -show_format -of json "$out/$name" > "$out/$name.probe.json"
    sha256sum "$out/$name"
done
'''
target=repo/'test/libvlc/generate_paused_video_startup.sh'
assert not target.exists();target.write_text(text,encoding='utf-8',newline='\n')
