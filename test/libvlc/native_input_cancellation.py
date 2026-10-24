#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Real Windows input cancellation, recovery, and genuine failure regression."""
from pathlib import Path
import argparse
import ctypes as C
import json
import os
import re
import sys
import time
import wave
import subprocess

from native_http_runtime import stage_runtime, regenerate_cache, environment
from native_mmdevice_lifecycle import Stats, digest


def write(path, value):
    path.write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8', newline='\n')


def wait(predicate, description, seconds=10):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        if predicate():
            return
        time.sleep(.005)
    raise AssertionError(description)


def child(runtime, output, fixtures, mode):
    os.environ.pop('VLC_PLUGIN_PATH', None)
    directory = os.add_dll_directory(str(runtime))
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
    player_new = bind('libvlc_media_player_new', ptr, ptr, ptr, ptr)
    player_release = bind('libvlc_media_player_release', None, ptr)
    set_media = bind('libvlc_media_player_set_media', None, ptr, ptr)
    play = bind('libvlc_media_player_play', C.c_int, ptr)
    stop = bind('libvlc_media_player_stop_async', C.c_int, ptr)
    state = bind('libvlc_media_player_get_state', C.c_int, ptr)
    clock = bind('libvlc_media_player_get_time', C.c_int64, ptr)
    seek = bind('libvlc_media_player_set_time', C.c_int, ptr, C.c_int64, C.c_bool)
    stats_get = bind('libvlc_media_get_stats', C.c_bool, ptr, C.POINTER(Stats))
    assert bind('libvlc_get_version', C.c_char_p)().startswith(b'4.')
    options = ['--ignore-config', '--no-media-library', '--no-video', '--no-spu',
        '--no-sub-autodetect-file', '--no-metadata-network-access', '--no-ml-network-discovery',
        '--aout=mmdevice,none', '--mmdevice-backend=wasapi,none', '--gain=0',
        '--file-logging', '--verbose=2', '--log-verbose=2', '--logfile=' + str(output / 'vlc.log')]
    argv = (C.c_char_p * len(options))(*(s.encode('utf-8') for s in options))
    instance = new(len(options), argv)
    assert instance
    # Version-0 public callbacks, all optional fields initialized to null.
    callback_names = ['on_media_changed', 'on_media_stopping', 'on_state_changed',
        'on_buffering_changed', 'on_rate_changed', 'on_capabilities_changed',
        'on_position_changed', 'on_length_changed', 'on_track_list_changed',
        'on_track_selection_changed', 'on_program_list_changed', 'on_program_selection_changed',
        'on_titles_changed', 'on_title_selection_changed', 'on_chapter_selection_changed',
        'on_recording_changed', 'on_screenshot_taken', 'on_media_parsed', 'on_media_meta_changed',
        'on_media_subitems_changed', 'on_media_attachments_added', 'on_next_frame_status',
        'on_prev_frame_status', 'on_vout_changed', 'on_cork_changed', 'on_audio_volume_changed',
        'on_audio_mute_changed', 'on_audio_device_changed']
    header = (Path(__file__).resolve().parents[2] / 'include/vlc/libvlc_media_player.h').read_text(encoding='utf-8')
    declaration = re.search(r'struct libvlc_media_player_cbs\s*\{(.*?)\n\};', header, re.S).group(1)
    assert re.findall(r'\(\*(on_\w+)\)', declaration) == callback_names
    class Callbacks(C.Structure):
        _fields_ = [('version', C.c_uint32)] + [(name, ptr) for name in callback_names]
    events = []
    @C.CFUNCTYPE(None, ptr, C.c_int)
    def state_changed(opaque, value):
        events.append(int(value))
    callbacks = Callbacks()
    callbacks.on_state_changed = C.cast(state_changed, ptr).value
    player = player_new(instance, C.byref(callbacks), None)
    assert player
    media = None
    report = dict(passed=False, mode=mode, cancellations=[], samples=[], stateEvents=events)

    def save():
        write(output / 'observations.json', report)

    def select(path):
        nonlocal media
        if media:
            media_release(media)
        media = media_new(str(path).encode('utf-8'))
        assert media
        set_media(player, media)
        assert play(player) == 0

    def stop_and_wait():
        start = len(events)
        result = stop(player)
        if result == 0:
            wait(lambda: 4 in events[start:], 'new native stop callback acknowledges this request')
        return result

    def sample():
        stats = Stats()
        valid = stats_get(media, C.byref(stats))
        return dict(state=state(player), timeUs=clock(player), statsValid=bool(valid),
            readBytes=stats.read, demuxBytes=stats.demux_read, decoded=stats.decoded_audio,
            played=stats.played_audio, lost=stats.lost_audio)

    try:
        if mode == 'rapid-recovery':
            for repetition in range(3):
                for delay in [0, .001, .004, .010, .025, .060]:
                    start = len(events)
                    select(fixtures / 'valid.wav')
                    if delay:
                        time.sleep(delay)
                    before = sample()
                    assert stop_and_wait() == 0
                    report['cancellations'].append(dict(repetition=repetition,
                        delaySeconds=delay, beforeStop=before, finalState=state(player),
                        stateEvents=events[start:].copy()))
                    save()
                    assert 6 not in events[start:], 'valid media cancellation must not report an Error event'

        if mode in ['empty', 'invalid']:
            start = len(events)
            select(fixtures / (mode + '.wav'))
            wait(lambda: 6 in events[start:], 'genuine invalid media reports Error callback')
            report.update(errorStateObserved=True, errorSample=sample(), passed=True)
        else:
            select(fixtures / ('misnamed.mp3' if mode == 'misnamed' else 'valid.wav'))
            wait(lambda: state(player) == 2 and sample()['played'] > 0 and clock(player) > 250_000,
                 'fresh input reaches real decoded/played audio')
            for _ in range(12):
                report['samples'].append(sample())
                save()
                time.sleep(.1)
            assert seek(player, 5_000_000, False) == 0
            wait(lambda: state(player) == 2 and clock(player) >= 4_900_000,
                 'fresh input can seek after cancellation')
            report['afterSeek'] = sample()
            wait(lambda: clock(player) > 5_200_000 and sample()['played'] > report['afterSeek']['played'],
                 'decoded playback continues beyond the seek destination')
            report['afterSeekProgress'] = sample()
            assert stop_and_wait() == 0
            report['explicitStopAcknowledged'] = True
            samples = report['samples']
            assert all(s['state'] == 2 and s['statsValid'] and s['decoded'] > 0
                       and s['played'] > 0 for s in samples)
            assert all(b['timeUs'] > a['timeUs'] and b['played'] >= a['played']
                       for a, b in zip(samples, samples[1:]))
            report['passed'] = True
    except Exception as error:
        report.update(passed=False, error=str(error))
    finally:
        stop_and_wait()
        player_release(player)
        if media:
            media_release(media)
        release(instance)
        directory.close()
        save()
    return 0 if report['passed'] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package', type=Path, required=True)
    parser.add_argument('--output-dir', type=Path, required=True)
    parser.add_argument('--source-inputs', type=Path)
    parser.add_argument('--child', choices=['rapid-recovery', 'empty', 'invalid', 'misnamed'])
    args = parser.parse_args()
    output = args.output_dir.resolve()
    if args.child:
        return child(args.package.resolve(), output, output.parent / 'fixtures', args.child)

    repo = Path(__file__).resolve().parents[2]
    assert output.is_relative_to(repo / 'build-windows'), 'Use an owned output under build-windows'
    inputs = json.loads(args.source_inputs.read_text(encoding='utf-8')) if args.source_inputs else {}
    assert all(digest(repo / p) == h for p, h in inputs.items())
    output.mkdir(parents=True)
    fixtures = output / 'fixtures'
    fixtures.mkdir()
    with wave.open(str(fixtures / 'valid.wav'), 'wb') as wav:
        wav.setnchannels(2)
        wav.setsampwidth(2)
        wav.setframerate(48000)
        wav.writeframes(b'\0' * 4 * 48000 * 20)
    (fixtures / 'misnamed.mp3').write_bytes((fixtures / 'valid.wav').read_bytes())
    (fixtures / 'empty.wav').write_bytes(b'')
    (fixtures / 'invalid.wav').write_bytes(b'Owned deliberately invalid media\n' * 2048)
    fixture_hashes = {p.name: digest(p) for p in fixtures.iterdir()}
    runtime = output / 'runtime'
    stage_runtime(args.package, runtime)
    regenerate_cache(runtime, output / 'cache')
    identities = {p: digest(runtime / p) for p in ['libvlccore.dll',
        'plugins/stream_filter/libcache_read_plugin.dll']}
    report = dict(passed=False, fullBetaAccepted=False, consoleOnly=True,
        scope='Real native Windows PCM playback/cancellation and failure behavior; no GUI or audible-quality acceptance.',
        runtimeIdentities=identities, fixtureHashes=fixture_hashes,
        runnerSha256=digest(Path(__file__)), cases={})
    for mode in ['rapid-recovery', 'empty', 'invalid', 'misnamed']:
        case = output / mode
        case.mkdir()
        # Remove the duplicate plugin path before Python/its C runtime starts.
        env = environment(runtime, case)
        env.pop('VLC_PLUGIN_PATH', None)
        command = [sys.executable, '-X', 'utf8', str(Path(__file__).resolve()),
            '--package', str(runtime), '--output-dir', str(case), '--child', mode]
        result = dict(forcedCleanupUsed=False)
        with (case / 'stdout.txt').open('wb') as stdout, (case / 'stderr.txt').open('wb') as stderr:
            process = subprocess.Popen(command, cwd=runtime, env=env, stdout=stdout,
                stderr=stderr, creationflags=subprocess.CREATE_NO_WINDOW)
            result['pid'] = process.pid
            try:
                result['exitCode'] = process.wait(timeout=120)
            except subprocess.TimeoutExpired:
                result['forcedCleanupUsed'] = True
                process.kill()
                result['exitCode'] = process.wait(timeout=10)
        result['normalExit'] = result['exitCode'] == 0 and not result['forcedCleanupUsed']
        if (case / 'observations.json').is_file():
            result['observations'] = json.loads((case / 'observations.json').read_text(encoding='utf-8'))
        log = (case / 'vlc.log').read_text(encoding='utf-8', errors='replace') if (case / 'vlc.log').exists() else ''
        stderr = (case / 'stderr.txt').read_text(encoding='utf-8', errors='replace')
        result['nativeErrors'] = [s for s in (log + '\n' + stderr).splitlines() if re.search(r'\berror:|Assertion failed', s)]
        result['runtimeFailureSeen'] = bool(re.search(r'Assertion failed|Traceback|Exception ignored on calling ctypes callback', log + '\n' + stderr))
        result['nativeWarnings'] = [s for s in log.splitlines() if 'warning:' in s]
        result['prebufferBytes'] = [int(n) for n in re.findall(r'pre-buffering done (\d+) bytes', log)]
        result['wasapiOpened'] = 'using aout stream module "wasapi"' in log
        result['wavDemuxSelected'] = 'using demux module "wav"' in log
        result['pluginCacheLoads'] = log.count('loading plugins cache file ')
        result['loadedModules'] = re.findall(r'plug-ins loaded: (\d+) modules', log)
        observation = result.get('observations', {})
        expected_failure = mode in ['empty', 'invalid']
        result['passed'] = bool(result['normalExit'] and not result['runtimeFailureSeen'] and observation.get('passed') and result['pluginCacheLoads'] == 1
            and (observation.get('errorStateObserved') and result['nativeErrors'] if expected_failure
                 else not result['nativeErrors'] and result['wasapiOpened'] and result['wavDemuxSelected']))
        if mode == 'empty':
            result['passed'] &= any('cannot pre fill buffer' in s for s in result['nativeErrors'])
        if mode == 'rapid-recovery':
            result['passed'] &= len(observation.get('cancellations', [])) == 18
            result['passed'] &= 0 in result['prebufferBytes'] and any(n > 0 for n in result['prebufferBytes'])
        report['cases'][mode] = result
        write(output / 'verification.json', report)
        print(mode, 'passed', result['passed'], 'normalExit', result['normalExit'],
              'nativeErrors', len(result['nativeErrors']), flush=True)
    report.update(runtimeIdentitiesUnchanged=all(digest(runtime / p) == h for p, h in identities.items()),
        fixtureHashesUnchanged=all(digest(fixtures / p) == h for p, h in fixture_hashes.items()),
        sourceInputsUnchanged=all(digest(repo / p) == h for p, h in inputs.items()))
    report['passed'] = bool(all(c['passed'] for c in report['cases'].values())
        and report['runtimeIdentitiesUnchanged'] and report['fixtureHashesUnchanged'] and report['sourceInputsUnchanged'])
    write(output / 'verification.json', report)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
