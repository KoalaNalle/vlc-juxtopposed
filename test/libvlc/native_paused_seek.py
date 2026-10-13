#!/usr/bin/env python3
"""Exercise a built Windows LibVLC 4 with a red/blue/green six-second fixture.

Pass --package, --media and --output-dir. The media must contain three six-second
solid-color segments (red, blue, green). Uses the Qt image reader already used by
the UI checks; playback and snapshots come from the actual packaged LibVLC.
"""
import argparse
import ctypes as C
import json
import os
from pathlib import Path
import time

from PySide6.QtGui import QImage

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--package', type=Path, required=True)
parser.add_argument('--media', type=Path, required=True)
parser.add_argument('--output-dir', type=Path, required=True)
args = parser.parse_args()
package = args.package.resolve()
output = args.output_dir.resolve()
output.mkdir(parents=True, exist_ok=True)
(output / 'native.log').write_text('')
dll_directory = os.add_dll_directory(str(package))
os.environ['VLC_PLUGIN_PATH'] = str(package / 'plugins')
vlc = C.CDLL(str(package / 'libvlc.dll'))

def bind(name, restype, *argtypes):
    fn = getattr(vlc, name)
    fn.restype, fn.argtypes = restype, argtypes
    return fn

new = bind('libvlc_new', C.c_void_p, C.c_int, C.POINTER(C.c_char_p))
release = bind('libvlc_release', None, C.c_void_p)
media_new = bind('libvlc_media_new_path', C.c_void_p, C.c_char_p)
media_release = bind('libvlc_media_release', None, C.c_void_p)
player_new = bind('libvlc_media_player_new', C.c_void_p, C.c_void_p, C.c_void_p, C.c_void_p)
player_release = bind('libvlc_media_player_release', None, C.c_void_p)
set_media = bind('libvlc_media_player_set_media', None, C.c_void_p, C.c_void_p)
play = bind('libvlc_media_player_play', C.c_int, C.c_void_p)
pause = bind('libvlc_media_player_set_pause', None, C.c_void_p, C.c_int)
stop = bind('libvlc_media_player_stop_async', C.c_int, C.c_void_p)
state = bind('libvlc_media_player_get_state', C.c_int, C.c_void_p)
get_time = bind('libvlc_media_player_get_time', C.c_int64, C.c_void_p)
seek = bind('libvlc_media_player_set_time', C.c_int, C.c_void_p, C.c_int64, C.c_bool)
snapshot = bind('libvlc_video_take_snapshot', C.c_int, C.c_void_p, C.c_uint,
                C.c_char_p, C.c_uint, C.c_uint)

options = [b'--ignore-config', b'--no-media-library', b'--no-audio',
           b'--no-spu', b'--no-osd', b'--no-video-title-show', b'--vout=vdummy',
           b'--no-hw-dec', b'--verbose=2', b'--file-logging',
           ('--logfile=' + str(output / 'native.log')).encode()]
instance = new(len(options), (C.c_char_p * len(options))(*options))
assert instance, 'LibVLC initialization failed'
player = player_new(instance, None, None)
assert player
media = media_new(str(args.media.resolve()).encode())
assert media
set_media(player, media)
media_release(media)
results = []

def wait_for(predicate, label, timeout=10):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if predicate():
            return
        time.sleep(0.02)
    raise AssertionError(label)

def check_picture(name, color):
    path = output / (name + '.png')
    path.unlink(missing_ok=True)
    assert snapshot(player, 0, str(path).encode(), 320, 180) == 0
    wait_for(lambda: path.exists() and not QImage(str(path)).isNull(), 'Snapshot missing')
    pixel = QImage(str(path)).pixelColor(160, 90)
    rgb = [pixel.red(), pixel.green(), pixel.blue()]
    assert rgb[color] > 180 and all(v < 80 for i, v in enumerate(rgb) if i != color), (name, rgb)
    results.append({'snapshot': path.name, 'rgb': rgb, 'state': state(player),
                    'time_us': get_time(player)})

try:
    assert play(player) == 0
    wait_for(lambda: state(player) == 2 and get_time(player) >= 500_000, 'Playback did not start')
    pause(player, 1)
    wait_for(lambda: state(player) == 3, 'Pause did not apply')
    check_picture('initial-red', 0)
    # Exceed caching duration so wall time cannot masquerade as PCR lateness.
    time.sleep(2)
    for index, (target, color) in enumerate([(8, 2), (14, 1), (2, 0)]):
        assert seek(player, target * 1_000_000, False) == 0
        wait_for(lambda: abs(get_time(player) - target * 1_000_000) < 250_000,
                 'Requested seek time did not arrive')
        time.sleep(0.4)
        assert state(player) == 3, 'Seek resumed playback'
        check_picture('seek-' + str(index), color)
        paused_time = get_time(player)
        time.sleep(0.3)
        assert get_time(player) == paused_time, 'Paused time advanced'
    pause(player, 0)
    wait_for(lambda: state(player) == 2 and get_time(player) > 2_500_000,
             'Resume did not advance from the target')
    assert stop(player) == 0
    wait_for(lambda: state(player) == 4, 'Stop did not finish')
finally:
    stop(player)
    player_release(player)
    release(instance)

log = (output / 'native.log').read_text(errors='replace')
assert 'ES_OUT_SET_(GROUP_)PCR  is called' not in log, 'Clock lateness/reset loop'
(output / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
print('Native LibVLC passes paused target-frame updates, repeated seeks, frozen time and resume.')
