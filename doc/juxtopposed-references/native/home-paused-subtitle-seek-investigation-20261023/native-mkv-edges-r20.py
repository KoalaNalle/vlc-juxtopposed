"""Native investigation, not a passing subtitle/startup regression."""
from pathlib import Path
from hashlib import sha256
import argparse,ctypes as C,json,os,re,time
from PySide6.QtGui import QImage
parser=argparse.ArgumentParser();parser.add_argument('--scenario',type=Path,required=True);parser.add_argument('--runtime',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent;out=args.output;runtime=args.runtime
cfg=json.loads(args.scenario.read_text(encoding='utf-8-sig'));mediaPath=work/cfg['media'];audio=cfg['audio']

def write(name,v):(out/name).write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
def digest(p):return sha256(p.read_bytes()).hexdigest()
dllDirectory=os.add_dll_directory(str(runtime));os.environ['VLC_PLUGIN_PATH']=str(runtime/'plugins');os.environ['VLC_LIBEXEC_PATH']=str(runtime/'vlc-preparser.exe')
vlc=C.CDLL(str(runtime/'libvlc.dll'))
def bind(name,restype,*argtypes):
    fn=getattr(vlc,name);fn.restype=restype;fn.argtypes=argtypes;return fn
new=bind('libvlc_new',C.c_void_p,C.c_int,C.POINTER(C.c_char_p));release=bind('libvlc_release',None,C.c_void_p)
mediaNew=bind('libvlc_media_new_path',C.c_void_p,C.c_char_p);mediaRelease=bind('libvlc_media_release',None,C.c_void_p)
mediaOption=bind('libvlc_media_add_option',None,C.c_void_p,C.c_char_p)
playerNew=bind('libvlc_media_player_new',C.c_void_p,C.c_void_p,C.c_void_p,C.c_void_p);playerRelease=bind('libvlc_media_player_release',None,C.c_void_p)
setMedia=bind('libvlc_media_player_set_media',None,C.c_void_p,C.c_void_p);play=bind('libvlc_media_player_play',C.c_int,C.c_void_p)
pause=bind('libvlc_media_player_set_pause',None,C.c_void_p,C.c_int);stop=bind('libvlc_media_player_stop_async',C.c_int,C.c_void_p)
state=bind('libvlc_media_player_get_state',C.c_int,C.c_void_p);getTime=bind('libvlc_media_player_get_time',C.c_int64,C.c_void_p)
seek=bind('libvlc_media_player_set_time',C.c_int,C.c_void_p,C.c_int64,C.c_bool);vouts=bind('libvlc_media_player_has_vout',C.c_uint,C.c_void_p)
snapshot=bind('libvlc_video_take_snapshot',C.c_int,C.c_void_p,C.c_uint,C.c_char_p,C.c_uint,C.c_uint)
select=bind('libvlc_media_player_select_tracks_by_ids',None,C.c_void_p,C.c_int,C.c_char_p)
slave=bind('libvlc_media_player_add_slave',C.c_int,C.c_void_p,C.c_int,C.c_char_p,C.c_bool)
class Track(C.Structure):
    _fields_=[('codec',C.c_uint32),('originalCodec',C.c_uint32),('numericId',C.c_int),('type',C.c_int),('profile',C.c_int),('level',C.c_int),('data',C.c_void_p),('bitrate',C.c_uint),('language',C.c_char_p),('description',C.c_char_p),('id',C.c_char_p),('idStable',C.c_bool),('name',C.c_char_p),('selected',C.c_bool)]
assert C.sizeof(Track)==88
tracklist=bind('libvlc_media_player_get_tracklist',C.c_void_p,C.c_void_p,C.c_int,C.c_bool)
trackcount=bind('libvlc_media_tracklist_count',C.c_size_t,C.c_void_p);trackat=bind('libvlc_media_tracklist_at',C.POINTER(Track),C.c_void_p,C.c_size_t);trackdelete=bind('libvlc_media_tracklist_delete',None,C.c_void_p)
decode=lambda value:value.decode('utf-8',errors='replace') if value else None
def tracks():
    listing=tracklist(player,2,False)
    if not listing:return []
    try:
        values=[]
        for index in range(trackcount(listing)):
            track=trackat(listing,index).contents
            values.append(dict(id=decode(track.id),language=decode(track.language),name=decode(track.name),selected=bool(track.selected)))
        return values
    finally:trackdelete(listing)
def wait(predicate,timeout):
    end=time.monotonic()+timeout
    while time.monotonic()<end:
        if predicate():return True
        time.sleep(.02)
    return bool(predicate())
class Stats(C.Structure):
    _fields_=[('readBytes',C.c_uint64),('inputBitrate',C.c_float),('demuxReadBytes',C.c_uint64),('demuxBitrate',C.c_float),('demuxCorrupted',C.c_uint64),('demuxDiscontinuity',C.c_uint64),('decodedVideo',C.c_uint64),('decodedAudio',C.c_uint64),('displayedPictures',C.c_uint64),('latePictures',C.c_uint64),('lostPictures',C.c_uint64),('playedAudioBuffers',C.c_uint64),('lostAudioBuffers',C.c_uint64)]
assert C.sizeof(Stats)==104
getStats=bind('libvlc_media_get_stats',C.c_bool,C.c_void_p,C.POINTER(Stats))
options=['--ignore-config','--no-media-library','--no-metadata-network-access','--no-ml-network-discovery','--no-osd','--no-video-title-show','--no-sub-autodetect-file','--vout=vdummy','--no-hw-dec','--audio-language=eng','--sub-language='+cfg['language'],'--stats-min-report-interval=10','--verbose=2','--file-logging','--logfile='+str(out/'native.log')]
options+=['--aout=mmdevice','--mmdevice-backend=wasapi'] if audio else ['--no-audio']
encoded=[v.encode() for v in options];instance=new(len(encoded),(C.c_char_p*len(encoded))(*encoded));assert instance
player=playerNew(instance,None,None);assert player
media=mediaNew(str(mediaPath).encode());assert media
mediaOption(media,b':start-paused');setMedia(player,media)
result=dict(passed=False,candidateOnly=True,repairAccepted=False,configuration=cfg,producerSha256=digest(Path(__file__)),coreSha256=digest(runtime/'libvlccore.dll'),mkvSha256=digest(runtime/'plugins/demux/libmkv_plugin.dll'),fixtureSha256=digest(mediaPath),options=options,observations=[],teardownComplete=False,scope='Native coarse target colors, actual subtitle pixels/masks, frozen paused clock and resume; counters/times are observations, not an accepted performance budget or exact video PTS.')
def stats():
    value=Stats();available=getStats(media,C.byref(value))
    return dict(available=bool(available),**{name:getattr(value,name) for name,_ in Stats._fields_})
def observe(name,expected=None):
    row=dict(step=name,state=state(player),timeUs=getTime(player),vouts=vouts(player),tracks=tracks(),stats=stats())
    if row['vouts']:
        path=out/(name+'.png');row['snapshotReturn']=snapshot(player,0,str(path).encode(),960,540)
        row['snapshotAvailableWhilePaused']=wait(lambda:path.exists() and not QImage(str(path)).isNull(),2)
        if row['snapshotAvailableWhilePaused']:
            im=QImage(str(path)).convertToFormat(QImage.Format_RGBA8888);raw=bytes(im.bits());white=bytes(1 if raw[i]>160 and raw[i+1]>160 and raw[i+2]>160 else 0 for i in range(0,len(raw),4))
            center=im.pixelColor(im.width()//2,im.height()//2);row.update(snapshot=path.name,snapshotSha256=digest(path),rgb=[center.red(),center.green(),center.blue()],brightPixels=sum(white),whiteMaskSha256=sha256(white).hexdigest())
    if expected:
        rgb=row.get('rgb',[-1,-1,-1]);channel=expected['color']
        color_ok=rgb[channel]>180 and all(v<80 for i,v in enumerate(rgb) if i!=channel)
        subtitle_ok=(row.get('brightPixels',0)>1000)==expected['subtitle']
        mask_ok='whiteMaskSha256' not in expected or row.get('whiteMaskSha256')==expected['whiteMaskSha256']
        row.update(expected=expected,maskComparisonPerformed='whiteMaskSha256' in expected,assertions=dict(paused=row['state']==3,picture=bool(row.get('snapshotAvailableWhilePaused')),targetColor=color_ok,subtitlePresence=subtitle_ok,subtitleMask=mask_ok))
        row['passed']=all(row['assertions'].values())
    result['observations'].append(row);write('result.json',result)
    print(cfg['name'],name,row['state'],row.get('rgb'),row.get('brightPixels'),row.get('passed'),flush=True)
    return row
try:
    assert play(player)==0
    result['initialPauseReached']=wait(lambda:state(player)==3,10);time.sleep(.6)
    result['requestedSubtitleLanguageSelected']=any(t['language']==cfg['language'] and t['selected'] for t in tracks())
    assert result['initialPauseReached'] and result['requestedSubtitleLanguageSelected']
    observe('initial-paused')
    for index,step in enumerate(cfg['steps']):
        before=stats();started=time.monotonic();ret=seek(player,int(step['target']*1_000_000),False);assert ret==0
        time.sleep(.9)
        row=observe('seek-'+str(index)+'-'+step['name'],step)
        row['seekToSnapshotSecondsIncludingFixedSettle']=time.monotonic()-started
        row['statsBeforeSeek']=before
        row['reportedReadBytesDelta']=row['stats']['readBytes']-before['readBytes']
        held=[dict(state=state(player),timeUs=getTime(player))]
        for unused in range(3):
            time.sleep(.1);held.append(dict(state=state(player),timeUs=getTime(player)))
        row['pausedSamples']=held;row['assertions']['frozenPausedClock']=all(r['state']==3 for r in held) and len({r['timeUs'] for r in held})==1
        row['passed']=all(row['assertions'].values());write('result.json',result)
    pause(player,0)
    final=cfg['steps'][-1]
    result['resumeAdvancedFromRequestedTarget']=wait(lambda:state(player)==2 and getTime(player)>int((final['target']+.2)*1_000_000),8)
    pause(player,1);result['repaused']=wait(lambda:state(player)==3,4);time.sleep(.3)
    observe('after-resume-at-final-target',final)
finally:
    result['stopReturn']=stop(player);result['stopReached']=wait(lambda:state(player)==4,10);write('result.json',result)
    playerRelease(player);mediaRelease(media);release(instance);result['teardownComplete']=True
lines=(out/'native.log').read_text(encoding='utf-8',errors='replace').splitlines()
result.update(diagnostics=[l for l in lines if re.search(r'\berror:|Assertion failed|Binding loop|ReferenceError|TypeError',l)],warnings=[l for l in lines if 'warning:' in l],actualWasapiSelected=any('using aout stream module "wasapi"' in l for l in lines))
result['passed']=all(r.get('passed',True) for r in result['observations']) and result['resumeAdvancedFromRequestedTarget'] and result['repaused'] and result['stopReached'] and result['teardownComplete']
write('result.json',result);assert result['stopReached'] and result['teardownComplete']
print(cfg['name'],'normal terminal','behavior passed',result['passed'],flush=True)
