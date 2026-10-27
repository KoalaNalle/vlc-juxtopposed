#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Verify saved x264 defaults and overrides on actual encoded/decoded media."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
from native_http_runtime import stage_runtime, regenerate_cache, run_owned


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['package', 'video-fixture', 'output-dir']:
        parser.add_argument('--'+name, required=True, type=Path)
    for name in ['core', 'x264']:
        parser.add_argument('--'+name, type=Path)
    args = parser.parse_args()
    source, fixture, out = (p.resolve() for p in [args.package, args.video_fixture, args.output_dir])
    assert (source/'portable').is_dir() and not out.is_relative_to(source)
    out.mkdir(parents=True, exist_ok=False)
    runtime = out/'runtime'
    selected = ['libvlccore.dll', 'plugins/codec/libx264_plugin.dll']
    before = {name: digest(source/name) for name in selected}
    stage_runtime(source, runtime)
    for name, replacement in zip(selected, [args.core, args.x264]):
        if replacement:
            (runtime/name).unlink()
            os.link(replacement.resolve(), runtime/name)
    regenerate_cache(runtime, out/'cache')
    report = dict(passed=False, fullBetaAccepted=False, cases={},
        runtimeSha256={name: digest(runtime/name) for name in selected})

    def save():
        (out/'verification.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8', newline='\n')

    def execute(folder, media, extra, config=''):
        folder.mkdir()
        cfg = folder/'vlcrc'
        cfg.write_text(config, encoding='utf-8', newline='\n')
        command = [runtime/'vlc.exe', '--intf=dummy', '--no-one-instance', '--no-ignore-config', '--config='+str(cfg),
            '--no-media-library', '--no-metadata-network-access', '--no-ml-network-discovery', '--no-auto-preparse',
            '--extraintf=', '--control=', '--no-hw-dec', '--avcodec-threads=1', '--no-audio', '--no-spu', '--no-sub-autodetect-file',
            '--vout=dummy', '--play-and-exit', '--file-logging', '--verbose=2', '--log-verbose=2', '--logfile='+str(folder/'vlc.log'),
            *extra, media]
        process = run_owned(command, runtime, folder, timeout=30)
        text = (folder/'vlc.log').read_text(errors='replace') if (folder/'vlc.log').exists() else ''
        return process, text

    # External x264's preset/tune parameters, read back from its open encoder,
    # are stronger evidence than merely echoing the requested VLC preferences.
    specs = [
        dict(name='unset-defaults', config='', preset='medium', tune='', expected=(1, 3, 40)),
        dict(name='saved-ultrafast', config='sout-x264-preset=ultrafast\n', preset='ultrafast', tune='', expected=(0, 0, 0)),
        dict(name='saved-fast', config='sout-x264-preset=fast\n', preset='fast', tune='', expected=(1, 3, 30)),
        dict(name='saved-zerolatency', config='sout-x264-preset=fast\nsout-x264-tune=zerolatency\n', preset='fast', tune='zerolatency', expected=(1, 0, 0)),
        dict(name='none-after-tuning', config='sout-x264-preset=fast\nsout-x264-tune=\n', preset='fast', tune='', expected=(1, 3, 30)),
        dict(name='explicit-profile-wins', config='sout-x264-preset=ultrafast\nsout-x264-tune=film\n',
            encoder='x264{preset=fast,tune=zerolatency}', preset='fast', tune='zerolatency', expected=(1, 0, 0)),
        dict(name='explicit-cli-wins', config='sout-x264-preset=ultrafast\n', options=['--sout-x264-preset=fast'], preset='fast', tune='', expected=(1, 3, 30)),
    ]
    save()
    try:
        for spec in specs:
            folder = out/spec['name']
            target = folder/'owned-output.ts'
            chain = '#transcode{threads=1,vcodec=h264,venc='+spec.get('encoder', 'x264')+',acodec=none}:std{access=file,mux=ts,dst='+target.as_posix()+'}'
            process, text = execute(folder, fixture, ['--run-time=2', '--sout='+chain, *spec.get('options', [])], spec['config'])
            requested = re.findall(r'x264 requested preset: ([^;\n]+); tuning: ([^\r\n]*)', text)
            effective = [tuple(map(int, match)) for match in re.findall(r'x264 effective parameters: threads=(\d+) cabac=(\d+) bframes=(\d+) lookahead=(\d+)', text)]
            result = dict(process=process, requested=requested, effective=effective,
                expectedPreset=spec['preset'], expectedTuning=spec['tune'], expectedParameters=(1, *spec['expected']))
            result['passed'] = process['normalExit'] and bool(requested) and bool(effective)
            result['passed'] &= all(preset == spec['preset'] and tune == spec['tune'] for preset, tune in requested)
            result['passed'] &= all(params == (1, *spec['expected']) for params in effective)
            report['cases'][spec['name']] = result
            save()
            assert result['passed'], result
            decode = out/(spec['name']+'-decode')
            replay, replay_text = execute(decode, target, ['--video-filter=scene', '--scene-path='+str(decode),
                '--scene-ratio=5', '--scene-width=320', '--scene-height=180', '--scene-prefix=owned-frame', '--scene-replace'])
            result['replay'] = replay
            result['decodedFrames'] = len(list(decode.glob('*.png')))
            result['passed'] &= replay['normalExit'] and result['decodedFrames'] > 0 and 'codec (h264) started' in replay_text
            save()
            print(spec['name'], 'PASS' if result['passed'] else 'FAIL', flush=True)
            assert result['passed'], result
        report['sourceRuntimeUnchanged'] = before == {name: digest(source/name) for name in selected}
        report['passed'] = report['sourceRuntimeUnchanged'] and all(case['passed'] for case in report['cases'].values())
        assert report['passed']
    finally:
        save()


if __name__ == '__main__':
    main()
