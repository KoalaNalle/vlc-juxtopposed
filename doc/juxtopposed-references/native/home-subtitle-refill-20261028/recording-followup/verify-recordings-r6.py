from pathlib import Path
from hashlib import sha256
import json
import subprocess
import sys

root = Path(__file__).parent
for label in sys.argv[1:]:
    run = root / label
    record = json.loads((run / 'verification.json').read_text())
    assert record['status'] == 'terminal' and record['exitCode'] == 0
    assert all(record[k] for k in ['sourceUnchanged', 'testUnchanged', 'fixturesUnchanged', 'runtimeUnchanged', 'producerUnchanged'])
    outputs = sorted((run / 'recordings').iterdir())
    assert outputs and all(p.is_file() for p in outputs)
    rows = []
    for path in outputs:
        before = sha256(path.read_bytes()).hexdigest()
        probe = subprocess.run(['ffprobe', '-v', 'error', '-show_streams', '-show_format', '-of', 'json', str(path)], capture_output=True, check=True)
        assert not probe.stderr, probe.stderr.decode(errors='replace')
        metadata = json.loads(probe.stdout)
        video = next(s for s in metadata['streams'] if s['codec_type'] == 'video')
        audio = next(s for s in metadata['streams'] if s['codec_type'] == 'audio')
        assert video['width'] == 960 and video['height'] == 540
        decoded = subprocess.run(['ffmpeg', '-v', 'error', '-threads', '1', '-i', str(path), '-map', '0:v:0', '-vf', 'crop=960:40:0:0', '-fps_mode', 'passthrough', '-pix_fmt', 'rgb24', '-f', 'rawvideo', '-'], capture_output=True, check=True)
        assert not decoded.stderr, decoded.stderr.decode(errors='replace')
        stride = 960 * 40 * 3
        assert len(decoded.stdout) % stride == 0
        markers = [sum(1 << bit for bit in range(11) if decoded.stdout[offset + (20 * 960 + 32 + 64 * bit) * 3] > 128) for offset in range(0, len(decoded.stdout), stride)]
        assert len(markers) >= 50 and all(b == a + 1 for a, b in zip(markers, markers[1:])), markers
        assert 0 <= markers[0] < markers[-1] < 1500
        pcm = subprocess.run(['ffmpeg', '-v', 'error', '-threads', '1', '-i', str(path), '-map', '0:a:0', '-f', 's16le', '-'], capture_output=True, check=True)
        assert not pcm.stderr, pcm.stderr.decode(errors='replace')
        frame_bytes = 2 * audio['channels']
        assert len(pcm.stdout) % frame_bytes == 0
        samples = len(pcm.stdout) // frame_bytes
        assert samples >= int(audio['sample_rate']) * 2
        assert sha256(path.read_bytes()).hexdigest() == before
        rows.append(dict(filename=path.name, sha256=before, size=path.stat().st_size,
            streams=[{k: s.get(k) for k in ['codec_type', 'codec_name', 'duration', 'nb_frames', 'sample_rate', 'channels']} for s in metadata['streams']],
            decodedFrames=len(markers), firstMarker=markers[0], lastMarker=markers[-1], consecutiveMarkers=True,
            decodedAudioSamplesPerChannel=samples, decodedAudioSeconds=samples / int(audio['sample_rate']),
            decoderDiagnostics=[], unchanged=True))
    result = dict(status='passed-scoped-recording-decode', producerSha256=sha256(Path(__file__).read_bytes()).hexdigest(),
        verificationSha256=sha256((run / 'verification.json').read_bytes()).hexdigest(), recordings=rows,
        scope='Recording stays enabled across a live external-subtitle FPS edit and subsequently produces decodable AV with consecutive frames. Video starts at the next keyframe AFTER the edit; this does not establish AV continuity across the edit, exact record-toggle boundary, subtitle timing parity, perceptual audio sync or final-package acceptance.')
    (run / 'recording-decode-r6.json').write_text(json.dumps(result, indent=2) + '\n')
    print(label, json.dumps(result), flush=True)
