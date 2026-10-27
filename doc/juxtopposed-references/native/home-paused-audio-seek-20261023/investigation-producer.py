from pathlib import Path
from hashlib import sha256
import argparse,ctypes as C,json,os,subprocess,sys,time
repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
def digest(p):return sha256(p.read_bytes()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
parser=argparse.ArgumentParser();parser.add_argument('--worker',action='store_true');parser.add_argument('--backend');parser.add_argument('--mode');parser.add_argument('--method');parser.add_argument('--out',type=Path);args=parser.parse_args()
if not args.worker:
    sys.path.insert(0,str(repo/'test/libvlc'))
    from native_http_runtime import stage_runtime,environment,regenerate_cache
    out=work/'initial-audio-r20c';assert not out.exists();out.mkdir()
    runtime=out/'runtime';base=work/'ordinary-ui-r19/runtime';stage_runtime(base,runtime);regenerate_cache(runtime,out/'cache')
    names=['libvlc.dll','libvlccore.dll','plugins/audio_output/libwasapi_plugin.dll','plugins/audio_output/libmmdevice_plugin.dll']
    hashes={p:digest(runtime/p) for p in names}
    result=dict(status='running',purpose='Investigate actual paused audio time/position mismatch; no production change or full acceptance.',runtimeSha256=hashes,producerSha256=digest(Path(__file__)),mediaSha256=digest(work/'ordinary-ui-r19/media/Cedar.wav'),cases=[],forcedCleanupUsed=False)
    for backend in ['dummy','mmdevice']:
        for mode,method in [(mode,method) for mode in ['initial-paused','played-then-paused'] for method in ['position','time']]:
            case=out/(backend+'-'+mode+'-'+method);case.mkdir();env=environment(runtime,case)
            env['VLC_LIBEXEC_PATH']=str(runtime/'vlc-preparser.exe')
            command=[sys.executable,str(Path(__file__)), '--worker','--backend',backend,'--mode',mode,'--method',method,'--out',str(case)]
            with (case/'stdout.txt').open('wb') as stdout,(case/'stderr.txt').open('wb') as stderr:
                child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
                result['liveWorker']=dict(pid=child.pid,backend=backend,mode=mode,method=method);write(out/'verification.json',result)
                try:code=child.wait(timeout=35)
                except subprocess.TimeoutExpired:
                    result['status']='observation-timeout-process-left-running';write(out/'verification.json',result);raise SystemExit('Inspect the saved worker PID before any restart')
            record=json.loads((case/'result.json').read_text(encoding='utf-8-sig')) if (case/'result.json').exists() else dict(error='No result')
            result['cases'].append(dict(backend=backend,mode=mode,method=method,exitCode=code,result=record))
            print(json.dumps(dict(backend=backend,mode=mode,method=method,exitCode=code,settled=record.get('seekSamples',[])[-1:],resumed=record.get('resumed'))),flush=True)
            assert code==0,record
    result.pop('liveWorker',None);result.update(status='terminal',runtimeUnchanged=all(digest(runtime/p)==h and digest(base/p)==h for p,h in hashes.items()))
    write(out/'verification.json',result);print('Native investigation finished normally.',flush=True);raise SystemExit(0)

runtime=args.out.parent/'runtime';dll_directory=os.add_dll_directory(str(runtime));vlc=C.CDLL(str(runtime/'libvlc.dll'))
def bind(name,restype,*argtypes):
    fn=getattr(vlc,name);fn.restype=restype;fn.argtypes=argtypes;return fn
ptr=C.c_void_p
new=bind('libvlc_new',ptr,C.c_int,C.POINTER(C.c_char_p));release=bind('libvlc_release',None,ptr)
media_new=bind('libvlc_media_new_path',ptr,C.c_char_p);media_release=bind('libvlc_media_release',None,ptr)
media_option=bind('libvlc_media_add_option',None,ptr,C.c_char_p)
player_new=bind('libvlc_media_player_new',ptr,ptr,ptr,ptr);player_release=bind('libvlc_media_player_release',None,ptr)
set_media=bind('libvlc_media_player_set_media',None,ptr,ptr);play=bind('libvlc_media_player_play',C.c_int,ptr)
pause=bind('libvlc_media_player_set_pause',None,ptr,C.c_int);stop=bind('libvlc_media_player_stop_async',C.c_int,ptr)
state=bind('libvlc_media_player_get_state',C.c_int,ptr);get_time=bind('libvlc_media_player_get_time',C.c_int64,ptr)
get_position=bind('libvlc_media_player_get_position',C.c_double,ptr);get_length=bind('libvlc_media_player_get_length',C.c_int64,ptr)
seekable=bind('libvlc_media_player_is_seekable',C.c_int,ptr);seek=bind('libvlc_media_player_set_position',C.c_int,ptr,C.c_double,C.c_bool)
seek_time=bind('libvlc_media_player_set_time',C.c_int,ptr,C.c_int64,C.c_bool)
options=['--ignore-config','--no-media-library','--no-metadata-network-access','--no-ml-network-discovery','--no-video','--no-osd','--no-spu','--aout='+args.backend,'--mmdevice-backend=wasapi,none','--verbose=2','--file-logging','--logfile='+str(args.out/'native.log')]
if args.mode=='initial-paused':options.append('--start-paused')
encoded=[x.encode() for x in options];instance=new(len(encoded),(C.c_char_p*len(encoded))(*encoded));assert instance
player=player_new(instance,None,None);assert player
media=media_new(str(work/'ordinary-ui-r19/media/Cedar.wav').encode());assert media
if args.mode=='initial-paused':media_option(media,b':start-paused')
set_media(player,media);media_release(media)
record=dict(backend=args.backend,mode=args.mode,method=args.method,seekSamples=[])
def save():write(args.out/'result.json',record)
def sample():return dict(state=state(player),timeUs=get_time(player),position=get_position(player),lengthUs=get_length(player),seekable=bool(seekable(player)))
def wait(predicate,label,timeout=8):
    deadline=time.monotonic()+timeout
    while time.monotonic()<deadline:
        if predicate():return
        time.sleep(.02)
    raise AssertionError(label+': '+str(sample()))
try:
    assert play(player)==0
    if args.mode=='played-then-paused':
        wait(lambda:state(player)==2 and get_time(player)>300000,'First playback');pause(player,1)
    wait(lambda:state(player)==3 and get_length(player)>0 and seekable(player),'Paused ready')
    record['baseline']=sample();save()
    assert (seek(player,.3,False) if args.method=='position' else seek_time(player,9000000,False))==0
    started=time.monotonic()
    for interval in [0,.2,.5,1,2,4]:
        delay=started+interval-time.monotonic()
        if delay>0:time.sleep(delay)
        record['seekSamples'].append(dict(atSeconds=interval,**sample()));save()
    record['pausedSeekTimeConsistent']=all(abs(x['timeUs']-9000000)<250000 for x in record['seekSamples'][1:])
    pause(player,0);wait(lambda:state(player)==2,'Resumed state');time.sleep(.5)
    record['resumed']=sample()
    record['resumedAtTarget']=8900000<record['resumed']['timeUs']<11000000
    assert stop(player)==0;wait(lambda:state(player)==4,'Stopped')
    record['stopped']=sample();record['normalStop']=True;save()
finally:
    stop(player);player_release(player);release(instance)
record['releasedNormally']=True;save()
