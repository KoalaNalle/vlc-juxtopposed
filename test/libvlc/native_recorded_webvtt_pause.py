# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 VLC authors and VideoLAN
"""Actual LibVLC paused selection and seek snapshots of the generated recording."""
import ctypes as C
import hashlib
import json
import os
from pathlib import Path
import sys
import time
import traceback
from PySide6.QtGui import QImage

runtime, out, media_path = map(Path, sys.argv[1:4])
start = float(sys.argv[4])
assert (start == 0 or 12 <= start <= 12.04) and out.is_dir() and not any(out.iterdir())
directory = os.add_dll_directory(str(runtime))
vlc = C.CDLL(str(runtime / 'libvlc.dll'))


def bind(name, result, *args):
    f=getattr(vlc,name);f.restype=result;f.argtypes=args;return f


new=bind('libvlc_new',C.c_void_p,C.c_int,C.POINTER(C.c_char_p))
release=bind('libvlc_release',None,C.c_void_p)
media_new=bind('libvlc_media_new_path',C.c_void_p,C.c_char_p)
media_release=bind('libvlc_media_release',None,C.c_void_p)
media_option=bind('libvlc_media_add_option',None,C.c_void_p,C.c_char_p)
player_new=bind('libvlc_media_player_new',C.c_void_p,C.c_void_p,C.c_void_p,C.c_void_p)
player_release=bind('libvlc_media_player_release',None,C.c_void_p)
set_media=bind('libvlc_media_player_set_media',None,C.c_void_p,C.c_void_p)
play=bind('libvlc_media_player_play',C.c_int,C.c_void_p)
state=bind('libvlc_media_player_get_state',C.c_int,C.c_void_p)
clock=bind('libvlc_media_player_get_time',C.c_int64,C.c_void_p)
seek=bind('libvlc_media_player_set_time',C.c_int,C.c_void_p,C.c_int64,C.c_bool)
stop=bind('libvlc_media_player_stop_async',C.c_int,C.c_void_p)
snapshot=bind('libvlc_video_take_snapshot',C.c_int,C.c_void_p,C.c_uint,C.c_char_p,C.c_uint,C.c_uint)
select_ids=bind('libvlc_media_player_select_tracks_by_ids',None,C.c_void_p,C.c_int,C.c_char_p)


class TrackData(C.Union):
    _fields_=[('pointer',C.c_void_p)]


class Track(C.Structure):
    _fields_=[('codec',C.c_uint32),('original',C.c_uint32),('id',C.c_int),('type',C.c_int),
              ('profile',C.c_int),('level',C.c_int),('data',TrackData),('bitrate',C.c_uint),
              ('language',C.c_char_p),('description',C.c_char_p),('string_id',C.c_char_p),
              ('stable',C.c_bool),('name',C.c_char_p),('selected',C.c_bool)]


tracklist=bind('libvlc_media_player_get_tracklist',C.c_void_p,C.c_void_p,C.c_int,C.c_bool)
track_count=bind('libvlc_media_tracklist_count',C.c_size_t,C.c_void_p)
track_at=bind('libvlc_media_tracklist_at',C.POINTER(Track),C.c_void_p,C.c_size_t)
track_delete=bind('libvlc_media_tracklist_delete',None,C.c_void_p)


def wait(test, seconds=8):
    until=time.monotonic()+seconds
    while time.monotonic()<until:
        if test():return True
        time.sleep(.025)
    return bool(test())


def tracks():
    listing=tracklist(player,2,False)
    if not listing:return []
    try:
        return [dict(id=(track_at(listing,i).contents.string_id or b'').decode(),
                     selected=bool(track_at(listing,i).contents.selected)) for i in range(track_count(listing))]
    finally:track_delete(listing)


