#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Automatic encoder preferences with real selection and complete FLAC PCM."""
import argparse, hashlib, json, re, shutil, sys
from pathlib import Path
from native_http_runtime import stage_runtime, regenerate_cache, run_owned
from native_flac_drain import fixture, Runtime


def save(path, value):
    path.write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8', newline='\n')


def worker(package, out, expected_missing, scope):
    source = out / 'source.wav'
    original = fixture(source, 88200)
    decoder = Runtime(package, out)
    report = dict(passed=False, expectedMissingPreference=expected_missing, preferenceScope=scope, cases={})
    cases = [('automatic', None, None), ('prefer-flac', 'flac', None),
             ('prefer-avcodec', 'avcodec', None), ('explicit-flac', 'avcodec', 'flac'),
             ('incompatible-preference', 'vorbis', None),
             ('missing-preference', 'owned-missing-encoder', None),
             ('none-preference', 'none', None)]
    automatic = None
    try:
        for name, preference, explicit in cases:
            case = out / name
            case.mkdir()
            target, log = case / 'out.flac', case / 'vlc.log'
            fields = 'acodec=flac,channels=1,samplerate=44100'
            if preference and scope == 'chain':
                # Stream-output resources belong to the player, not the input.
                # An old producer ignores this unknown chain option normally.
                fields += ',preferred-encoder=' + preference
            if explicit:
                fields += ',aenc=' + explicit
            chain = "#transcode{" + fields + "}:std{access=file,mux=flac,dst='" + target.as_posix() + "'}"
            command = [package / 'vlc.exe', '--intf=dummy', '--ignore-config', '--extraintf=', '--control=',
                       '--no-one-instance', '--no-media-library', '--no-crashdump', '--no-metadata-network-access',
                       '--http-proxy=direct://', '--no-ml-network-discovery', '--no-video', '--no-spu',
                       '--avcodec-threads=1', '--sout-transcode-threads=1', '--play-and-exit', '--no-sout-keep',
                       '--file-logging', '--verbose=2', '--log-verbose=2', '--logfile=' + str(log),
                       '--sout=' + chain, str(source)]
            if preference and scope == 'global':
                command.insert(1, '--sout-transcode-preferred-encoder=' + preference)
            result = run_owned(command, package, case)
            report['cases'][name] = result
            text = log.read_text(encoding='utf-8', errors='replace') if log.exists() else ''
            modules = re.findall(r'using audio encoder module "([^"]+)"', text)
            result['encoderModules'] = modules
            result['encodeEofObserved'] = 'EOF reached' in text
            assert result['normalExit'] and result['encodeEofObserved'] and target.is_file(), name
            events, pcm = decoder.stage(target, name + '-decode')
            result.update(decode=events, decodedFrames=len(pcm) // 2, exactPcm=pcm == original)
            header = target.read_bytes()[:42]
            assert len(header) == 42 and header[:4] == b'fLaC' and header[4] & 0x7f == 0 and header[5:8] == b'\0\0\x22', name
            packed = int.from_bytes(header[18:26], 'big')
            result['flacFormat'] = dict(rate=packed >> 44, channels=((packed >> 41) & 7) + 1, bits=((packed >> 36) & 31) + 1)
            if automatic is None:
                automatic = set(modules)
            wanted = {explicit or preference} if explicit or preference in ['flac', 'avcodec'] else automatic
            result['selectionMatches'] = bool(modules) and set(modules) == wanted
            result['payloadPass'] = events['eos'] and pcm == original and len(pcm) == 88200 * 2 and result['flacFormat'] == dict(rate=44100, channels=1, bits=16)
            result['passed'] = result['payloadPass'] and result['selectionMatches']
            print(name, 'selection=' + str(modules), 'frames=' + str(result['decodedFrames']), 'exact=' + str(result['exactPcm']), flush=True)
            save(out / 'verification.json', report)
            assert result['payloadPass'], name
        if expected_missing:
            report['passed'] = not report['cases']['prefer-avcodec']['selectionMatches'] and all(v['passed'] for k, v in report['cases'].items() if k != 'prefer-avcodec')
        else:
            report['passed'] = all(case['passed'] for case in report['cases'].values())
    finally:
        save(out / 'verification.json', report)
    return 0 if report['passed'] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['package', 'output-dir']:
        parser.add_argument('--' + name, type=Path, required=True)
    for name in ['core', 'transcode']:
        parser.add_argument('--' + name, type=Path)
    parser.add_argument('--expected-missing-preference', action='store_true')
    parser.add_argument('--scope', choices=['chain', 'global'], default='chain')
    parser.add_argument('--worker', action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args()
    package, out = args.package.resolve(), args.output_dir.resolve()
    if args.worker:
        return worker(package, out, args.expected_missing_preference, args.scope)
    if args.expected_missing_preference and args.scope != 'chain':
        parser.error('Use chain scope for an older producer without the preference option')
    out.mkdir(parents=True, exist_ok=False)
    runtime = out / 'runtime'
    stage_runtime(package, runtime)
    for source, name in [(args.core, 'libvlccore.dll'), (args.transcode, 'plugins/stream_out/libstream_out_transcode_plugin.dll')]:
        if source:
            target = runtime / name
            target.unlink()  # Remove only this new runtime's hard link before copying.
            shutil.copy2(source, target)
    regenerate_cache(runtime, out / 'cache')
    command = [sys.executable, Path(__file__).resolve(), '--worker', '--package', runtime, '--output-dir', out, '--scope', args.scope]
    if args.expected_missing_preference:
        command.append('--expected-missing-preference')
    process = run_owned(command, runtime, out / 'worker', timeout=100)
    report = json.loads((out / 'verification.json').read_text()) if (out / 'verification.json').exists() else dict(passed=False)
    report.update(workerProcess=process, runtimeSha256={name:hashlib.sha256((runtime/name).read_bytes()).hexdigest() for name in ['libvlccore.dll', 'plugins/stream_out/libstream_out_transcode_plugin.dll']})
    report['passed'] = bool(report['passed'] and process['normalExit'])
    save(out / 'verification.json', report)
    print('Encoder preference:', 'PASS' if report['passed'] else 'FAIL', flush=True)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
