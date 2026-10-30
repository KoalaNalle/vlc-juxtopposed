from pathlib import Path
from hashlib import sha256
import json
import subprocess
import sys

root = Path(__file__).resolve().parent
label = sys.argv[1]
assert label.isidentifier()
run = root / label
file, = (run / 'recordings').glob('*.mkv')
fixture = root / 'media-formats/subtitle-style.ass'
digest = lambda p: sha256(p.read_bytes()).hexdigest()
before = digest(file)
source_hash = digest(fixture)

def command(args):
    r = subprocess.run(args, capture_output=True, check=True)
    assert not r.stderr, r.stderr.decode(errors='replace')
    return r.stdout.decode()

def timestamp(s):
    h, m, sec = s.split(':')
    return int(h) * 3600 + int(m) * 60 + float(sec)

probe = json.loads(command(['ffprobe', '-v', 'error', '-show_packets',
    '-show_streams', '-of', 'json', str(file)]))
video = next(s for s in probe['streams'] if s['codec_type'] == 'video')
sub = next(s for s in probe['streams'] if s['codec_type'] == 'subtitle')
assert sub['codec_name'] == 'ass'
# Matroska does not store the initial H.264 decode timestamps. Match the
# recorded first presentation timestamp to the immutable MP4 fixture's keyframe.
first_pts = min(float(p['pts_time']) for p in probe['packets'] if p['stream_index'] == video['index'])
source_video = root / 'media-formats/frame-marker.mp4'
verification = json.loads((run / 'verification.json').read_text())
assert digest(source_video) == verification['fixtureSha256']['frame-marker.mp4']
original = json.loads(command(['ffprobe', '-v', 'error', '-select_streams', 'v:0', '-show_packets', '-of', 'json', str(source_video)]))
keyframe, = [p for p in original['packets'] if abs(float(p['pts_time']) - first_pts) < .00001]
assert 'K' in keyframe['flags']
start = float(keyframe['dts_time'])
packets = [p for p in probe['packets'] if p['stream_index'] == sub['index']]
export = command(['ffmpeg', '-v', 'error', '-i', str(file), '-map', '0:s:0',
    '-c:s', 'copy', '-f', 'ass', '-'])
source = fixture.read_text()
styles = [line for line in source.splitlines() if line.startswith(('Style:', 'PlayResX:', 'PlayResY:'))]
assert styles and all(line in export.splitlines() for line in styles)
source_cues = {}
for line in source.splitlines():
    if line.startswith('Dialogue:'):
        parts = line.split(',', 9)
        source_cues[parts[-1]] = (timestamp(parts[1]) / 2, timestamp(parts[2]) / 2)
actual = [line.split(',', 9) for line in export.splitlines() if line.startswith('Dialogue:')]
assert len(actual) == len(packets) == 2
assert [parts[-1] for parts in actual] == ['EXTERNAL MIDDLE CUE', 'EXTERNAL LAST CUE']
checks = []
for event, packet in zip(actual, packets):
    original_start, original_end = source_cues[event[-1]]
    expected_start = max(original_start, start)
    expected_duration = original_end - expected_start
    pts = float(packet['pts_time'])
    duration = float(packet.get('duration_time', 0))
    checks.append(dict(text=event[-1], pts=pts, duration=duration,
        expectedStart=expected_start, expectedDuration=expected_duration,
        passed=expected_duration > 0 and abs(pts - expected_start) <= .002
        and abs(duration - expected_duration) <= .002))
assert digest(file) == before and digest(fixture) == source_hash
result = dict(status='passed' if all(c['passed'] for c in checks) else 'failed',
    recordingSha256=before, sourceSubtitleSha256=source_hash,
    producerSha256=digest(Path(__file__)), videoStartDts=start, sourceVideoSha256=digest(source_video), firstVideoPts=first_pts,
    styleAndResolutionPreserved=True, events=checks,
    scope='Exact generated ASS cue text and finite intervals at 50 FPS, clipped to original fixture keyframe DTS matched to the recorded first video PTS. Native packet/export validation; not ordinary recorded-file playback or final-package acceptance.')
(run / 'ass-subtitle-check-r3.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result, indent=2))
sys.exit(0 if result['status'] == 'passed' else 1)