def observe(label):
    p=out/(label+'.png');r=dict(step=label,state=state(player),timeUs=clock(player),tracks=tracks())
    r['snapshotReturn']=snapshot(player,0,str(p).encode(),960,540)
    assert r['snapshotReturn']==0 and wait(lambda:p.exists() and not QImage(str(p)).isNull(),4)
    im=QImage(str(p)).convertToFormat(QImage.Format_RGBA8888);data=bytes(im.bits());stride=im.bytesPerLine()
    r['frameMarker']=sum(1<<b for b in range(11) if im.pixelColor(32+64*b,20).red()>128)
    r['captionPixels']=sum(all(data[y*stride+x*4+c]>160 for c in range(3))
                           for y in range(3*im.height()//4,im.height()) for x in range(im.width()))
    result['observations'].append(r);print(json.dumps(r),flush=True);return r


options=['--ignore-config','--no-media-library','--no-metadata-network-access','--no-ml-network-discovery',
         '--no-osd','--no-video-title-show','--no-sub-autodetect-file','--vout=vdummy','--aout=adummy',
         '--no-hw-dec','--sub-language=none','--sub-fps=0','--sub-delay=0','--verbose=2',
         '--file-logging','--logfile='+str(out/'native.log')]
encoded=[x.encode() for x in options]
instance=new(len(encoded),(C.c_char_p*len(encoded))(*encoded));assert instance
player=player_new(instance,None,None);assert player
result=dict(status='running',scope='Generated recorded embedded WebVTT, actual decoder snapshots and paused time/frame; no full beta acceptance',
            startTimeOption=start,observations=[],producerSha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            coreSha256=hashlib.sha256((runtime/'libvlccore.dll').read_bytes()).hexdigest(),
            mediaSha256=hashlib.sha256(media_path.read_bytes()).hexdigest())
try:
    media=media_new(str(media_path).encode());assert media
    media_option(media,b':start-paused')
    if start:media_option(media,(':start-time='+str(start)).encode())
    set_media(player,media);media_release(media);assert play(player)==0
    assert wait(lambda:state(player)==3 and len(tracks())==1)
    select_ids(player,2,b'');assert wait(lambda:not any(t['selected'] for t in tracks()))
    if not start:assert seek(player,12_100_000,False)==0
    time.sleep(.6);before=observe('01-paused-with-no-track')
    assert before['captionPixels']==0
    select_ids(player,2,tracks()[0]['id'].encode());assert wait(lambda:all(t['selected'] for t in tracks()))
    time.sleep(.5);observe('02-selected-half-second')
    time.sleep(1);observe('03-selected-one-more-second')
    time.sleep(4.5);selected=observe('04-selected-after-refresh-window')
    assert seek(player,round((12.1-start)*1_000_000),False)==0
    time.sleep(.6);explicit=observe('05-explicit-seek-file-12.1')
    early=None
    if not start:
        assert seek(player,5_000_000,False)==0
        time.sleep(.6);early=observe('06-explicit-seek-5')
    else:
        result['notApplicable']=['File time 5 lies before the start-time=12 playback interval']
    assert seek(player,round((25-start)*1_000_000),False)==0
    time.sleep(.6);late=observe('07-explicit-seek-25')
    result['checks']=dict(selectionRendered=selected['captionPixels']>100,
                          selectionHeldFrame=selected['frameMarker']==before['frameMarker'],
                          selectionHeldTime=abs(selected['timeUs']-before['timeUs'])<50_000,
                          explicitSeekRenders=explicit['captionPixels']>100,
                          lateCueRenders=late['captionPixels']>100)
    if early is not None:result['checks']['beforeCueIsEmpty']=early['captionPixels']==0
except Exception:result['executionError']=traceback.format_exc()
finally:
    result['stopReturn']=stop(player);result['stopReached']=wait(lambda:state(player)==4,10)
    player_release(player);release(instance)
    result['status']='terminal';result['teardownComplete']=True
    result['passed']='executionError' not in result and result['stopReached'] and all(result.get('checks',{}).values())
    (out/'result.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k in ['passed','checks','executionError']}),flush=True)
sys.exit(0 if result['passed'] else 1)
