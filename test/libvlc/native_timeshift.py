#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Check native pause-buffer writes, rollover, data continuity and cleanup on an owned live source."""
from pathlib import Path
from hashlib import sha256
import argparse,ctypes as C,http.server,json,os,re,struct,threading,time,traceback
parser=argparse.ArgumentParser()
parser.add_argument('--runtime',type=Path,required=True)
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--chunk-bytes',type=int,choices=[1048576,2097152],required=True)
args=parser.parse_args();runtime=args.runtime.resolve();out=args.output.resolve()
if os.name!='nt' or C.sizeof(C.c_void_p)!=8:
    raise SystemExit('Use Windows x64 Python with the matching LibVLC 4 runtime')
assert out.is_dir() and not any(out.iterdir())
userdata=out/'userdata';userdata.mkdir()
os.environ.pop('VLC_PLUGIN_PATH',None)
os.environ.update(VLC_LIB_PATH=str(runtime/'plugins'),
    VLC_LIBEXEC_PATH=str(runtime/'vlc-preparser.exe'),VLC_USERDATA_PATH=str(userdata))
buffer=out/'chosen buffer \u00e4';buffer.mkdir()
stop_server=threading.Event();sent={'bytes':0,'blocks':0,'requests':[]};handler_threads=[]
rate,channels,blockframes,totalblocks=48000,8,4800,240
class Handler(http.server.BaseHTTPRequestHandler):
    protocol_version='HTTP/1.1'
    def log_message(self,*_):pass
    def do_GET(self):
        handler_threads.append(threading.current_thread())
        sent['requests'].append(self.path)
        if self.path!='/owned-live.pcm':self.send_error(404);return
        self.send_response(200);self.send_header('Content-Type','application/octet-stream')
        self.send_header('Connection','close');self.end_headers()
        start=time.monotonic()
        try:
            for index in range(totalblocks):
                if stop_server.wait(max(0,start+index*.1-time.monotonic())):break
                block=struct.pack('<h',1000+index*90)*(blockframes*channels)
                self.wfile.write(block);self.wfile.flush()
                sent['blocks']+=1;sent['bytes']+=len(block)
        except (BrokenPipeError,ConnectionResetError,ConnectionAbortedError,OSError):pass
        self.close_connection=True
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler);server.daemon_threads=True
server_thread=threading.Thread(target=server.serve_forever,daemon=True);server_thread.start()
report=dict(passed=False,chunkBytes=args.chunk_bytes,server=sent,observations=[],checks={},fullBetaAccepted=False,
    runnerSha256=sha256(Path(__file__).read_bytes()).hexdigest(),isolatedUserdata=True,
    scope='Owned paced continuous HTTP/raw-PCM stream with explicit native format, actual LibVLC pause buffering, chunk files, resume callbacks and cleanup; no GUI/hardware claim')
write=lambda:(out/'verification.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n')
def wait(predicate,seconds):
    until=time.monotonic()+seconds
    while time.monotonic()<until:
        if predicate():return True
        time.sleep(.02)
    return bool(predicate())
def files():return {p.name:p.stat().st_size for p in buffer.iterdir() if p.is_file()}
handle=os.add_dll_directory(str(runtime));dll=C.CDLL(str(runtime/'libvlc.dll'))
def bind(name,result,*arguments):
    fn=getattr(dll,name);fn.restype=result;fn.argtypes=arguments;return fn
ptr=C.c_void_p
new=bind('libvlc_new',ptr,C.c_int,C.POINTER(C.c_char_p));release=bind('libvlc_release',None,ptr)
player_new=bind('libvlc_media_player_new',ptr,ptr,ptr,ptr);player_release=bind('libvlc_media_player_release',None,ptr)
media_new=bind('libvlc_media_new_location',ptr,C.c_char_p);media_release=bind('libvlc_media_release',None,ptr)
set_media=bind('libvlc_media_player_set_media',None,ptr,ptr);play=bind('libvlc_media_player_play',C.c_int,ptr)
pause=bind('libvlc_media_player_set_pause',None,ptr,C.c_int);stop=bind('libvlc_media_player_stop_async',C.c_int,ptr)
state=bind('libvlc_media_player_get_state',C.c_int,ptr);clock=bind('libvlc_media_player_get_time',C.c_int64,ptr)
can_pause=bind('libvlc_media_player_can_pause',C.c_int,ptr)
AUDIO=C.CFUNCTYPE(None,ptr,ptr,C.c_uint,C.c_int64)
audio_callbacks=bind('libvlc_audio_set_callbacks',None,ptr,AUDIO,ptr,ptr,ptr,ptr,ptr)
audio_format=bind('libvlc_audio_set_format',None,ptr,C.c_char_p,C.c_uint,C.c_uint)
pcm={'callbacks':0,'frames':0,'lastPts':None,'firstPts':None,'bytes':0}
@AUDIO
def audio(_opaque,samples,count,pts):
    pcm['callbacks']+=1;pcm['frames']+=count;pcm['lastPts']=pts;pcm['bytes']+=count*channels*2
    if pcm['firstPts'] is None:pcm['firstPts']=pts
    if len(report['observations'])<600:
        report['observations'].append(dict(event='audio',wall=time.monotonic(),frames=count,pts=pts,
            firstSample=C.cast(samples,C.POINTER(C.c_int16))[0]))
