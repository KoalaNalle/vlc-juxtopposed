"""Opt-in LibVLC 4 regression for delayed captions and paused timing changes."""
from pathlib import Path
from hashlib import sha256
import argparse
import ctypes as C
import json
import os
import re
import time
import traceback

from PySide6.QtGui import QImage


parser = argparse.ArgumentParser()
parser.add_argument('--fixture-root', type=Path, required=True)
parser.add_argument('--runtime', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--media', choices=['frame-marker.mkv', 'frame-marker-ass.mkv',
                                      'frame-marker.mp4'], default='frame-marker-ass.mkv')
parser.add_argument('--audio', choices=['wasapi', 'none'], default='wasapi')
parser.add_argument('--delay-ms', type=int, choices=[-1200, 0, 1200], default=1200)
args = parser.parse_args()
runtime = args.runtime.resolve()
out = args.output.resolve()
media_path = args.fixture_root.resolve() / args.media
assert out.is_dir() and not any(out.iterdir())
assert media_path.is_file()


def digest(path):
    return sha256(path.read_bytes()).hexdigest()


def write():
    (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n',
                                   encoding='utf-8', newline='\n')


def wait(predicate, timeout):
    end = time.monotonic() + timeout
    while time.monotonic() < end:
        if predicate():
            return True
        time.sleep(.02)
    return bool(predicate())


dll_directory = os.add_dll_directory(str(runtime))
os.environ['VLC_PLUGIN_PATH'] = str(runtime / 'plugins')
os.environ['VLC_LIBEXEC_PATH'] = str(runtime / 'vlc-preparser.exe')
vlc = C.CDLL(str(runtime / 'libvlc.dll'))


def bind(name, restype, *argtypes):
    function = getattr(vlc, name)
    function.restype = restype
    function.argtypes = argtypes
    return function


new = bind('libvlc_new', C.c_void_p, C.c_int, C.POINTER(C.c_char_p))
release = bind('libvlc_release', None, C.c_void_p)
media_new = bind('libvlc_media_new_path', C.c_void_p, C.c_char_p)
media_release = bind('libvlc_media_release', None, C.c_void_p)
media_option = bind('libvlc_media_add_option', None, C.c_void_p, C.c_char_p)
player_new = bind('libvlc_media_player_new', C.c_void_p, C.c_void_p, C.c_void_p, C.c_void_p)
player_release = bind('libvlc_media_player_release', None, C.c_void_p)
set_media = bind('libvlc_media_player_set_media', None, C.c_void_p, C.c_void_p)
play = bind('libvlc_media_player_play', C.c_int, C.c_void_p)
pause = bind('libvlc_media_player_set_pause', None, C.c_void_p, C.c_int)
stop = bind('libvlc_media_player_stop_async', C.c_int, C.c_void_p)
state = bind('libvlc_media_player_get_state', C.c_int, C.c_void_p)
get_time = bind('libvlc_media_player_get_time', C.c_int64, C.c_void_p)
seek = bind('libvlc_media_player_set_time', C.c_int, C.c_void_p, C.c_int64, C.c_bool)
vouts = bind('libvlc_media_player_has_vout', C.c_uint, C.c_void_p)
snapshot = bind('libvlc_video_take_snapshot', C.c_int, C.c_void_p, C.c_uint,
                C.c_char_p, C.c_uint, C.c_uint)
get_delay = bind('libvlc_video_get_spu_delay', C.c_int64, C.c_void_p)
set_delay = bind('libvlc_video_set_spu_delay', C.c_int, C.c_void_p, C.c_int64)

