"""Native investigation, not a passing subtitle/startup regression."""
from pathlib import Path
from hashlib import sha256
import argparse,ctypes as C,json,os,re,time
from PySide6.QtGui import QImage
parser=argparse.ArgumentParser();parser.add_argument('--case',required=True);parser.add_argument('--runtime',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent;out=args.output;runtime=args.runtime
cases={
    'original-wasapi':(work/'media-r1/multi-language.mkv',True,True,0),
    'preserve-wasapi':(work/'media-r8/preserve.mkv',True,True,0),
    'original-noaudio':(work/'media-r1/multi-language.mkv',True,False,0),
}
mediaPath,spu,audio,threads=cases[args.case]
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
options=['--ignore-config','--no-media-library','--no-metadata-network-access','--no-ml-network-discovery','--no-osd','--no-video-title-show','--no-sub-autodetect-file','--vout=vdummy','--no-hw-dec','--audio-language=eng','--sub-language=eng','--verbose=2','--file-logging','--logfile='+str(out/'native.log')]
options+=['--aout=adummy'] if audio=='dummy' else (['--aout=mmdevice','--mmdevice-backend=wasapi'] if audio else ['--no-audio'])
if not spu:options+=['--no-spu']
if threads:options+=['--avcodec-threads='+str(threads)]
encoded=[x.encode() for x in options];instance=new(len(encoded),(C.c_char_p*len(encoded))(*encoded));assert instance
player=playerNew(instance,None,None);assert player
result=dict(case=args.case,passed=False,investigationOnly=True,diagnosticOnly=True,privateCore=True,privateCoreSha256=digest(runtime/'libvlccore.dll'),traceSourceRecordSha256=digest(work/'core-trace-r9c-source/source-record.json'),options=options,sourceManifest='build-windows/beta-tracks-subtitles-20261023/inputs-r4.json',producerSha256=digest(Path(__file__)),fixtureProducerSha256=digest(work/('generate-fixtures-r1.sh' if mediaPath.parent.name=='media-r1' else 'generate-timestamp-controls-r8.sh')),mediaSha256=digest(mediaPath),observations=[],teardownComplete=False)
def observe(name,take=True):
    entry=dict(step=name,state=state(player),timeUs=getTime(player),vouts=vouts(player),subtitleTracks=tracks())
    if take and entry['vouts']:
        picture=out/(name+'.png');started=time.monotonic();entry['snapshotReturn']=snapshot(player,0,str(picture).encode(),960,540);entry['snapshotCallSeconds']=time.monotonic()-started
        available=wait(lambda:picture.exists() and not QImage(str(picture)).isNull(),2)
        entry['snapshotAvailable']=available
        if available:
            im=QImage(str(picture)).convertToFormat(QImage.Format_RGBA8888);pixels=bytes(im.bits());bright=sum(1 for i in range(0,len(pixels),4) if pixels[i]>160 and pixels[i+1]>160 and pixels[i+2]>160)
            entry.update(snapshot=picture.name,snapshotSha256=digest(picture),width=im.width(),height=im.height(),brightPixels=bright)
    result['observations'].append(entry);write('investigation.json',result);print(args.case,name,entry['state'],entry['vouts'],entry.get('brightPixels'),flush=True)
media=mediaNew(str(mediaPath).encode());assert media
mediaOption(media,b':start-paused');setMedia(player,media);mediaRelease(media)
try:
    assert play(player)==0
    result['initialPauseReached']=wait(lambda:state(player)==3,10);time.sleep(1)
    observe('initial-paused')
    pause(player,0);result['resumeReached']=wait(lambda:state(player)==2 and getTime(player)>=1_000_000,10)
    pause(player,1);result['playedPauseReached']=wait(lambda:state(player)==3,5);time.sleep(.4)
    observe('played-paused')
    if spu:
        assert seek(player,25_000_000,False)==0;time.sleep(.9);observe('seek-middle-paused')
        assert seek(player,5_000_000,False)==0;time.sleep(.9);observe('seek-first-paused')
        available=tracks();finnish=next(t for t in available if t['language']=='fin')
        select(player,2,finnish['id'].encode());result['finnishSelected']=wait(lambda:any(t['id']==finnish['id'] and t['selected'] for t in tracks()),3);time.sleep(.5);observe('finnish-selected-paused')
        result['externalAddReturn']=slave(player,0,(work/'media-r1/manual-external.srt').as_uri().encode(),True)
        result['externalTrackAdded']=wait(lambda:len(tracks())==3,3);time.sleep(.6);observe('external-added-selected-paused')
        external=next(t for t in tracks() if t['id'] not in [v['id'] for v in available]);select(player,2,external['id'].encode());result['externalExplicitSelected']=wait(lambda:any(t['id']==external['id'] and t['selected'] for t in tracks()),3);time.sleep(.5);observe('external-explicit-selected-paused')
        pause(player,0);result['externalResumeReached']=wait(lambda:state(player)==2 and getTime(player)>5_600_000,8)
        time.sleep(.5);pause(player,1);wait(lambda:state(player)==3,4);time.sleep(.3);observe('external-after-resume')
finally:
    result['stopReturn']=stop(player);result['stopReached']=wait(lambda:state(player)==4,10)
    write('investigation.json',result)
    playerRelease(player);release(instance);result['teardownComplete']=True
lines=(out/'native.log').read_text(encoding='utf-8',errors='replace').splitlines()
result.update(status='terminal',diagnostics=[line for line in lines if re.search(r'\berror:|Assertion failed|Binding loop|ReferenceError|TypeError',line)],warnings=[line for line in lines if 'warning:' in line],actualWasapiSelected=any('using aout stream module "wasapi"' in line for line in lines),actualAudioModules=[line for line in lines if 'using audio output module' in line or 'using aout stream module' in line],actualVideoDecoderLines=[line for line in lines if 'thread mode' in line or 'allowing ' in line or 'using video decoder module' in line])
write('investigation.json',result)
assert result['stopReached'] and result['teardownComplete']
print('Investigation terminal; no whole-workflow pass claimed',args.case,flush=True)
