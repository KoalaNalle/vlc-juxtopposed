"""Force the installed built-in FLAC encoder, then fully decode real outputs.

Run unchanged against the pre-fix and repaired packages in separate fresh output
directories. A pre-fix failure remains a failure, with preserved native evidence.
No mocked encoder, primary app player, shell launcher or native build is used.
"""
import argparse
import array
import ctypes as C
import hashlib
import json
import math
import os
from pathlib import Path
import re
import subprocess
import sys
import time
import wave


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def write_json(path, value):
    path.write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8', newline='\n')


def local_argument(text):
    # Reject remote/device/mixed prefixes before normalizing or touching files.
    if len(text) >= 2 and all(c in '/\\' for c in text[:2]):
        raise ValueError('Use an absolute local path: ' + text)
    if not re.match(r'^[A-Za-z]:[/\\]', text):
        raise ValueError('Use an absolute Windows drive-rooted path: ' + text)
    return Path(text).resolve()


def fixture(path, frames, rate=44100):
    pcm = array.array('h', (round(13000 * math.sin(2 * math.pi * 440 * i / rate))
                           for i in range(frames)))
    # A deliberately nonzero final sample exposes missing final frames.
    pcm[-1] = 13000
    with wave.open(str(path), 'wb') as stream:
        stream.setnchannels(1)
        stream.setsampwidth(2)
        stream.setframerate(rate)
        stream.writeframes(pcm.tobytes())
    return pcm.tobytes()


# Exact version-0 callback layout in include/vlc/libvlc_media_player.h.
# Null optional entries remain real pointer-sized fields, not a partial struct.
CALLBACK_FIELDS = [
    'on_media_changed', 'on_media_stopping', 'on_state_changed',
    'on_buffering_changed', 'on_rate_changed', 'on_capabilities_changed',
    'on_position_changed', 'on_length_changed', 'on_track_list_changed',
    'on_track_selection_changed', 'on_program_list_changed',
    'on_program_selection_changed', 'on_titles_changed',
    'on_title_selection_changed', 'on_chapter_selection_changed',
    'on_recording_changed', 'on_screenshot_taken', 'on_media_parsed',
    'on_media_meta_changed', 'on_media_subitems_changed',
    'on_media_attachments_added', 'on_next_frame_status', 'on_prev_frame_status',
    'on_vout_changed', 'on_cork_changed', 'on_audio_volume_changed',
    'on_audio_mute_changed', 'on_audio_device_changed',
]


class PlayerCallbacks(C.Structure):
    _fields_ = [('version', C.c_uint32)] + [(name, C.c_void_p) for name in CALLBACK_FIELDS]


STOPPING = C.CFUNCTYPE(None, C.c_void_p, C.c_void_p, C.c_int)
STATE = C.CFUNCTYPE(None, C.c_void_p, C.c_int)
AUDIO = C.CFUNCTYPE(None, C.c_void_p, C.c_void_p, C.c_uint, C.c_int64)
DRAIN = C.CFUNCTYPE(None, C.c_void_p)


