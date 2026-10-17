#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Check actual recording files and independent native output-track defaults."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import sys
from native_http_runtime import stage_runtime, regenerate_cache, run_owned


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def ebml_number(data, pos, identifier=False):
    first = data[pos]
    length = 1
    while length <= 8 and not first & (1 << (8-length)):
        length += 1
    assert length <= 8 and pos+length <= len(data), 'Invalid/truncated EBML number'
    value = first if identifier else first & ((1 << (8-length))-1)
    for byte in data[pos+1:pos+length]:
        value = (value << 8) | byte
    return value, length


def ebml_elements(data):
    pos = 0
    while pos < len(data):
        tag, length = ebml_number(data, pos, True)
        pos += length
        size, length = ebml_number(data, pos)
        pos += length
        if size == (1 << (7*length))-1:
            size = len(data)-pos
        yield tag, data[pos:pos+size]
        pos += size


def matroska_tracks(path):
    # Header metadata is sufficient; do not read an entire offered media file.
    with path.open('rb') as stream:
        data = stream.read(1024*1024)
    segment = next(body for tag, body in ebml_elements(data) if tag == 0x18538067)
    tracks = next(body for tag, body in ebml_elements(segment) if tag == 0x1654AE6B)
    result = []
    for tag, body in ebml_elements(tracks):
        if tag == 0xAE:
            fields = dict(ebml_elements(body))
            result.append(dict(type=int.from_bytes(fields[0x83]), codec=fields[0x86].decode('utf-8')))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['package', 'video-fixture', 'output-dir']:
        parser.add_argument('--'+name, type=Path, required=True)
    for name in ['core', 'x264', 'mkv', 'stream-sample']:
        parser.add_argument('--'+name, type=Path)
    parser.add_argument('--sample-start', type=float, default=60)
    args = parser.parse_args()
    source, fixture, out = (p.resolve() for p in [args.package, args.video_fixture, args.output_dir])
    assert not out.is_relative_to(source) and (source/'portable').is_dir()
    out.mkdir(parents=True, exist_ok=False)
    runtime = out/'runtime'
    selected = ['libvlccore.dll', 'plugins/codec/libx264_plugin.dll', 'plugins/demux/libmkv_plugin.dll']
    before = {name: digest(source/name) for name in selected}
    stage_runtime(source, runtime)
    for name, replacement in zip(selected, [args.core, args.x264, args.mkv]):
        if replacement:
            (runtime/name).unlink()  # Detach only the new immutable runtime link.
            os.link(replacement.resolve(), runtime/name)
    regenerate_cache(runtime, out/'cache')
    report = dict(passed=False, fullBetaAccepted=False, nativeWindowAccepted=False,
        runtimeSha256={name: digest(runtime/name) for name in selected}, cases={})

    def save():
        (out/'verification.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8', newline='\n')

    def execute(case, media, extra=(), config=''):
        case.mkdir()
        cfg = case/'vlcrc'
        cfg.write_text(config, encoding='utf-8', newline='\n')
        command = [runtime/'vlc.exe', '--intf=dummy', '--no-one-instance', '--no-ignore-config', '--config='+str(cfg),
            '--no-media-library', '--no-metadata-network-access', '--no-ml-network-discovery', '--no-auto-preparse',
            '--no-sub-autodetect-file', '--extraintf=', '--control=', '--no-hw-dec', '--avcodec-threads=1',
            '--vout=dummy', '--aout=dummy', '--play-and-exit', '--file-logging', '--verbose=2', '--log-verbose=2',
            '--logfile='+str(case/'vlc.log'), *extra, media]
        process = run_owned(command, runtime, case, timeout=30)
        text = (case/'vlc.log').read_text(errors='replace') if (case/'vlc.log').exists() else ''
        return process, text

    save()
    try:
        generated = out/'owned-h264.ts'
        chain = '#transcode{threads=1,vcodec=h264,venc=x264{preset=ultrafast,keyint=1},acodec=none}:std{access=file,mux=ts,dst='+generated.as_posix()+'}'
        process, text = execute(out/'generate-h264', fixture, ['--run-time=2', '--no-audio', '--no-spu', '--sout='+chain])
        assert process['normalExit'] and generated.exists() and generated.stat().st_size > 0
        report['fixture'] = dict(process=process, sha256=digest(generated), synthetic=True)
        for policy in [0, 1]:
            case = out/('record-native-'+str(policy))
            case.mkdir()
            folder = case/'Records Ää with spaces'
            folder.mkdir()
            cfg = case/'vlcrc'
            cfg.write_text('input-record-path='+str(folder)+'\ninput-record-native='+str(policy)+'\n', encoding='utf-8', newline='\n')
            process = run_owned([sys.executable, Path(__file__).with_name('native_recording_worker.py'),
                runtime, generated, cfg, case], runtime, case, timeout=30)
            files = [p for p in folder.iterdir() if p.is_file()]
            assert process['normalExit'] and len(files) == 1 and files[0].stat().st_size > 0
            text = (case/'vlc.log').read_text(errors='replace')
            decode = out/('decode-record-native-'+str(policy))
            # The scene filter creates its files in the case directory itself.
            replay, replay_log = execute(decode, files[0], ['--no-audio', '--no-spu', '--video-filter=scene',
                '--scene-path='+str(decode), '--scene-ratio=1', '--scene-width=320', '--scene-height=180',
                '--scene-prefix=owned-frame', '--scene-replace'])
            frames = list(decode.glob('*.png'))
            expected_backend = 'using stream_filter module "record"' if policy else 'using sout output module "record"'
            result = dict(process=process, replay=replay, decodedFrames=len(frames),
                backendObserved=expected_backend in text, h264Decoded='codec (h264) started' in replay_log,
                bytes=files[0].stat().st_size, sha256=digest(files[0]), container=files[0].suffix)
            result['passed'] = result['backendObserved'] and result['h264Decoded'] and replay['normalExit'] and len(frames)>0
            report['cases']['record-native-'+str(policy)] = result
            save()
            print('record-native-'+str(policy), 'PASS' if result['passed'] else 'FAIL', flush=True)
            assert result['passed']
        if args.stream_sample:
            sample = args.stream_sample.resolve()
            stat = (sample.stat().st_size, sample.stat().st_mtime_ns)
            assert {1, 2} <= {entry['type'] for entry in matroska_tracks(sample)}, 'Use a Matroska A/V sample'
            report['mediaPathsLogsAndStreamOutputsPrivate'] = True
            subtitle = out/'owned.srt'
            start = int(args.sample_start)
            assert start >= 0 and start <= 3600
            def timestamp(seconds):
                return f'{seconds//3600:02d}:{seconds//60%60:02d}:{seconds%60:02d},000'
            subtitle.write_text('1\n'+timestamp(start)+' --> '+timestamp(start+5)+'\nOWNED STREAM SUBTITLE\n', encoding='utf-8', newline='\n')
            specs = [('all', 1, 1, 1), ('no-audio', 0, 1, 1), ('no-video', 1, 0, 1),
                ('no-subtitles', 1, 1, 0), ('subtitles-only', 0, 0, 1), ('audio-only', 1, 0, 0), ('video-only', 0, 1, 0),
                ('explicit-options-override-defaults', 1, 1, 1)]
            for name, audio, video, subs in specs:
                case = out/name
                target = case/'private-output.mkv'
                chain = '#std{access=file,mux=mkv,dst='+target.as_posix()+'}'
                explicit = ['--sout-audio', '--sout-video', '--sout-spu'] if name.startswith('explicit-') else []
                config = 'sout-audio=0\nsout-video=0\nsout-spu=0\n' if explicit else f'sout-audio={audio}\nsout-video={video}\nsout-spu={subs}\n'
                process, text = execute(case, sample, ['--start-time='+str(args.sample_start), '--run-time=2',
                    '--sub-file='+str(subtitle), '--sub-track=0', '--sout='+chain, *explicit], config)
                entries = matroska_tracks(target) if target.exists() and target.stat().st_size else []
                expected = {kind for kind, enabled in [(2, audio), (1, video), (17, subs)] if enabled}
                result = dict(process=process, tracks=entries, expectedTrackTypes=sorted(expected),
                    subtitlePacketPresent=b'OWNED STREAM SUBTITLE' in target.read_bytes() if target.exists() else False,
                    sourceStatUnchanged=stat == (sample.stat().st_size, sample.stat().st_mtime_ns))
                result['passed'] = process['normalExit'] and {entry['type'] for entry in entries} == expected
                result['passed'] &= result['subtitlePacketPresent'] == bool(subs) and result['sourceStatUnchanged']
                report['cases'][name] = result
                save()
                print(name, 'PASS' if result['passed'] else 'FAIL', flush=True)
                assert result['passed']
        report['sourceRuntimeUnchanged'] = before == {name: digest(source/name) for name in selected}
        report['passed'] = report['sourceRuntimeUnchanged'] and all(case['passed'] for case in report['cases'].values())
        assert report['passed']
    finally:
        save()


if __name__ == '__main__':
    main()
