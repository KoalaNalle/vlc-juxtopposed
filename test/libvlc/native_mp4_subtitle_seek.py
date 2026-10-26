# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 VLC authors and VideoLAN
"""Generated-media diagnostic: positive subtitle delay, held frame and resume."""
from pathlib import Path
from hashlib import sha256
import ctypes as C, json, os, sys, time, traceback, collections
from PySide6.QtGui import QImage

runtime, out, media_path, subtitle_path = map(Path, sys.argv[1:5])
rate, audio = float(sys.argv[5]), sys.argv[6]
seek_seconds, selection = float(sys.argv[7]), sys.argv[8]
assert selection in ['eng','fin','both','none']
assert 0 < seek_seconds < 54
assert out.is_dir() and not any(out.iterdir())
digest = lambda p: sha256(p.read_bytes()).hexdigest()
directory = os.add_dll_directory(str(runtime))
vlc = C.CDLL(str(runtime/'libvlc.dll'))
def bind(name, result, *types):
    fn = getattr(vlc, name); fn.restype=result; fn.argtypes=types; return fn
new=bind('libvlc_new',C.c_void_p,C.c_int,C.POINTER(C.c_char_p))
release=bind('libvlc_release',None,C.c_void_p)
media_new=bind('libvlc_media_new_path',C.c_void_p,C.c_char_p)
media_release=bind('libvlc_media_release',None,C.c_void_p)
media_option=bind('libvlc_media_add_option',None,C.c_void_p,C.c_char_p)
player_new=bind('libvlc_media_player_new',C.c_void_p,C.c_void_p,C.c_void_p,C.c_void_p)
player_release=bind('libvlc_media_player_release',None,C.c_void_p)
set_media=bind('libvlc_media_player_set_media',None,C.c_void_p,C.c_void_p)
play=bind('libvlc_media_player_play',C.c_int,C.c_void_p)
pause=bind('libvlc_media_player_set_pause',None,C.c_void_p,C.c_int)
stop=bind('libvlc_media_player_stop_async',C.c_int,C.c_void_p)
state=bind('libvlc_media_player_get_state',C.c_int,C.c_void_p)
clock=bind('libvlc_media_player_get_time',C.c_int64,C.c_void_p)
seek=bind('libvlc_media_player_set_time',C.c_int,C.c_void_p,C.c_int64,C.c_bool)
get_rate=bind('libvlc_media_player_get_rate',C.c_float,C.c_void_p)
snapshot=bind('libvlc_video_take_snapshot',C.c_int,C.c_void_p,C.c_uint,C.c_char_p,C.c_uint,C.c_uint)
get_delay=bind('libvlc_video_get_spu_delay',C.c_int64,C.c_void_p)
set_delay=bind('libvlc_video_set_spu_delay',C.c_int,C.c_void_p,C.c_int64)

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
select_track=bind('libvlc_media_player_select_track',None,C.c_void_p,C.POINTER(Track))
select_ids=bind('libvlc_media_player_select_tracks_by_ids',None,C.c_void_p,C.c_int,C.c_char_p)

def wait(predicate, seconds=8):
    end=time.monotonic()+seconds
    while time.monotonic()<end:
        if predicate():return True
        time.sleep(.02)
    return bool(predicate())
options=['--ignore-config','--no-media-library','--no-metadata-network-access','--no-ml-network-discovery',
         '--no-osd','--no-video-title-show','--no-sub-autodetect-file','--vout=vdummy','--no-hw-dec',
         '--sub-language=eng','--sub-fps=0','--sub-delay=0','--sub-file='+str(subtitle_path),
         '--rate='+str(rate),'--verbose=2','--file-logging','--logfile='+str(out/'native.log')]
options += ['--aout=mmdevice','--mmdevice-backend=wasapi'] if audio=='wasapi' else ['--aout=adummy']
options += ['--sub-primary-alignment=8','--secondary-sub-alignment=4','--secondary-sub-margin=0']
encoded=[s.encode() for s in options]
instance=new(len(encoded),(C.c_char_p*len(encoded))(*encoded)); assert instance
player=player_new(instance,None,None); assert player
result=dict(status='running',passed=False,fullBetaAccepted=False,teardownComplete=False,
            options=options,coreSha256=digest(runtime/'libvlccore.dll'),mediaSha256=digest(media_path),
            subtitleSha256=digest(subtitle_path),producerSha256=digest(Path(__file__)),observations=[])
def write(): (out/'result.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8',newline='\n')
def tracks(select_embedded=False):
    listing=tracklist(player,2,False)
    if not listing:return []
    rows=[]
    try:
        for i in range(track_count(listing)):
            ptr=track_at(listing,i); item=ptr.contents
            identity=(item.string_id or b'').decode()
            rows.append(dict(id=identity,selected=bool(item.selected),name=(item.name or b'').decode(),language=(item.language or b'').decode()))
            if select_embedded and (item.language or b'').decode().lower() in ['en','eng']:
                select_track(player,ptr)
    finally:track_delete(listing)
    return rows
