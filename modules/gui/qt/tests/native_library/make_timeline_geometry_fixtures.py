# SPDX-License-Identifier: GPL-2.0-or-later
"""Generate owned preview geometry media and independent FFmpeg references.

Requires existing ffmpeg/ffprobe. Supply a new output directory; no downloads,
private media, network service or additional Python packages are used.
"""
from pathlib import Path
from hashlib import sha256
import argparse
import json
import shutil
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('output', type=Path)
parser.add_argument('--duration',type=int,default=2,
                    help='Clip length in seconds; use18 for visible playback/resume journeys.')
args = parser.parse_args()
assert 2 <= args.duration <= 60, 'Use a synthetic clip duration from2 to60 seconds'
out = args.output.resolve()
assert not out.exists(), 'Use a new output directory'
ffmpeg, ffprobe = shutil.which('ffmpeg'), shutil.which('ffprobe')
assert ffmpeg and ffprobe, 'Existing ffmpeg and ffprobe are required'
out.mkdir(parents=True)

def run(command):
    return subprocess.run(command, check=True, capture_output=True).stdout

def digest(path):
    return sha256(path.read_bytes()).hexdigest()

def source(name, width, height, sar):
    pixels = bytearray()
    colors = [(255, 0, 0), (0, 255, 0), (0, 0, 255), (255, 255, 0)]
    for y in range(height):
        for x in range(width):
            color = colors[int(x >= width / 2) + 2 * int(y >= height / 2)]
            if ((x + .5 - width / 2) * sar) ** 2 + (y + .5 - height / 2) ** 2 <= 24 ** 2:
                color = (255, 255, 255)
            pixels.extend(color)
    ppm = out / (name + '.ppm')
    ppm.write_bytes(f'P6\n{width} {height}\n255\n'.encode() + pixels)
    media = out / (name + '.mp4')
    run([ffmpeg, '-nostdin', '-v', 'error', '-loop', '1', '-framerate', '5',
         '-i', str(ppm), '-t', str(args.duration), '-vf', f'setsar={sar}', '-an',
         '-c:v', 'libx264', '-threads', '1', '-preset', 'veryfast', '-crf', '18',
         '-g', '5', '-bf', '0', '-pix_fmt', 'yuv420p', '-movflags', '+faststart', str(media)])
    return media

media = [source('landscape', 320, 180, 1), source('portrait', 180, 320, 1),
         source('sar-2-to-1', 320, 180, 2)]
for angle in [90, 180, 270]:
    target = out / f'rotate-{angle}.mp4'
    run([ffmpeg, '-nostdin', '-v', 'error', '-display_rotation', str(angle),
         '-i', str(media[0]), '-c', 'copy', str(target)])
    media.append(target)

records = {}
for path in media:
    reference = out / (path.stem + '-reference.png')
    # FFmpeg applies the container display matrix before filtering. First
    # normalize sample aspect ratio, then independently center-crop to the card.
    run([ffmpeg, '-nostdin', '-v', 'error', '-i', str(path), '-frames:v', '1',
         '-vf', "scale=w='trunc(iw*sar/2)*2':h=ih,setsar=1,scale=252:142:force_original_aspect_ratio=increase,crop=252:142",
         '-threads', '1', str(reference)])
    probe = json.loads(run([ffprobe, '-v', 'error', '-show_streams', '-show_format', '-of', 'json', str(path)]))
    stream = probe['streams'][0]
    if path.stem.startswith('rotate-'):
        expected = int(path.stem.split('-')[1])
        rotations = [side['rotation'] % 360 for side in stream.get('side_data_list', []) if 'rotation' in side]
        assert rotations == [expected], f'{path.name}: missing or wrong display rotation'
    if path.stem == 'sar-2-to-1':
        assert stream['sample_aspect_ratio'] == '2:1'
    if path.stem == 'portrait':
        assert stream['width'] == 180 and stream['height'] == 320
    records[path.name] = dict(sha256=digest(path), referenceSha256=digest(reference), probe=probe)
(out / 'manifest.json').write_text(json.dumps(dict(
    syntheticOnly=True, durationSeconds=args.duration, reference='FFmpeg autorotate, square pixels, center crop252x142',
    ffmpegVersion=run([ffmpeg, '-version']).decode().splitlines()[0],
    generatorSha256=digest(Path(__file__)), fixtures=records), indent=2) + '\n', encoding='utf-8')
print(f'Generated {len(media)} geometry clips and reference images in {out}')