options = ['--ignore-config', '--no-media-library', '--no-metadata-network-access',
           '--no-ml-network-discovery', '--no-osd', '--no-video-title-show',
           '--no-sub-autodetect-file', '--vout=vdummy', '--no-hw-dec',
           '--sub-language=eng', '--sub-track=-1', '--sub-track-id=',
           '--sub-delay=' + str(args.delay_ms // 100), '--verbose=2',
           '--file-logging', '--logfile=' + str(out / 'native.log')]
options += (['--aout=mmdevice', '--mmdevice-backend=wasapi']
            if args.audio == 'wasapi' else ['--no-audio'])
encoded = [option.encode() for option in options]
instance = new(len(encoded), (C.c_char_p * len(encoded))(*encoded))
assert instance
player = player_new(instance, None, None)
assert player
result = dict(status='running', passed=False, fullBetaAccepted=False,
              scope='Generated English cue masks, delayed paused seek and live delay/reset '
                    'frame conservation, plus a matched playing boundary. No desktop, '
                    'audibility, full synchronization or full beta acceptance.',
              coreSha256=digest(runtime / 'libvlccore.dll'),
              mediaSha256=digest(media_path), producerSha256=digest(Path(__file__)),
              media=args.media, audio=args.audio, defaultDelayMs=args.delay_ms,
              options=options, observations=[], teardownComplete=False)
caption_masks = {}


def observe(name):
    entry = dict(step=name, state=state(player), timeUs=get_time(player),
                 delayUs=get_delay(player), vouts=vouts(player))
    if entry['vouts']:
        picture = out / (name + '.png')
        entry['snapshotReturn'] = snapshot(player, 0, str(picture).encode(), 960, 540)
        entry['snapshotAvailable'] = wait(
            lambda: picture.exists() and not QImage(str(picture)).isNull(), 3)
        if entry['snapshotAvailable']:
            image = QImage(str(picture)).convertToFormat(QImage.Format_RGBA8888)
            pixels = bytes(image.bits())
            mask = bytes(1 if pixels[i] > 160 and pixels[i+1] > 160
                         and pixels[i+2] > 160 else 0
                         for i in range(0, len(pixels), 4))
            caption_masks[name] = mask
            entry.update(snapshot=picture.name, snapshotSha256=digest(picture),
                         captionMaskSha256=sha256(mask).hexdigest(),
                         brightPixels=sum(mask), width=image.width(), height=image.height(),
                         frameMarker=sum((1 << bit) if image.pixelColor(32+64*bit, 20).red()
                                         > 128 else 0 for bit in range(11)))
    entry['pausedSamples'] = []
    for unused in range(3):
        entry['pausedSamples'].append(dict(state=state(player), timeUs=get_time(player)))
        time.sleep(.1)
    result['observations'].append(entry)
    write()
    print(name, 'delay', entry['delayUs'], 'frame', entry.get('frameMarker'),
          'caption pixels', entry.get('brightPixels'), flush=True)
    return entry


def seek_paused(seconds, name):
    assert seek(player, round(seconds * 1_000_000), False) == 0
    time.sleep(1)
    return observe(name)


try:
    media = media_new(str(media_path).encode())
    assert media
    media_option(media, b':start-paused')
    set_media(player, media)
    media_release(media)
    assert play(player) == 0
    result['initialPauseReached'] = wait(lambda: state(player) == 3, 10)
    time.sleep(.7)
    observe('cold-paused')
    seek_paused(5, 'reference-first')
    seek_paused(25, 'reference-middle')
    seek_paused(20.5, 'boundary-seek')
    time.sleep(.7)
    observe('boundary-settled')
    for name, delay_ms in [('live-reset', 0), ('live-positive', 1200),
                           ('live-negative', -1200), ('live-zero', 0)]:
        assert set_delay(player, delay_ms * 1000) == 0
        time.sleep(1)
        observe(name)
    assert set_delay(player, args.delay_ms * 1000) == 0
    seek_paused(18, 'before-playing-boundary')
    pause(player, 0)
    result['playingBoundaryReached'] = wait(
        lambda: state(player) == 2 and get_time(player) >= 20_200_000, 8)
    pause(player, 1)
    result['playingPauseReached'] = wait(lambda: state(player) == 3, 4)
    time.sleep(.3)
    observe('after-playing-boundary')
except Exception:
    result['executionError'] = traceback.format_exc()
finally:
    result['stopReturn'] = stop(player)
    result['stopReached'] = wait(lambda: state(player) == 4, 10)
    write()
    player_release(player)
    release(instance)
    result['teardownComplete'] = True

lines = (out / 'native.log').read_text(encoding='utf-8', errors='replace').splitlines()
result['diagnostics'] = [line for line in lines if re.search(
    r'\berror:|Assertion failed|Binding loop|ReferenceError|TypeError', line)]
result['actualWasapiSelected'] = any('using aout stream module "wasapi"' in line
                                     for line in lines)
rows = {row['step']: row for row in result['observations']}
result['captionComparisons'] = {}


def matches(actual_name, reference_name):
    actual = caption_masks[actual_name]
    reference = caption_masks[reference_name]
    mismatch = sum(a != b for a, b in zip(actual, reference))
    union = sum(a or b for a, b in zip(actual, reference))
    result['captionComparisons'][actual_name + ':' + reference_name] = dict(
        differingPixels=mismatch, unionPixels=union, maximumDifferenceFraction=.005)
    # Different video colors can change a few blended glyph-edge pixels. The
    # allowance is relative to the caption union, not the whole video image;
    # empty or different fixture cues still fail by a large margin.
    return union > 1000 and mismatch <= union * .005


checks = dict(executionComplete='executionError' not in result,
              initialPause=result.get('initialPauseReached', False),
              teardown=result['stopReached'] and result['teardownComplete'],
              audioBackend=args.audio != 'wasapi' or result['actualWasapiSelected'])
if 'executionError' not in result:
    first = rows['reference-first']['captionMaskSha256']
    middle = rows['reference-middle']['captionMaskSha256']
    boundary = rows['boundary-seek']
    boundary_reference = 'reference-first' if args.delay_ms > 0 else 'reference-middle'
    live_names = ['live-reset', 'live-positive', 'live-negative', 'live-zero']
    checks.update(
        distinctReferenceCaptions=first != middle and all(
            rows[name].get('brightPixels', 0) > 1000
            for name in ['reference-first', 'reference-middle']),
        defaultDelayReadback=all(rows[name]['delayUs'] == args.delay_ms * 1000
                                for name in ['reference-first', 'reference-middle',
                                             'boundary-seek', 'boundary-settled']),
        boundarySeekCaption=matches('boundary-seek', boundary_reference),
        boundarySettledCaption=matches('boundary-settled', boundary_reference),
        boundaryPictureSettled=boundary.get('frameMarker')
                               == rows['boundary-settled'].get('frameMarker'),
        pausedPictures=all(row['state'] == 3 and row.get('snapshotAvailable')
                           for row in result['observations']),
        sampledPause=all(all(sample['state'] == 3 for sample in row['pausedSamples'])
                         and len({sample['timeUs'] for sample in row['pausedSamples']}) == 1
                         for row in result['observations']),
        liveFramesConserved=all(rows[name].get('frameMarker') == boundary.get('frameMarker')
                                for name in live_names),
        liveDelayReadback=all(rows[name]['delayUs'] == value * 1000
                             for name, value in zip(live_names, [0, 1200, -1200, 0])),
        playingControlReached=result['playingBoundaryReached'] and result['playingPauseReached'],
        playingControlFrame=505 <= rows['after-playing-boundary'].get('frameMarker', -1) <= 525,
        playingControlCaption=matches('after-playing-boundary', boundary_reference))
    for name in live_names:
        checks[name + 'Caption'] = matches(name,
            'reference-first' if name == 'live-positive' else 'reference-middle')
    if args.audio == 'none':
        checks['liveReportedTimesConserved'] = all(
            rows[name]['timeUs'] == boundary['timeUs'] for name in live_names)
result.update(status='terminal', checks=checks, passed=all(checks.values()))
write()
print(json.dumps(dict(passed=result['passed'], checks=checks)), flush=True)
if 'executionError' in result:
    raise SystemExit(result['executionError'])