instance=player=media=None
try:
    options=['--ignore-config','--no-media-library','--no-auto-preparse','--no-video','--no-spu',
        '--no-sub-autodetect-file','--aout=dummy','--demux=rawaud','--rawaud-fourcc=s16l','--rawaud-channels=8','--rawaud-samplerate=48000','--http-continuous','--network-caching=200',
        '--input-timeshift-path='+str(buffer),'--input-timeshift-granularity='+str(args.chunk_bytes),
        '--verbose=2','--file-logging','--logfile='+str(out/'native.log')]
    encoded=[s.encode('utf-8') for s in options]
    instance=new(len(encoded),(C.c_char_p*len(encoded))(*encoded));assert instance
    player=player_new(instance,None,None);assert player
    audio_callbacks(player,audio,None,None,None,None,None);audio_format(player,b'S16N',rate,channels)
    media=media_new(('http://127.0.0.1:'+str(server.server_address[1])+'/owned-live.pcm').encode());assert media
    set_media(player,media);media_release(media);media=None
    assert play(player)==0
    assert wait(lambda:pcm['callbacks']>=8 and clock(player)>500000,8),'No initial live PCM and advancing clock'
    report['checks']['nativePauseAvailable']=bool(can_pause(player))
    pause(player,1);assert wait(lambda:state(player)==3,3),'Pause state unavailable'
    time.sleep(.4)
    before=dict(pcm);held_clock=clock(player);sent_before=sent['bytes']
    report['observations'].append(dict(event='paused',wall=time.monotonic(),clock=held_clock,pcm=before,files=files(),sentBytes=sent_before))
    assert wait(lambda:len(files())>=3 and sent['bytes']-sent_before>=2*args.chunk_bytes,9),'Live buffering did not roll over three chunks'
    during=files();final_clock=clock(player)
    report['observations'].append(dict(event='buffered',wall=time.monotonic(),clock=final_clock,pcm=dict(pcm),files=during,sentBytes=sent['bytes']))
    report['checks'].update(pausedClockStable=final_clock==held_clock,
        pausedCallbacksStable=pcm['callbacks']==before['callbacks'],
        sourceContinued=sent['bytes']-sent_before>=2*args.chunk_bytes,
        nativeChunkRollover=len(during)>=3,
        nativeChunkBounded=all(0<=n<=args.chunk_bytes for n in during.values()),
        nativeDiskBytes=sum(during.values())>=args.chunk_bytes*1.5)
    pause(player,0);assert wait(lambda:pcm['callbacks']>before['callbacks']+8,4),'Buffered PCM did not resume'
    report['observations'].append(dict(event='resumed',wall=time.monotonic(),clock=clock(player),pcm=dict(pcm),files=files(),sentBytes=sent['bytes']))
    report['checks']['resumedClockAdvanced']=clock(player)>held_clock
    rows=[r for r in report['observations'] if r['event']=='audio']
    if len(rows)>before['callbacks']:
        old_sample=rows[before['callbacks']-1]['firstSample'];new_sample=rows[before['callbacks']]['firstSample']
        report['checks']['bufferedSamplesContinueWithoutJumpToLive']=0<=new_sample-old_sample<=180
    stop(player);assert wait(lambda:state(player)==4,5),'Native stop did not finish'
    player_release(player);player=None;release(instance);instance=None
    report['checks']['nativeFilesCleaned']=not files()
    report['pcm']=pcm
except Exception:
    report['error']=traceback.format_exc()
finally:
    if media:media_release(media)
    if player:
        stop(player);player_release(player)
    if instance:release(instance)
    stop_server.set();server.shutdown();server.server_close();server_thread.join(3)
    for thread in handler_threads:thread.join(3)
    report['serverJoined']=not server_thread.is_alive() and all(not t.is_alive() for t in handler_threads)
    report['filesAfterRelease']=files()
    log=(out/'native.log').read_text(encoding='utf-8',errors='replace') if (out/'native.log').exists() else ''
    lines=log.splitlines()
    errors=[line for line in lines if 'error:' in line]
    warnings=[line for line in lines if 'warning:' in line]
    first_pause=next((i for i,line in enumerate(lines) if 'pausing audio' in line),-1)
    unexpected=[line for i,line in enumerate(lines) if 'warning:' in line
        and not ('resetting master clock: coefficient too unstable:' in line and 0<=i<first_pause)]
    report['nativeErrors']=errors;report['nativeWarnings']=warnings
    report['scopedStartupWarnings']='Input PCR may correct the generated network burst clock before Pause; no warning is accepted during buffered resume.'
    report['checks']['nativeErrorsAbsent']=not errors
    report['checks']['unexpectedWarningsAbsent']=not unexpected
    report['passed']=bool(not report.get('error') and report['checks'] and all(report['checks'].values()) and report['serverJoined'])
    write()
print(json.dumps({k:report[k] for k in ['passed','checks','error'] if k in report}),flush=True)
raise SystemExit(0 if report['passed'] else 1)
