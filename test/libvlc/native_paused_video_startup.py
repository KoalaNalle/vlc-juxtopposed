# SPDX-License-Identifier: GPL-2.0-or-later
"""Observe first-picture delivery, paused clock stability and normal resume.

Run in an isolated, fully staged Windows runtime. The config JSON contains
name, media (absolute path), audio (wasapi/dummy/none), video (boolean), and
optional extraOptions and expectedSubtitleAtStart for the generated flat-colour
caption fixture. A normal exit records observations; inspect result.json
passed separately, allowing the identical probe to record a failing old Core.
Snapshot files delivered only after resume do not qualify as paused pictures.
"""
from pathlib import Path
from hashlib import sha256
import argparse, ctypes as C, json, os, re, time
from PySide6.QtGui import QImage

parser = argparse.ArgumentParser()
parser.add_argument('--config', type=Path, required=True)
parser.add_argument('--runtime', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
cfg = json.loads(args.config.read_text(encoding='utf-8-sig'))
out, runtime = args.output, args.runtime
digest = lambda p: sha256(p.read_bytes()).hexdigest()
def write(v): (out/'result.json').write_text(json.dumps(v, indent=2)+'\n', encoding='utf-8', newline='\n')
directory = os.add_dll_directory(str(runtime))
vlc = C.CDLL(str(runtime/'libvlc.dll'))
def bind(name, result, *types):
    fn = getattr(vlc, name); fn.restype = result; fn.argtypes = types; return fn
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
vouts = bind('libvlc_media_player_has_vout', C.c_uint, C.c_void_p)
get_rate = bind('libvlc_media_player_get_rate', C.c_float, C.c_void_p)
snapshot = bind('libvlc_video_take_snapshot', C.c_int, C.c_void_p, C.c_uint, C.c_char_p, C.c_uint, C.c_uint)
get_tracks = bind('libvlc_media_player_get_tracklist', C.c_void_p, C.c_void_p, C.c_int, C.c_bool)
count_tracks = bind('libvlc_media_tracklist_count', C.c_size_t, C.c_void_p)
delete_tracks = bind('libvlc_media_tracklist_delete', None, C.c_void_p)
def selected_video_count():
    listing = get_tracks(player, 1, True)
    if not listing: return 0
    try: return count_tracks(listing)
    finally: delete_tracks(listing)

def wait(predicate, seconds):
    deadline = time.monotonic()+seconds
    while time.monotonic() < deadline:
        if predicate(): return True
        time.sleep(.02)
    return bool(predicate())
options = ['--ignore-config', '--no-media-library', '--no-metadata-network-access', '--no-ml-network-discovery', '--no-osd', '--no-video-title-show', '--no-sub-autodetect-file', '--vout=vdummy', '--no-hw-dec', '--audio-language=eng', '--sub-language=eng', '--verbose=2', '--file-logging', '--logfile='+str(out/'native.log')]
options += ['--aout=mmdevice', '--mmdevice-backend=wasapi'] if cfg['audio']=='wasapi' else (['--aout=adummy'] if cfg['audio']=='dummy' else ['--no-audio'])
options += cfg.get('extraOptions', [])
encoded = [v.encode() for v in options]
instance = new(len(encoded), (C.c_char_p*len(encoded))(*encoded)); assert instance
player = player_new(instance, None, None); assert player
media_path = Path(cfg['media'])
result = dict(config=cfg, passed=False, options=options, producerSha256=digest(Path(__file__)), coreSha256=digest(runtime/'libvlccore.dll'), mediaSha256=digest(media_path), observations=[], teardownComplete=False, assertions={})
def observe(name):
    row = dict(step=name, state=state(player), timeUs=get_time(player), vouts=vouts(player), selectedVideoTracks=selected_video_count(), rate=get_rate(player))
    result['observations'].append(row); write(result); return row
media = media_new(str(media_path).encode()); assert media
media_option(media, b':start-paused'); set_media(player, media); media_release(media)
try:
    result['playReturn'] = play(player)
    result['initialPauseReached'] = wait(lambda: state(player)==3, 10)
    time.sleep(.6)
    initial = observe('initial-paused')
    if cfg['video'] and initial['vouts']:
        picture = out/'initial-paused.png'
        initial['snapshotReturn'] = snapshot(player, 0, str(picture).encode(), 960, 540)
        initial['snapshotAvailableWhilePaused'] = wait(lambda: picture.exists() and not QImage(str(picture)).isNull(), 2)
        if initial['snapshotAvailableWhilePaused']:
            image = QImage(str(picture))
            initial.update(snapshotSha256=digest(picture), width=image.width(), height=image.height())
            if 'expectedSubtitleAtStart' in cfg:
                rgba = image.convertToFormat(QImage.Format_RGBA8888)
                pixels = bytes(rgba.bits())
                initial['brightPixels'] = sum(pixels[i] > 160 and pixels[i+1] > 160 and pixels[i+2] > 160
                                             for i in range(0, len(pixels), 4))
    samples = []
    for i in range(4):
        samples.append(observe('held-paused-'+str(i))); time.sleep(.2)
    stable = len({r['timeUs'] for r in samples})==1 and all(r['state']==3 for r in samples)
    pause(player, 0)
    resumed = wait(lambda: state(player)==2, 5)
    advanced = wait(lambda: get_time(player) > samples[-1]['timeUs']+200_000 or state(player)==4, 8)
    observe('resumed')
    pause(player, 1)
    repaused = wait(lambda: state(player)==3 or state(player)==4, 3)
    observe('paused-again-or-ended')
    result['assertions'] = dict(initialPaused=result['initialPauseReached'], expectedVideoSelection=initial['selectedVideoTracks']==int(cfg['video']), initialPicture=bool(initial.get('snapshotAvailableWhilePaused')) if cfg['video'] else initial['selectedVideoTracks']==0, heldPausedClockStable=stable, resumedOrShortMediaEnded=resumed or state(player)==4, advancedOrShortMediaEnded=advanced, repausedOrShortMediaEnded=repaused)
    if 'expectedSubtitleAtStart' in cfg:
        bright = initial.get('brightPixels')
        result['assertions']['initialSubtitlePresence'] = bright is not None and (
            bright > 1000 if cfg['expectedSubtitleAtStart'] else bright <= 10)
finally:
    result['stopReturn'] = stop(player)
    result['stopReached'] = wait(lambda: state(player)==4, 10)
    write(result)
    player_release(player); release(instance); result['teardownComplete']=True
lines = (out/'native.log').read_text(encoding='utf-8', errors='replace').splitlines()
result.update(diagnostics=[s for s in lines if re.search(r'\berror:|Assertion failed|Binding loop|ReferenceError|TypeError',s)], warnings=[s for s in lines if 'warning:' in s], actualWasapiSelected=any('using aout stream module "wasapi"' in s for s in lines))
result['passed'] = all(result['assertions'].values()) and result['stopReached'] and result['teardownComplete']
write(result)
assert result['stopReached'] and result['teardownComplete']
print(cfg['name'], 'normal terminal', 'assertions', result['assertions'], flush=True)
