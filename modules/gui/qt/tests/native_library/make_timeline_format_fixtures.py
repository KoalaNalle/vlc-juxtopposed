# SPDX-License-Identifier: GPL-2.0-or-later
"""Generate synthetic codec/container preview cases with FFmpeg references."""
from pathlib import Path
from hashlib import sha256
import argparse
import json
import shutil
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("source", type=Path, help="Generated 18s red/green/blue MP4")
parser.add_argument("output", type=Path, help="New output directory")
args = parser.parse_args()
source = args.source.resolve(strict=True)
output = args.output.resolve()
if output.exists():
    parser.error("Output directory must not exist")
output.mkdir(parents=True)

def ffmpeg(*arguments):
    subprocess.run(["ffmpeg", "-nostdin", "-hide_banner", "-loglevel", "error",
                    *map(str, arguments)], check=True)

def digest(path):
    return sha256(path.read_bytes()).hexdigest()

subtitle = output / "synthetic.srt"
subtitle.write_text("1\n00:00:01,000 --> 00:00:04,000\nSynthetic preview subtitle\n\n"
                    "2\n00:00:07,000 --> 00:00:10,000\nIndependent preview track\n",
                    encoding="utf-8")
shutil.copyfile(source, output / "h264.mp4")
ffmpeg("-i", source, "-f", "lavfi", "-i", "anullsrc=r=48000:cl=stereo",
       "-i", subtitle, "-map", "0:v:0", "-map", "1:a:0", "-map", "1:a:0",
       "-map", "2:s:0", "-t", "18", "-c:v", "libx265", "-threads", "2",
       "-preset", "ultrafast", "-x265-params", "pools=2:frame-threads=2:log-level=error:keyint=25:min-keyint=25:scenecut=0",
       "-crf", "24", "-pix_fmt", "yuv420p", "-c:a", "aac", "-b:a", "64k",
       "-c:s", "srt", "-metadata:s:a:0", "language=eng", "-metadata:s:a:1", "language=fin",
       output / "hevc-multitrack.mkv")
ffmpeg("-i", source, "-f", "lavfi", "-i", "anullsrc=r=48000:cl=stereo",
       "-map", "0:v:0", "-map", "1:a:0", "-t", "18", "-c:v", "libvpx-vp9",
       "-threads", "2", "-deadline", "realtime", "-cpu-used", "8", "-g", "25",
       "-b:v", "300k", "-pix_fmt", "yuv420p", "-c:a", "libopus", "-b:a", "32k",
       output / "vp9.webm")
ffmpeg("-i", source, "-c:v", "mpeg4", "-threads", "2", "-q:v", "3", "-g", "25",
       "-pix_fmt", "yuv420p", "-an", output / "mpeg4.avi")
ffmpeg("-i", source, "-c:v", "mpeg2video", "-threads", "2", "-q:v", "3", "-g", "12",
       "-pix_fmt", "yuv420p", "-an", output / "mpeg2.mpg")
ffmpeg("-i", source, "-c:v", "copy", "-an", "-f", "mpegts", output / "h264.ts")
ffmpeg("-f", "lavfi", "-i", "anullsrc=r=48000:cl=stereo", "-t", "3", "-c:a", "flac",
       output / "audio-only.flac")

cases = {}
for name in ["h264.mp4", "hevc-multitrack.mkv", "vp9.webm", "mpeg4.avi", "mpeg2.mpg", "h264.ts", "audio-only.flac"]:
    path = output / name
    streams = json.loads(subprocess.check_output(
        ["ffprobe", "-v", "error", "-show_streams", "-show_format", "-of", "json", str(path)], text=True))
    (output / (name + ".metadata.json")).write_text(json.dumps(streams, indent=2) + "\n", encoding="utf-8")
    references = {}
    if name != "audio-only.flac":
        for second in [2, 8, 14]:
            reference = output / f"{name}-{second}.png"
            ffmpeg("-ss", second, "-i", path, "-map", "0:v:0", "-an", "-sn",
                   "-vf", "scale=iw*sar:ih,setsar=1,crop='min(iw,ih*252/142)':'min(ih,iw*142/252)',scale=252:142:flags=bicubic",
                   "-frames:v", "1", "-update", "1", reference)
            references[str(second)] = {"path": reference.name, "sha256": digest(reference)}
    cases[name] = {"sha256": digest(path), "references": references,
                   "codecs": [{"type": s.get("codec_type"), "codec": s.get("codec_name")} for s in streams["streams"]]}
assert sum(s["type"] == "audio" for s in cases["hevc-multitrack.mkv"]["codecs"]) == 2
assert sum(s["type"] == "subtitle" for s in cases["hevc-multitrack.mkv"]["codecs"]) == 1
(output / "manifest.json").write_text(json.dumps({"synthetic": True, "sourceSha256": digest(source),
    "generatorSha256": digest(Path(__file__)), "cases": cases}, indent=2) + "\n", encoding="utf-8")
print(f"Generated {len(cases)} synthetic formats with independent FFmpeg references in {output}")