def observe(name):
    row=dict(step=name,state=state(player),timeUs=clock(player),delayUs=get_delay(player),rate=get_rate(player),tracks=tracks())
    picture=out/(name+'.png')
    row['snapshotReturn']=snapshot(player,0,str(picture).encode(),960,540)
    row['snapshotAvailable']=wait(lambda:picture.exists() and not QImage(str(picture)).isNull(),3)
    if row['snapshotAvailable']:
        im=QImage(str(picture)).convertToFormat(QImage.Format_RGBA8888)
        raw=bytes(im.bits()); stride=im.bytesPerLine()
        row.update(frameMarker=sum(1<<b for b in range(11) if im.pixelColor(32+64*b,20).red()>128),
                   captionPixels=sum(raw[y*stride+x*4]>160 and raw[y*stride+x*4+1]>160 and raw[y*stride+x*4+2]>160
                                     for y in range(3*im.height()//4,im.height()) for x in range(im.width())),
                   snapshotSha256=digest(picture))
    if row['snapshotAvailable']:
        row['secondaryCaptionPixels']=sum(raw[y*stride+x*4]>160 and raw[y*stride+x*4+1]>160 and raw[y*stride+x*4+2]>160
            for y in range(im.height()//4) for x in range(im.width()))
    row['observedMonotonicSeconds']=time.monotonic()
    result['observations'].append(row);write()
    print(name,row['timeUs'],row.get('frameMarker'),row.get('captionPixels'),flush=True)
    return row
try:
    media=media_new(str(media_path).encode());assert media
    media_option(media,b':start-paused');set_media(player,media);media_release(media)
    assert play(player)==0
    assert wait(lambda:state(player)==3)
    assert wait(lambda:len(tracks())==3)
    languages = {'eng':['en','eng'], 'fin':['fi','fin'], 'both':['en','eng','fi','fin'], 'none':[]}[selection]
    chosen=[t['id'] for t in tracks() if t['language'].lower() in languages]
    assert len(chosen)==(2 if selection=='both' else 0 if selection=='none' else 1),tracks()
    result.update(seekSeconds=seek_seconds,selection=selection,selectedTrackIds=chosen)
    select_ids(player,2,','.join(chosen).encode())
    assert wait(lambda:{t['id'] for t in tracks() if t['selected']}==set(chosen))
    assert seek(player,int(seek_seconds*1_000_000),False)==0
    time.sleep(.8);observe('reference-zero')
    for delay in [5_000_000,5_508_000,0,5_508_000,0]:
        assert set_delay(player,delay)==0
        time.sleep(.8);observe('delay-'+str(delay)+'-'+str(len(result['observations'])))
    pause(player,0);assert wait(lambda:state(player)==2)
    initial=clock(player)
    assert wait(lambda:clock(player)>initial+2_000_000)
    observe('playing-two-seconds')
    time.sleep(2);observe('playing-four-seconds')
    pause(player,1);assert wait(lambda:state(player)==3)
    time.sleep(.3);observe('paused-after-resume')
except Exception:result['executionError']=traceback.format_exc()
finally:
    result['stopReturn']=stop(player);result['stopReached']=wait(lambda:state(player)==4,10);write()
    player_release(player);release(instance);result['teardownComplete']=True
lines=(out/'native.log').read_text(encoding='utf-8',errors='replace').splitlines()
result['diagnosticCounts']=dict(collections.Counter(s.split('error:',1)[1].strip() for s in lines if 'error:' in s))
result['latePictureWarnings']=sum('picture is too late' in s for s in lines)
result['wasapiSelected']=any('using aout stream module "wasapi"' in s for s in lines)
result['checks']=dict(executed='executionError' not in result,normalTeardown=result['stopReached'] and result['teardownComplete'],
    captions=all((r.get('captionPixels',0)==0 and r.get('secondaryCaptionPixels',0)==0) if selection=='none' else (r.get('captionPixels',0)>100 and (selection!='both' or r.get('secondaryCaptionPixels',0)>100)) for r in result['observations']),
    heldFrame=len({r.get('frameMarker') for r in result['observations'][:6]})==1,
    audioBackend=audio!='wasapi' or result['wasapiSelected'],noLatePictureWarnings=result['latePictureWarnings']==0)
result.update(status='terminal',passed=all(result['checks'].values()));write()
print(json.dumps(dict(passed=result['passed'],checks=result['checks'],diagnosticCount=sum(result['diagnosticCounts'].values()),late=result['latePictureWarnings'])),flush=True)