class Runtime:
    def __init__(self, package, output):
        self.package, self.output = package, output
        self.dll_directory = os.add_dll_directory(str(package))
        self.dll = C.CDLL(str(package / 'libvlc.dll'))
        self.deadline = time.monotonic() + 90

        def bind(name, result, *arguments):
            fn = getattr(self.dll, name)
            fn.restype, fn.argtypes = result, arguments
            return fn

        pointer = C.c_void_p
        self.version = bind('libvlc_get_version', C.c_char_p)().decode('utf-8')
        if not self.version.startswith('4.') or C.sizeof(pointer) != 8:
            raise RuntimeError('This source requires the current x64 LibVLC 4 callback ABI')
        self.new = bind('libvlc_new', pointer, C.c_int, C.POINTER(C.c_char_p))
        self.release = bind('libvlc_release', None, pointer)
        self.media_new = bind('libvlc_media_new_path', pointer, C.c_char_p)
        self.media_release = bind('libvlc_media_release', None, pointer)
        self.option = bind('libvlc_media_add_option', None, pointer, C.c_char_p)
        self.player_new = bind('libvlc_media_player_new', pointer, pointer, pointer, pointer)
        self.player_release = bind('libvlc_media_player_release', None, pointer)
        self.set_media = bind('libvlc_media_player_set_media', None, pointer, pointer)
        self.play = bind('libvlc_media_player_play', C.c_int, pointer)
        self.stop = bind('libvlc_media_player_stop_async', C.c_int, pointer)
        self.state = bind('libvlc_media_player_get_state', C.c_int, pointer)
        self.audio_callbacks = bind('libvlc_audio_set_callbacks', None,
                                    pointer, AUDIO, pointer, pointer, pointer, DRAIN, pointer)
        self.audio_format = bind('libvlc_audio_set_format', None,
                                 pointer, C.c_char_p, C.c_uint, C.c_uint)

    def stage(self, source, name, destination=None):
        log_path = self.output / (name + '.log')
        events = {'states': [], 'reasons': [], 'callbackErrors': [], 'drains': 0}
        pcm = bytearray()

        @STOPPING
        def on_stopping(_opaque, _media, reason):
            events['reasons'].append(reason)  # error=0, EOS=1, user=2

        @STATE
        def on_state(_opaque, state):
            events['states'].append(state)

        @AUDIO
        def on_audio(_opaque, samples, count, _pts):
            try:
                if count > 1_000_000 or len(pcm) + count * 2 > 4 * 1024 * 1024:
                    events['callbackErrors'].append('Decoded PCM exceeded bounded capacity')
                    return
                pcm.extend(C.string_at(samples, count * 2))
            except Exception as error:
                events['callbackErrors'].append(str(error))

        @DRAIN
        def on_drain(_opaque):
            events['drains'] += 1

        callbacks = PlayerCallbacks()
        callbacks.on_media_stopping = C.cast(on_stopping, C.c_void_p).value
        callbacks.on_state_changed = C.cast(on_state, C.c_void_p).value
        options = ['--ignore-config', '--no-media-library', '--no-video', '--no-spu',
                   '--no-sub-autodetect-file', '--aout=amem,none', '--verbose=2',
                   '--file-logging', '--logfile=' + str(log_path)]
        argv = (C.c_char_p * len(options))(*(s.encode('utf-8') for s in options))
        instance = self.new(len(options), argv)
        if not instance:
            raise RuntimeError('LibVLC initialization failed')
        player = None
        try:
            player = self.player_new(instance, C.byref(callbacks), None)
            if not player:
                raise RuntimeError('Native player allocation failed')
            media = self.media_new(str(source).encode('utf-8'))
            if not media:
                raise RuntimeError('Native media allocation failed')
            try:
                if destination is not None:
                    # Equivalent to config_StringEscape, with forward slashes.
                    dst = destination.as_posix().replace('\\', '\\\\').replace("'", "\\'")
                    chain = ("#transcode{acodec=flac,aenc=flac,channels=1,samplerate=44100}"
                             ":std{access=file,mux=raw,dst='" + dst + "'}")
                    self.option(media, (':sout=' + chain).encode('utf-8'))
                    for option in [':no-sout-keep', ':sout-audio', ':no-sout-video',
                                   ':no-sout-spu', ':no-sout-all', ':sout-transcode-afilter=',
                                   ':sout-transcode-aenc=flac', ':rate=1',
                                   ':start-time=0', ':stop-time=0', ':run-time=0']:
                        self.option(media, option.encode('utf-8'))
                else:
                    self.option(media, b':codec=flac')
                    self.audio_callbacks(player, on_audio, None, None, None, on_drain, None)
                    self.audio_format(player, b'S16N', 44100, 1)
                self.set_media(player, media)
            finally:
                self.media_release(media)
            if self.play(player) != 0:
                raise RuntimeError('Native playback start failed')
            deadline = min(time.monotonic() + 15, self.deadline)
            while time.monotonic() < deadline:
                if events['reasons'] and self.state(player) in (4, 6):
                    break
                time.sleep(0.01)
            else:
                events['callbackErrors'].append('Native stage exceeded deadline')
            events['finalState'] = self.state(player)
        finally:
            # The parent caps the entire owned process, including blocking release.
            # Callback objects and context stay alive through native player release.
            if player:
                self.stop(player)
                self.player_release(player)
            self.release(instance)
        text = log_path.read_text(encoding='utf-8', errors='replace') if log_path.exists() else ''
        events['audioEncoderModules'] = re.findall(r'using audio encoder module "([^"]+)"', text)
        events['audioDecoderModules'] = re.findall(r'using audio decoder module "([^"]+)"', text)
        events['eos'] = (events['reasons'] == [1] and events.get('finalState') == 4
                         and not events['callbackErrors'] and 6 not in events['states'])
        return events, bytes(pcm)


def streaminfo(path):
    with path.open('rb') as stream:
        header = stream.read(42)
    if len(header) != 42 or header[:4] != b'fLaC' or header[4] != 0x80 or header[5:8] != b'\0\0\x22':
        raise ValueError('Initial header must be magic + last STREAMINFO header + all 34 payload bytes')
    payload = header[8:]
    packed = int.from_bytes(payload[10:18], 'big')
    return {'hex': header.hex(), 'minBlockSize': int.from_bytes(payload[:2], 'big'),
            'maxBlockSize': int.from_bytes(payload[2:4], 'big'), 'rate': packed >> 44,
            'channels': ((packed >> 41) & 7) + 1, 'bits': ((packed >> 36) & 31) + 1,
            'totalSamples': packed & ((1 << 36) - 1), 'md5': payload[18:].hex(),
            'initialUnknownTotalsAndMd5': packed & ((1 << 36) - 1) == 0 and payload[18:] == bytes(16)}


