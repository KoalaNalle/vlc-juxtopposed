"""Native investigation, not a passing subtitle/startup regression."""
from pathlib import Path
from hashlib import sha256
import argparse,ctypes as C,json,os,re,time,threading
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
class ProcessIo(C.Structure):
    _fields_=[(name,C.c_uint64) for name in ['readOperations','writeOperations','otherOperations','readBytes','writeBytes','otherBytes']]
assert C.sizeof(ProcessIo)==48
kernel=C.WinDLL('kernel32',use_last_error=True)
kernel.GetCurrentProcess.restype=C.c_void_p;kernel.GetCurrentProcess.argtypes=[]
kernel.GetProcessIoCounters.restype=C.c_int;kernel.GetProcessIoCounters.argtypes=[C.c_void_p,C.POINTER(ProcessIo)]
currentProcess=kernel.GetCurrentProcess()
def process_io():
    value=ProcessIo();assert kernel.GetProcessIoCounters(currentProcess,C.byref(value))
    return {name:getattr(value,name) for name,_ in ProcessIo._fields_}

options=['--ignore-config','--no-media-library','--no-metadata-network-access','--no-ml-network-discovery','--no-osd','--no-video-title-show','--no-sub-autodetect-file','--vout=vdummy','--no-hw-dec','--audio-language=eng','--sub-language=eng','--verbose=2','--file-logging','--logfile='+str(out/'native.log'),'--aout=mmdevice','--mmdevice-backend=wasapi']
encoded=[v.encode() for v in options]
instance=new(len(encoded),(C.c_char_p*len(encoded))(*encoded));assert instance
# LibVLC's public log callback copies only format strings and signals events.
# It never calls playback/stop/log-registration APIs and never blocks the input.
# Windows file-stream buffering can hide the earlier preroll lines until later.
armed=False
request_event=threading.Event();anchor_event=threading.Event();finished_event=threading.Event()
timeline=[]
LogCallback=C.CFUNCTYPE(None,C.c_void_p,C.c_int,C.c_void_p,C.c_char_p,C.c_void_p)
@LogCallback
def on_log(opaque,level,context,fmt,varargs):
    if not armed or not fmt:return
    format_string=fmt.decode('utf-8',errors='replace')
    if format_string.startswith('seek request to i_pos = '):
        timeline.append(dict(kind='request',at=time.monotonic(),format=format_string));request_event.set()
    elif request_event.is_set() and format_string.startswith('seek: preroll{ track: '):
        timeline.append(dict(kind='anchor',at=time.monotonic(),format=format_string));anchor_event.set()
    elif request_event.is_set() and format_string.startswith('seek: preroll{ req: '):
        timeline.append(dict(kind='finished',at=time.monotonic(),format=format_string));finished_event.set()
set_log=bind('libvlc_log_set',None,C.c_void_p,LogCallback,C.c_void_p)
set_log(instance,on_log,None)
player=playerNew(instance,None,None);assert player
media=mediaNew(str(mediaPath).encode());assert media
mediaOption(media,b':start-paused');setMedia(player,media)
result=dict(passed=False,candidateOnly=True,repairAccepted=False,configuration=cfg,
    producerSha256=digest(Path(__file__)),fixtureSha256=digest(mediaPath),
    coreSha256=digest(runtime/'libvlccore.dll'),mkvSha256=digest(runtime/'plugins/demux/libmkv_plugin.dll'),
    options=options,teardownComplete=False,indexCancellationIsolated=False,
    scope='Normal Stop request during a pending precise seek, not forced termination. Index-phase qualification uses a non-blocking public LibVLC log callback: request and AV anchors, increased own-process reads, and no final seek/preroll callback even after teardown. Own-process reads include probe/log activity and filesystem caches.')
log=out/'native.log'
def logtext():
    return log.read_text(encoding='utf-8',errors='replace') if log.exists() else ''
finished_marker='seek: preroll{ req: 55000001,'
anchor='seek: preroll{ track: 1, pts: 55000000,'
try:
    assert play(player)==0
    result['initialPauseReached']=wait(lambda:state(player)==3,10)
    assert result['initialPauseReached']
    time.sleep(.5)
    result['selectedEnglish']=any(t['language']=='eng' and t['selected'] for t in tracks())
    assert result['selectedEnglish'] and vouts(player)>0
    before_io=process_io();started=time.monotonic();armed=True;result['seekReturn']=seek(player,55_000_000,False)
    assert result['seekReturn']==0
    end=time.monotonic()+10
    trigger=False
    while time.monotonic()<end:
        now_io=process_io()
        anchor_seen=anchor_event.is_set()
        finished_seen=finished_event.is_set()
        delta=now_io['readBytes']-before_io['readBytes']
        if anchor_seen and not finished_seen and delta>2_000_000:
            trigger=True
            break
        if finished_seen:
            break
        time.sleep(.001)
    # Keep this observation in memory until after Stop to minimize the interval
    # between the observed stage and the ordinary application stop request.
    observation=dict(elapsedSeconds=time.monotonic()-started,anchorSeen=anchor_seen,
        finishedSeen=finished_seen,readBytesIncludingProbe=delta,triggered=trigger)
    stop_started=time.monotonic();result['stopReturn']=stop(player)
    result['stopReached']=wait(lambda:state(player)==4,10)
    result['stopToStoppedSeconds']=time.monotonic()-stop_started
    result['preStopObservation']=observation
    write('result.json',result)
finally:
    if not result.get('stopReached'):
        result['cleanupStopReturn']=stop(player)
        result['stopReached']=wait(lambda:state(player)==4,10)
    write('result.json',result)
    playerRelease(player);mediaRelease(media);release(instance)
    result['teardownComplete']=True
text=logtext()
result['finalSeekMarkerPresent']=finished_event.is_set()
result['callbackTimeline']=timeline
result['fileSeekMarkerPresent']=finished_marker in text
result['indexCancellationIsolated']=bool(result.get('preStopObservation',{}).get('triggered') and not result['finalSeekMarkerPresent'])
result['normalStopPassed']=result['stopReturn']==0 and result['stopReached'] and result['teardownComplete']
result['passed']=result['normalStopPassed'] and result['indexCancellationIsolated']
result['diagnostics']=[l for l in text.splitlines() if re.search(r'\berror:|Assertion failed',l)]
write('result.json',result)
print(json.dumps({k:result[k] for k in ['passed','normalStopPassed','indexCancellationIsolated','preStopObservation','stopToStoppedSeconds','finalSeekMarkerPresent','teardownComplete']}),flush=True)
assert result['normalStopPassed']
