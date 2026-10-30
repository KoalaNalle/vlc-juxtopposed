"""Read generated probe artifacts with installed WSL FFmpeg tools; no VLC job."""
from pathlib import Path
from hashlib import sha256
import json
import os
import re
import select
import signal
import subprocess
import time
import sys

prep = Path(__file__).resolve().parent
repo = Path('/mnt/c/Modding/vlc-juxtopposed')
variant = sys.argv[1]
assert variant in ('control', 'candidate', 'reversed')
out = prep / ('runtime-' + variant + '-r1')
def analyze_observation():
    config = json.loads((prep / 'runtime-config.json').read_text(encoding='utf-8-sig'))
    record = json.loads((out / 'verification.json').read_text())
    assert record['status'] == 'terminal' and record['normalExit'] and record['exitCode'] == 0
    assert record['variant'] == variant
    assert not record.get('failure') and not record.get('inspectionErrors')
    assert record['diagnostics'] == [] and record['moduleCounts'] == ['603'] and record['selectedLibavformat']
    required_guards = ('sourceUnchanged', 'fixtureUnchanged', 'producersUnchanged', 'configUnchanged',
        'buildReceiptUnchanged', 'unitReceiptUnchanged', 'preservedRuntimeUnchanged', 'runtimeUnchanged', 'humanConfigUnchanged')
    assert all(record.get(key) is True for key in required_guards)
    assert sha256((prep / 'runtime-config.json').read_bytes()).hexdigest() == record['configSha256']
    assert all(sha256((prep / p).read_bytes()).hexdigest() == h for p,h in config['producerSha256'].items())
    assert all(value for key, value in record.items() if key.endswith('Unchanged'))
    def run(arguments):
        result = subprocess.run(arguments, capture_output=True, check=True, timeout=90)
        assert not result.stderr, result.stderr.decode(errors='replace')
        return result.stdout

    def analyze(path):
        streams = json.loads(run(['ffprobe', '-v', 'error', '-show_streams', '-of', 'json', str(path)]))['streams']
        packets = json.loads(run(['ffprobe', '-v', 'error', '-select_streams', 'a:0', '-show_packets',
            '-show_data_hash', 'sha256', '-of', 'json', str(path)]))['packets']
        frames = json.loads(run(['ffprobe', '-v', 'error', '-select_streams', 'a:0', '-show_frames',
            '-show_entries', 'frame=pts_time,nb_samples', '-of', 'json', str(path)]))['frames']
        pcm = run(['ffmpeg', '-v', 'error', '-threads', '1', '-i', str(path), '-map', '0:a:0', '-f', 's16le', '-'])
        return streams, packets, frames, pcm

    source = repo / record['fixture']
    output = out / 'output.mkv'
    assert sha256(source.read_bytes()).hexdigest() == record['fixtureSha256']
    assert sha256(output.read_bytes()).hexdigest() == record['outputSha256']
    source_streams, source_packets, source_frames, source_pcm = analyze(source)
    streams, packets, frames, pcm = analyze(output)
    assert sorted((s['codec_type'], s['codec_name']) for s in streams) == [('audio', 'flac'), ('subtitle', 'subrip'), ('subtitle', 'subrip'), ('video', 'h264')]
    stride = 960 * 40 * 3
    # Stream one cropped frame at a time instead of buffering172.8MB of raw video.
    video_command = ['ffmpeg', '-v', 'error', '-threads', '1', '-i', str(output), '-map', '0:v:0',
        '-vf', 'crop=960:40:0:0', '-fps_mode', 'passthrough', '-pix_fmt', 'rgb24', '-f', 'rawvideo', '-']
    markers = []
    with (out / 'video-decode.stderr.txt').open('wb') as video_errors:
        video_process = subprocess.Popen(video_command, stdout=subprocess.PIPE, stderr=video_errors, start_new_session=True)
        try:
            deadline = time.monotonic() + 90
            while True:
                frame = bytearray()
                while len(frame) < stride:
                    remaining = deadline - time.monotonic()
                    assert remaining > 0 and select.select([video_process.stdout], [], [], remaining)[0], 'Video decode timed out'
                    chunk = os.read(video_process.stdout.fileno(), stride - len(frame))
                    if not chunk: break
                    frame.extend(chunk)
                if not frame: break
                assert len(frame) == stride, 'Partial decoded marker frame'
                markers.append(sum(1 << bit for bit in range(11) if frame[(20 * 960 + 32 + 64 * bit) * 3] > 128))
                assert len(markers) <= 1500, 'Unexpected extra decoded frames'
            assert video_process.wait(timeout=max(.1, deadline - time.monotonic())) == 0
        finally:
            if video_process.poll() is None:
                os.killpg(video_process.pid, signal.SIGKILL)
                video_process.wait(timeout=10)
    assert not (out / 'video-decode.stderr.txt').read_bytes()
    subtitle_checks = []
    all_source_packets = json.loads(run(['ffprobe', '-v', 'error', '-show_packets', '-of', 'json', str(source)]))['packets']
    all_packets = json.loads(run(['ffprobe', '-v', 'error', '-show_packets', '-of', 'json', str(output)]))['packets']
    for index in range(2):
        before = [s for s in source_streams if s['codec_type'] == 'subtitle'][index]
        after = [s for s in streams if s['codec_type'] == 'subtitle'][index]
        old = [p for p in all_source_packets if p['stream_index'] == before['index']]
        new = [p for p in all_packets if p['stream_index'] == after['index']]
        timings = len(old) == len(new) > 0 and all(abs(float(a['pts_time']) - float(b['pts_time'])) <= .002
            and abs(float(a['duration_time']) - float(b.get('duration_time', 0))) <= .002 for a, b in zip(old, new))
        old_srt = run(['ffmpeg', '-v', 'error', '-i', str(source), '-map', f'0:s:{index}', '-c:s', 'copy', '-f', 'srt', '-'])
        new_srt = run(['ffmpeg', '-v', 'error', '-i', str(output), '-map', f'0:s:{index}', '-c:s', 'copy', '-f', 'srt', '-'])
        subtitle_checks.append({'index': index, 'timingPreserved': timings, 'exactSrtExport': old_srt == new_srt})
    hashes = [packet['data_hash'] for packet in packets]
    source_hashes = [packet['data_hash'] for packet in source_packets]
    strict_audio = hashes == source_hashes and pcm == source_pcm
    source_samples = sum(int(frame['nb_samples']) for frame in source_frames)
    output_samples = sum(int(frame['nb_samples']) for frame in frames)
    assert source_samples == len(source_pcm) // 2 and output_samples == len(pcm) // 2
    pcm_prefix = pcm == source_pcm[:len(pcm)]
    known_tail_loss = (len(source_packets) == 626 and len(packets) == 625
        and hashes == source_hashes[:-1] and pcm_prefix
        and source_samples == 2880528 and output_samples == 2880000
        and len(source_pcm) - len(pcm) == 1056
        and int(source_packets[-1]['size']) == 619
        and abs(float(source_packets[-1]['pts_time']) - 60.) < .000001
        and abs(float(source_packets[-1]['dts_time']) - 60.) < .000001
        and source_packets[-1]['data_hash'] == 'SHA256:7b2a0c0f0e91ab6804b11077160743336c3ead5f7c1c583e4a4c072946766343')
    result = {'status': 'expected-control-tail-loss' if variant == 'control' else 'passed',
        'scope': 'Strict real-writer Matroska comparison; separate from ordinary recording-toggle/UI or final package.',
        'variant': variant, 'esRegistrationCodecs': record['esRegistrationCodecs'], 'esDeletionOrder': record['esDeletionOrder'],
        'producerSha256': sha256(Path(__file__).read_bytes()).hexdigest(), 'audioPackets': len(packets), 'sourceAudioPackets': len(source_packets),
        'audioSamples': output_samples, 'sourceAudioSamples': source_samples,
        'packetPayloadsExactSource': hashes == source_hashes,
        'packetPayloadsExactSourcePrefix': hashes == source_hashes[:len(hashes)],
        'exactSourcePcm': pcm == source_pcm, 'sourcePcmSha256': sha256(source_pcm).hexdigest(), 'outputPcmSha256': sha256(pcm).hexdigest(),
        'pcmExactSourcePrefix': pcm_prefix, 'missingSourcePcmBytes': len(source_pcm) - len(pcm), 'known528SampleTailLoss': known_tail_loss,
        'strictAudioCompleteness': strict_audio, 'videoFrames': len(markers), 'all1500Markers': markers == list(range(1500)),
        'subtitleChecks': subtitle_checks, 'verificationSha256': sha256((out / 'verification.json').read_bytes()).hexdigest()}
    assert result['all1500Markers'] and all(check['timingPreserved'] and check['exactSrtExport'] for check in subtitle_checks)
    assert len(source_packets) == 626 and result['sourceAudioSamples'] == 2880528
    def packet_times_match(before, after):
        return len(after) <= len(before) and all(
            all(key in old and key in new and abs(float(old[key]) - float(new[key])) <= .002
                for key in ('pts_time', 'duration_time'))
            for old, new in zip(before, after))
    source_video = next(s['index'] for s in source_streams if s['codec_type'] == 'video')
    output_video = next(s['index'] for s in streams if s['codec_type'] == 'video')
    old_video_packets = [p for p in all_source_packets if p['stream_index'] == source_video]
    new_video_packets = [p for p in all_packets if p['stream_index'] == output_video]
    result['audioPrefixTimelinePreserved'] = packet_times_match(source_packets, packets)
    result['videoTimelinePreserved'] = (len(old_video_packets) == len(new_video_packets) == 1500
        and packet_times_match(old_video_packets, new_video_packets))
    result['accepted'] = (result['audioPrefixTimelinePreserved'] and result['videoTimelinePreserved']
        and (strict_audio and len(packets) == 626 and output_samples == 2880528
             if variant != 'control' else known_tail_loss))
    assert sha256(source.read_bytes()).hexdigest() == record['fixtureSha256']
    assert sha256(output.read_bytes()).hexdigest() == record['outputSha256']
    return result


def interrupted(signum, frame):
    raise InterruptedError('Analyzer signal ' + str(signum))
for signum in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
    signal.signal(signum, interrupted)

result = {'status': 'analysis-failed', 'scope': 'Real-writer repair comparison; failed analysis is not acceptance.'}
failure = None
try:
    result = analyze_observation()
    assert result['accepted'], 'Strict comparison failed; inspect retained metrics'
except BaseException as error:
    failure = error
    result.update(status='analysis-failed', failure=repr(error))
finally:
    # A missing output/log or failed decoder still produces a terminal analysis.
    (out / 'analysis.json').write_text(json.dumps(result, indent=2) + '\n')
if failure is not None:
    raise failure
print(json.dumps(result, indent=2))