def worker(package, output):
    runtime = Runtime(package, output)
    report = {'passed': False, 'libvlcVersion': runtime.version, 'cases': [],
              'evidence': 'Actual forced built-in FLAC encode and complete built-in decode; no native unit/allocation fault or GUI claim.'}

    def run_case(name, frames, rate=44100):
        source, destination = output / (name + '.wav'), output / (name + '.flac')
        original = fixture(source, frames, rate)
        case = {'name': name, 'sourceFrames': frames, 'sourceRate': rate, 'passed': False}
        report['cases'].append(case)
        try:
            case['encode'], _ = runtime.stage(source, name + '-encode', destination)
            case['header'] = streaminfo(destination)
            case['decode'], pcm = runtime.stage(destination, name + '-decode')
            (output / (name + '.pcm')).write_bytes(pcm)
            decoded = array.array('h')
            decoded.frombytes(pcm)
            case['decodedFrames'] = len(decoded)
            case['peak'] = max((abs(int(value)) / 32768 for value in decoded), default=0)
            case['sourceSha256'], case['flacSha256'] = digest(source), digest(destination)
            case['exactPcm'] = pcm == original if rate == 44100 else None
            expected = frames if rate == 44100 else round(frames * 44100 / rate)
            count_ok = len(decoded) == expected if rate == 44100 else abs(len(decoded) - expected) <= 100
            case['expectedFrames'] = expected
            case['passed'] = (case['encode']['eos'] and case['decode']['eos']
                and case['encode']['audioEncoderModules'] and
                set(case['encode']['audioEncoderModules']) == {'flac'}
                and case['decode']['audioDecoderModules'] and
                set(case['decode']['audioDecoderModules']) == {'flac'}
                and case['header']['rate'] == 44100 and case['header']['channels'] == 1
                and case['header']['bits'] == 16 and case['header']['initialUnknownTotalsAndMd5']
                and count_ok and .3 < case['peak'] < .5
                and (case['exactPcm'] if rate == 44100 else any(decoded[-64:])))
            case['passed'] = bool(case['passed'])
        except Exception as error:
            case['error'] = str(error)
        write_json(output / 'verification.json', report)
        return case

    first = run_case('strict-88200', 88200)
    block = first.get('header', {}).get('maxBlockSize', 0)
    if not 16 <= block <= 65535:
        report['blockSizeError'] = 'Actual native STREAMINFO did not establish a bounded block size'
    else:
        report['actualBlockSize'] = block
        boundaries = [('one', 1), ('seventeen', 17), ('B-minus-one', block - 1),
                      ('B', block), ('B-plus-one', block + 1), ('two-B-plus-seventeen', 2 * block + 17)]
        for label, frames in boundaries:
            if time.monotonic() >= runtime.deadline:
                report['deadlineError'] = 'Worker exhausted its finite deadline'
                break
            run_case('boundary-' + label, frames)
        run_case('repeat-88200', 88200)
        run_case('resample-8000-to-44100', 16000, 8000)
    report['passed'] = bool(len(report['cases']) == 9 and all(case['passed'] for case in report['cases']))
    write_json(output / 'verification.json', report)
    return 0 if report['passed'] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package', required=True)
    parser.add_argument('--output-dir', required=True)
    parser.add_argument('--worker', action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args()
    if os.name != 'nt':
        raise SystemExit('Run this diagnostic with Windows x64 Python and the actual Windows package')
    package, output = local_argument(args.package), local_argument(args.output_dir)
    if args.worker:
        return worker(package, output)
    required = [package / 'libvlc.dll', package / 'libvlccore.dll',
                package / 'plugins/codec/libflac_plugin.dll']
    if not all(path.is_file() for path in required):
        raise SystemExit('Missing the actual package runtime or built-in FLAC plugin')
    output.mkdir(parents=True, exist_ok=False)
    userdata = output / 'userdata'
    userdata.mkdir()
    environment = os.environ.copy()
    environment.update(PATH=str(package) + os.pathsep + environment.get('PATH', ''),
                       VLC_PLUGIN_PATH=str(package / 'plugins'), VLC_USERDATA_PATH=str(userdata),
                       VLC_LIBEXEC_PATH=str(package / 'vlc-preparser.exe'))
    command = [sys.executable, str(Path(__file__).resolve()), '--worker',
               '--package', str(package), '--output-dir', str(output)]
    try:
        result = subprocess.run(command, cwd=output, env=environment, capture_output=True,
            timeout=100, creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
        code = result.returncode
        (output / 'stdout.txt').write_bytes(result.stdout)
        (output / 'stderr.txt').write_bytes(result.stderr)
    except subprocess.TimeoutExpired as error:
        code = 1
        (output / 'stdout.txt').write_bytes(error.stdout or b'')
        (output / 'stderr.txt').write_bytes(error.stderr or b'')
        (output / 'timeout.txt').write_text('Owned native FLAC worker exceeded 100 seconds.\n', encoding='utf-8')
    verification = output / 'verification.json'
    report = json.loads(verification.read_text(encoding='utf-8')) if verification.exists() else {'cases': []}
    report.update(exitCode=code, passed=bool(code == 0 and report.get('passed')),
                  isolatedUserdata=True, runtimeSha256={p.name: digest(p) for p in required})
    write_json(verification, report)
    print(json.dumps({'passed': report['passed'], 'exitCode': code, 'evidence': str(verification)}))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
