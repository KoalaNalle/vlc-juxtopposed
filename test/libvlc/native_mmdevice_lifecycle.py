#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Exercise repeated real WASAPI playback, device reselection and stop."""
from pathlib import Path
import argparse
import ctypes as C
import hashlib
import json
import os
import sys
import time
import wave

from native_http_runtime import stage_runtime, regenerate_cache, run_owned


class Stats(C.Structure):
    _fields_ = [('read', C.c_uint64), ('input_bitrate', C.c_float),
        ('demux_read', C.c_uint64), ('demux_bitrate', C.c_float),
        ('corrupted', C.c_uint64), ('discontinuity', C.c_uint64),
        ('decoded_video', C.c_uint64), ('decoded_audio', C.c_uint64),
        ('displayed', C.c_uint64), ('late', C.c_uint64), ('lost', C.c_uint64),
        ('played_audio', C.c_uint64), ('lost_audio', C.c_uint64)]


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def write(path, value):
    path.write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8')


def wait(predicate, message):
    deadline = time.monotonic() + 8
    while time.monotonic() < deadline:
        if predicate():
            return
        time.sleep(0.01)
    raise AssertionError(message)


def child(runtime, output):
    dll_dir = os.add_dll_directory(str(runtime))
    dll = C.CDLL(str(runtime / 'libvlc.dll'))

    def bind(name, result, *arguments):
        fn = getattr(dll, name)
        fn.restype, fn.argtypes = result, arguments
        return fn

    ptr = C.c_void_p
    new = bind('libvlc_new', ptr, C.c_int, C.POINTER(C.c_char_p))
    release = bind('libvlc_release', None, ptr)
    media_new = bind('libvlc_media_new_path', ptr, C.c_char_p)
    media_release = bind('libvlc_media_release', None, ptr)
    stats_get = bind('libvlc_media_get_stats', C.c_bool, ptr, C.POINTER(Stats))
    player_new = bind('libvlc_media_player_new', ptr, ptr, ptr, ptr)
    player_release = bind('libvlc_media_player_release', None, ptr)
    set_media = bind('libvlc_media_player_set_media', None, ptr, ptr)
    play = bind('libvlc_media_player_play', C.c_int, ptr)
    stop = bind('libvlc_media_player_stop_async', C.c_int, ptr)
    state = bind('libvlc_media_player_get_state', C.c_int, ptr)
    device_set = bind('libvlc_audio_output_device_set', C.c_int, ptr, C.c_char_p)
    version = bind('libvlc_get_version', C.c_char_p)().decode()
    assert version.startswith('4.') and C.sizeof(ptr) == 8
    log = output / 'vlc.log'
    options = ['--ignore-config', '--no-media-library', '--no-video', '--no-spu',
        '--no-metadata-network-access', '--no-ml-network-discovery',
        '--aout=mmdevice,none', '--mmdevice-backend=wasapi,none', '--gain=0',
        '--file-logging', '--verbose=2', '--log-verbose=2', '--logfile=' + str(log)]
    argv = (C.c_char_p * len(options))(*(s.encode('utf-8') for s in options))
    instance = new(len(options), argv)
    assert instance
    player = player_new(instance, None, None)
    assert player
    report = {'passed': False, 'libvlcVersion': version, 'cases': []}
    write(output / 'lifecycle.json', report)
    try:
        for cycle in range(16):
            media = media_new(str(output / 'silence.wav').encode('utf-8'))
            assert media
            try:
                set_media(player, media)
                assert play(player) == 0
                stats = Stats()

                def audio_started():
                    assert state(player) != 6, 'Native player entered error state'
                    return (stats_get(media, C.byref(stats)) and
                            stats.decoded_audio > 0 and stats.played_audio > 0)

                wait(audio_started, 'No decoded/played audio before deadline')
                case = {'cycle': cycle, 'mode': 'device-reselect' if cycle % 2 else 'stop',
                        'decodedAudio': stats.decoded_audio, 'playedAudio': stats.played_audio}
                if cycle % 2:
                    # Per-player selection only; MMDevice falls back to the default
                    # when this deliberately nonexistent endpoint is requested.
                    case['deviceSelectionResult'] = device_set(
                        player, b'__vlc_owned_missing_device__')
                    assert case['deviceSelectionResult'] == 0
                assert stop(player) == 0
                wait(lambda: state(player) == 4, 'Native stop did not acknowledge')
                case.update(stopped=True, passed=True)
                report['cases'].append(case)
                write(output / 'lifecycle.json', report)
            finally:
                media_release(media)
    finally:
        stop(player)
        player_release(player)
        release(instance)
        dll_dir.close()
    text = log.read_text(encoding='utf-8', errors='replace')
    report.update(wasapiLoads=text.count('using aout stream module "wasapi"'),
        missingDeviceFallbacks=text.count('cannot get selected device __vlc_owned_missing_device__'),
        assertionSeen='Assertion failed' in text)
    report['passed'] = (len(report['cases']) == 16 and report['wasapiLoads'] >= 16 and
                        report['missingDeviceFallbacks'] == 8 and not report['assertionSeen'])
    write(output / 'lifecycle.json', report)
    assert report['passed'], report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--child-runtime', type=Path, help=argparse.SUPPRESS)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    owned = root / 'build-windows'
    output = args.output.resolve()
    assert sys.platform == 'win32' and output.is_relative_to(owned)
    if args.child_runtime:
        runtime = args.child_runtime.resolve()
        assert runtime.is_relative_to(owned)
        child(runtime, output)
        return
    package = args.package.resolve()
    assert package.is_relative_to(owned) and not output.exists()
    output.mkdir(parents=True)
    names = ['libvlccore.dll', 'plugins/audio_output/libmmdevice_plugin.dll',
             'plugins/audio_output/libwasapi_plugin.dll']
    before = {name: digest(package / name) for name in names}
    runtime = output / 'runtime'
    stage_runtime(package, runtime)
    regenerate_cache(runtime, output / 'cache')
    run = output / 'run'
    run.mkdir()
    with wave.open(str(run / 'silence.wav'), 'wb') as stream:
        stream.setnchannels(2)
        stream.setsampwidth(2)
        stream.setframerate(48000)
        stream.writeframes(b'\0' * (48000 * 4 * 12))
    result = run_owned([sys.executable, '-X', 'utf8', Path(__file__).resolve(),
        '--child-runtime', runtime, '--output', run], runtime, run, timeout=90)
    result['lifecycle'] = json.loads((run / 'lifecycle.json').read_text(encoding='utf-8'))
    result.update(runtimeSha256=before, sourceRuntimeUnchanged=all(
        digest(package / name) == value for name, value in before.items()))
    result['passed'] = (result['normalExit'] and result['lifecycle']['passed'] and
                        result['sourceRuntimeUnchanged'])
    write(output / 'result.json', result)
    print(json.dumps({k: v for k, v in result.items() if k != 'lifecycle'}), flush=True)
    assert result['passed'], 'See result.json and run/stderr.txt'


if __name__ == '__main__':
    main()
