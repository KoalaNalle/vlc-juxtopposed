from pathlib import Path
work=Path(__file__).resolve().parent
repo=Path('C:/Modding/vlc-juxtopposed')
source=(work/'native-startup-edges-r12.py').read_text(encoding='utf-8')
source='''# SPDX-License-Identifier: GPL-2.0-or-later
"""Observe first-picture delivery, paused clock stability and normal resume.

Run in an isolated, fully staged Windows runtime. The config JSON contains
name, media (absolute path), audio (wasapi/dummy/none), video (boolean), and
optional extraOptions. A normal exit records observations; inspect result.json
passed separately, allowing the identical probe to record a failing old Core.
Snapshot files delivered only after resume do not qualify as paused pictures.
"""
'''+source
before="snapshot = bind('libvlc_video_take_snapshot', C.c_int, C.c_void_p, C.c_uint, C.c_char_p, C.c_uint, C.c_uint)"
after=before+'''
get_tracks = bind('libvlc_media_player_get_tracklist', C.c_void_p, C.c_void_p, C.c_int, C.c_bool)
count_tracks = bind('libvlc_media_tracklist_count', C.c_size_t, C.c_void_p)
delete_tracks = bind('libvlc_media_tracklist_delete', None, C.c_void_p)
def selected_video_count():
    listing = get_tracks(player, 1, True)
    if not listing: return 0
    try: return count_tracks(listing)
    finally: delete_tracks(listing)
'''
assert source.count(before)==1; source=source.replace(before,after)
source=source.replace('timeUs=get_time(player), vouts=vouts(player))', 'timeUs=get_time(player), vouts=vouts(player), selectedVideoTracks=selected_video_count())')
assert source.count("if initial['vouts']:")==1
source=source.replace("if initial['vouts']:","if cfg['video'] and initial['vouts']:")
assert source.count("else initial['vouts']==0")==1
source=source.replace("else initial['vouts']==0", "else initial['selectedVideoTracks']==0")
source=source.replace("initialPaused=result['initialPauseReached'], initialPicture=", "initialPaused=result['initialPauseReached'], expectedVideoSelection=initial['selectedVideoTracks']==int(cfg['video']), initialPicture=")
target=repo/'test/libvlc/native_paused_video_startup.py'
assert not target.exists();target.write_text(source,encoding='utf-8',newline='\n')
parent=(work/'run-native-startup-edges-r12.py').read_text(encoding='utf-8')
parent=parent.replace("worker = work/'native-startup-edges-r12.py'", "worker = repo/'test/libvlc/native_paused_video_startup.py'")
parent=parent.replace("root = work/'native-startup-edges-r12'", "root = work/'native-startup-edges-r13'")
start=parent.index('definitions = [');end=parent.index('\nfixtures =',start)
parent=parent[:start]+'''definitions = [
    ('original-wasapi', 'media-r1/multi-language.mkv', 'wasapi', True, []),
    ('audio-only-wasapi', 'media-r12/audio-only.mka', 'wasapi', False, []),
    ('single-picture', 'media-r12/single-picture.mkv', 'none', True, []),
]
'''+parent[end:]
target=work/'run-native-startup-edges-r13.py'
assert not target.exists();target.write_text(parent,encoding='utf-8',newline='\n')
