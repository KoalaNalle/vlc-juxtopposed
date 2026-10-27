# SPDX-License-Identifier: GPL-2.0-or-later
"""Independently decode one desktop-created FLAC with the exact selected VLC.

This ignored diagnostic reads media only. It does not operate the desktop,
encode/rewrite output, change the frozen FLAC helper, or claim UI acceptance.
"""
import argparse
import array
import ctypes as C
import hashlib
import importlib.util
import json
import math
import os
from pathlib import Path
import re
import subprocess
import sys
import time
import wave

EXPECTED_FRAMES = 1_323_000
MAX_PCM_BYTES = 4 * 1024 * 1024
STAGE_SECONDS = 40
PROCESS_SECONDS = 55


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def write_json(path, value):
    path.write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8', newline='\n')


def import_bindings(path):
    spec = importlib.util.spec_from_file_location('frozen_native_flac_drain', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def decode(bindings, package, source, output):
    runtime = bindings.Runtime(package, output)
    events = {'states': [], 'reasons': [], 'callbackErrors': [], 'drains': 0}
    pcm = bytearray()

    @bindings.STOPPING
    def on_stopping(_opaque, _media, reason):
        events['reasons'].append(reason)

    @bindings.STATE
    def on_state(_opaque, state):
        events['states'].append(state)

    @bindings.AUDIO
    def on_audio(_opaque, samples, count, _pts):
        try:
            if count > 1_000_000 or len(pcm) + count * 2 > MAX_PCM_BYTES:
                events['callbackErrors'].append('Decoded PCM exceeded bounded capacity')
                return
            pcm.extend(C.string_at(samples, count * 2))
        except Exception as error:
            events['callbackErrors'].append(str(error))

    @bindings.DRAIN
    def on_drain(_opaque):
        events['drains'] += 1

    callbacks = bindings.PlayerCallbacks()
    callbacks.on_media_stopping = C.cast(on_stopping, C.c_void_p).value
    callbacks.on_state_changed = C.cast(on_state, C.c_void_p).value
    log_path = output / 'complete-decode.log'
    options = ['--ignore-config', '--no-media-library', '--no-auto-preparse',
               '--no-video', '--no-spu', '--no-sub-autodetect-file',
               '--aout=amem,none', '--verbose=2', '--file-logging',
               '--logfile=' + str(log_path)]
    argv = (C.c_char_p * len(options))(*(option.encode('utf-8') for option in options))
    instance = runtime.new(len(options), argv)
    if not instance:
        raise RuntimeError('Native LibVLC initialization failed')
    player = None
    started = time.monotonic()
    try:
        player = runtime.player_new(instance, C.byref(callbacks), None)
        if not player:
            raise RuntimeError('Native independent player allocation failed')
        media = runtime.media_new(str(source).encode('utf-8'))
        if not media:
            raise RuntimeError('Native media allocation failed')
        try:
            runtime.option(media, b':codec=flac')
            runtime.audio_callbacks(player, on_audio, None, None, None, on_drain, None)
            runtime.audio_format(player, b'S16N', 44100, 1)
            runtime.set_media(player, media)
        finally:
            runtime.media_release(media)
        if runtime.play(player) != 0:
            raise RuntimeError('Native playback failed to start')
        deadline = time.monotonic() + STAGE_SECONDS
        while time.monotonic() < deadline:
            if events['reasons'] and runtime.state(player) in (4, 6):
                break
            time.sleep(0.01)
        else:
            events['callbackErrors'].append('Complete decode exceeded 40-second stage bound')
        events['finalState'] = runtime.state(player)
    finally:
        # Parent owns and bounds this entire worker, including native release.
        # All callback closures remain strongly referenced through release.
        if player:
            runtime.stop(player)
            runtime.player_release(player)
        runtime.release(instance)
    events['elapsedSeconds'] = time.monotonic() - started
    text = log_path.read_text(encoding='utf-8-sig', errors='replace') if log_path.is_file() else ''
    events['audioDecoderModules'] = re.findall(r'using audio decoder module "([^"]+)"', text)
    events['startupErrors'] = re.findall(r'unknown option|unrecognized option|could not load module|cannot load module', text, re.IGNORECASE)
    events['eos'] = bool(events['reasons'] == [1] and events.get('finalState') == 4
                         and not events['callbackErrors'] and 6 not in events['states'])
    return runtime.version, events, bytes(pcm)


def amplitudes(pcm):
    values = array.array('h')
    values.frombytes(pcm)
    return {'peakSample': max((abs(int(value)) for value in values), default=0),
            'rmsSample': math.sqrt(sum(int(value) ** 2 for value in values) / len(values)) if values else 0,
            'nonzeroSamples': sum(value != 0 for value in values)}


def worker(args):
    report = {'passed': False, 'scope': 'Independent native complete decode of actual desktop-created FLAC; no pointer/control/primary-player/Figma acceptance.'}
    try:
        with wave.open(str(args.source), 'rb') as stream:
            source_format = {'channels': stream.getnchannels(), 'widthBytes': stream.getsampwidth(),
                             'rate': stream.getframerate(), 'frames': stream.getnframes(),
                             'compression': stream.getcomptype()}
            if source_format != {'channels': 1, 'widthBytes': 2, 'rate': 44100,
                                 'frames': EXPECTED_FRAMES, 'compression': 'NONE'}:
                raise RuntimeError('Original fixture is not the exact mono16/44100/1323000 source')
            original_pcm = stream.readframes(EXPECTED_FRAMES + 1)
        if len(original_pcm) != EXPECTED_FRAMES * 2:
            raise RuntimeError('Original WAV is incomplete')
        with args.converted.open('rb') as stream:
            if stream.read(4) != b'fLaC':
                raise RuntimeError('Desktop output is not native FLAC')
        source_before, converted_before = digest(args.source), digest(args.converted)
        bindings = import_bindings(args.bindings)
        version, events, pcm = decode(bindings, args.package, args.converted, args.output_dir)
        (args.output_dir / 'complete-decoded.pcm').write_bytes(pcm)
        source_amplitude, decoded_amplitude = amplitudes(original_pcm), amplitudes(pcm)
        source_preserved = source_before == digest(args.source)
        output_preserved = converted_before == digest(args.converted)
        decoder_ok = bool(events['audioDecoderModules'] and set(events['audioDecoderModules']) == {'flac'}
                          and not events['startupErrors'])
        exact_pcm = pcm == original_pcm
        report.update(libvlcVersion=version, sourceFormat=source_format, decode=events,
                      sourceSha256=source_before, convertedSha256=converted_before,
                      sourcePcmSha256=hashlib.sha256(original_pcm).hexdigest(),
                      decodedPcmSha256=hashlib.sha256(pcm).hexdigest(),
                      decodedFrames=len(pcm) // 2, expectedFrames=EXPECTED_FRAMES,
                      decodedBytes=len(pcm), exactPcm=exact_pcm,
                      exactFinal64Samples=pcm[-128:] == original_pcm[-128:],
                      sourceAmplitude=source_amplitude, decodedAmplitude=decoded_amplitude,
                      sourceFilePreserved=source_preserved, convertedFilePreserved=output_preserved,
                      selectedFlacDecoderAudit={'passed': decoder_ok, 'resolvedPackageDll': 'plugins/codec/libflac_plugin.dll',
                          'sha256': digest(args.package / 'plugins/codec/libflac_plugin.dll'),
                          'basis': 'Actual selected decoder log under exclusive package root plus package DLL identity; no memory-map claim.'},
                      nativeLogSha256=digest(args.output_dir / 'complete-decode.log'))
        report['passed'] = bool(events['eos'] and decoder_ok and len(pcm) == EXPECTED_FRAMES * 2
                                and exact_pcm and source_preserved and output_preserved
                                and source_amplitude['nonzeroSamples'] > EXPECTED_FRAMES // 2
                                and 0 < source_amplitude['peakSample'] < 32768
                                and source_amplitude == decoded_amplitude)
    except Exception as error:
        report['error'] = str(error)
    write_json(args.output_dir / 'worker-verification.json', report)
    return 0 if report['passed'] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ['package', 'source', 'converted', 'bindings', 'output-dir']:
        parser.add_argument('--' + option, type=Path, required=True)
    parser.add_argument('--worker', action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args()
    for name in ['package', 'source', 'converted', 'bindings', 'output_dir']:
        setattr(args, name, getattr(args, name).resolve())
    if os.name != 'nt' or C.sizeof(C.c_void_p) != 8:
        raise SystemExit('Use Windows x64 Python with this current LibVLC4 package')
    if args.worker:
        return worker(args)
    for path in [args.source, args.converted, args.bindings, args.package / 'libvlc.dll',
                 args.package / 'libvlccore.dll', args.package / 'plugins/codec/libflac_plugin.dll']:
        if not path.is_file():
            raise SystemExit('Missing exact diagnostic input: ' + str(path))
    args.output_dir.mkdir(parents=True, exist_ok=False)
    userdata = args.output_dir / 'userdata'
    userdata.mkdir()
    environment = os.environ.copy()
    environment.pop('VLC_PLUGIN_PATH', None)
    environment.update(PATH=str(args.package) + os.pathsep + environment.get('PATH', ''),
                       VLC_LIB_PATH=str(args.package / 'plugins'),
                       VLC_LIBEXEC_PATH=str(args.package / 'vlc-preparser.exe'),
                       VLC_USERDATA_PATH=str(userdata))
    report = {'passed': False, 'timedOut': False, 'processTimeoutSeconds': PROCESS_SECONDS,
              'stageTimeoutSeconds': STAGE_SECONDS, 'maxDecodedPcmBytes': MAX_PCM_BYTES,
              'checkerSha256': digest(Path(__file__)), 'frozenBindingsSha256': digest(args.bindings),
              'sourceSha256': digest(args.source), 'convertedSha256': digest(args.converted),
              'selectedPackage': str(args.package), 'source': str(args.source), 'converted': str(args.converted),
              'isolatedUserdata': True,
              'exclusivePluginRoot': str(args.package / 'plugins'),
              'runtimeSha256': {name: digest(args.package / name) for name in ['libvlc.dll', 'libvlccore.dll']},
              'scope': 'Read-only independent native complete output check; actual desktop UI/control/primary state and broader conversion/Figma gates remain separate.'}
    command = [sys.executable, str(Path(__file__).resolve()), '--worker']
    for name in ['package', 'source', 'converted', 'bindings', 'output_dir']:
        command.extend(['--' + name.replace('_', '-'), str(getattr(args, name))])
    try:
        process = subprocess.run(command, cwd=args.output_dir, env=environment, capture_output=True,
                                 timeout=PROCESS_SECONDS, creationflags=subprocess.CREATE_NO_WINDOW)
        (args.output_dir / 'stdout.txt').write_bytes(process.stdout)
        (args.output_dir / 'stderr.txt').write_bytes(process.stderr)
        report['exitCode'] = process.returncode
        worker_path = args.output_dir / 'worker-verification.json'
        report['worker'] = json.loads(worker_path.read_text(encoding='utf-8')) if worker_path.is_file() else None
        report['passed'] = bool(process.returncode == 0 and report['worker'] and report['worker']['passed'])
    except subprocess.TimeoutExpired as error:
        (args.output_dir / 'stdout.txt').write_bytes(error.stdout or b'')
        (args.output_dir / 'stderr.txt').write_bytes(error.stderr or b'')
        report.update(timedOut=True, exitCode=None)
    report['evidenceSha256'] = {name: digest(args.output_dir / name) if (args.output_dir / name).is_file() else None
                              for name in ['worker-verification.json', 'complete-decode.log', 'complete-decoded.pcm', 'stdout.txt', 'stderr.txt']}
    write_json(args.output_dir / 'verification.json', report)
    print(json.dumps(report, indent=2))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
