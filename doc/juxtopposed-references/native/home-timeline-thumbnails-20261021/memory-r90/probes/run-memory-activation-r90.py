from pathlib import Path
from hashlib import sha256
import ctypes as C
from ctypes import wintypes as W
import json,os,re,shutil,socket,subprocess,sys,time
from resource_metrics_r45 import Gpu,nvidia_sample,snapshot,k

repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment,regenerate_cache
def digest(p):return sha256(p.read_bytes()).hexdigest()
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):
    temp=p.with_suffix('.pending.json');temp.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n');os.replace(temp,p)
case=sys.argv[1]
assert case in ['forward','dedup']
inputs=read(work/'inputs-resource-r67.json')
assert all(digest(repo/p)==h for p,h in inputs.items())
inputs[Path(__file__).relative_to(repo).as_posix()]=digest(Path(__file__))
for name in ['object-observer-r90.cpp','uia-request-filter-r90.hpp','activation-diagnostic-r90.hpp','object-observer-r90.mk','build-object-r90.sh']:
    inputs[(work/name).relative_to(repo).as_posix()]=digest(work/name)
write(work/('inputs-memory-activation-'+case+'-r90.json'),inputs)
out=work/('private-memory-activation-'+case+'-r90');out.mkdir();runtime=out/'runtime'
stage_runtime(repo/'build-windows/beta-precise-seek-20261021/ordinary-r8/runtime',runtime)
for name,path in [('libvlccore.dll',work/'native-core-r49/libvlccore.dll'),('plugins/gui/libqt_plugin.dll',work/'native-object-r90/libqt_plugin.dll')]:
    target=runtime/name;target.unlink();shutil.copyfile(path,target)
gui=(runtime/'plugins/gui/libqt_plugin.dll').read_bytes()
assert all(s not in gui for s in [b'VLC_OWNED_HOVER_TRACE',b'VLC_OWNED_UI_REPORT',b'VLC_OWNED_THUMBNAIL_MODE'])
env=environment(runtime,out);env.pop('VLC_PLUGIN_PATH',None);env.pop('QT_D3D_MAX_FRAME_LATENCY',None)
env['VLC_OWNED_OBJECT_TRACE']=str(out/'objects.jsonl')
env['VLC_OWNED_DEDUP_ACTIVATION']='1' if case=='dedup' else '0'
env.update(QT_LOGGING_RULES='qt.rhi.general=true;qt.quick.handler.hover.debug=true;qt.quick.focus.debug=true;qt.qml.gc.statistics.debug=true',QT_FORCE_STDERR_LOGGING='1',QT_MESSAGE_PATTERN='OWNED_QT_DIAG|%{type}|%{category}|%{time process}|%{message}')
config=out/'vlcrc';config.write_text('metadata-network-access=0\nml-network-discovery=0\none-instance=0\nqt-updates-notif=0\nqt-auto-raise=0\nqt-remember-window-geometry=0\nqt-controls-auto-hide=0\nqt-timeline-thumbnails=0\n',encoding='utf-8',newline='\n')
qt=runtime/'portable/vlc/vlc-qt-interface.ini';qt.parent.mkdir(parents=True,exist_ok=True)
qt.write_text('[General]\ngraphics-api=4\n[MainWindow]\ncolor-scheme-index=2\ninterface-scale=1\n',encoding='utf-8',newline='\n')
regenerate_cache(runtime,out/'cache')
with socket.socket() as s:s.bind(('127.0.0.1',0));port=s.getsockname()[1]
video=work/'resource-fixtures-r45/motion-1080p50-silent.mp4'
command=[str(runtime/'vlc.exe'),'--intf=qt','--no-one-instance','--no-media-library','--no-crashdump','--no-qt-privacy-ask','--no-qt-updates-notif','--no-metadata-network-access','--no-ml-network-discovery','--config='+str(config),'--file-logging','--log-verbose=2','--verbose=2','--logfile='+str(out/'vlc.log'),'--no-qt-video-autoresize','--restore-playback-pos=0','--start-paused','--vout=direct3d11,none']
identities={p:digest(runtime/p) for p in ['vlc.exe','libvlccore.dll','plugins/gui/libqt_plugin.dll','plugins/demux/libmp4_plugin.dll']}
result=dict(status='starting',passed=False,privateFixture=True,syntheticMedia=True,forcedCleanupUsed=False,fullBetaAccepted=False,identities=identities,sourceInputsSha256=digest(work/('inputs-memory-activation-'+case+'-r90.json')),phases={})
def save():write(out/'verification.json',result)
k.GetProcessHandleCount.argtypes=[W.HANDLE,C.POINTER(W.DWORD)];k.GetProcessHandleCount.restype=W.BOOL
with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
    child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
    result.update(status='warming-before-first-observation',pid=child.pid,loopbackPort=port);save()
    print('Owned unobserved Home PID',child.pid,'starting automatically; do not use Computer Use yet',flush=True)
    try:
        time.sleep(10)
        for phase in [('quiet-home-uia-'+case)]:
            if phase=='quiet-after-observation':
                result['status']='awaiting-first-computer-use-observation';save()
                print('Unobserved phase complete; now inspect Home and release observe-after gate',flush=True)
                deadline=time.monotonic()+600
                while not (out/'observe-after').exists():
                    assert child.poll() is None and time.monotonic()<deadline,'First observation gate expired; process left available'
                    time.sleep(.1)
                time.sleep(3)
            result['status']='measuring-'+phase;save();samples=[]
            gpu=Gpu() if phase=='pdh-only' else None
            start=time.perf_counter()
            try:
                for i in range(91):
                    delay=start+i-time.perf_counter()
                    if delay>0:time.sleep(delay)
                    assert child.poll() is None,'Owned VLC exited during memory isolation'
                    row=snapshot(child._handle);row['elapsedSeconds']=time.perf_counter()-start
                    count=W.DWORD();assert k.GetProcessHandleCount(child._handle,C.byref(count))
                    row['handleCount']=count.value
                    if gpu:row['gpu']=gpu.sample(child.pid)
                    if phase=='nvidia-only':row['board']=nvidia_sample()
                    samples.append(row)
                    write(out/(phase+'.json'),dict(status='terminal' if i==90 else 'measuring',phase=phase,samples=samples))
            finally:
                if gpu:gpu.close()
            result['phases'][phase]=dict(samples=len(samples),firstPrivateBytes=samples[0]['privateBytes'],lastPrivateBytes=samples[-1]['privateBytes'],durationSeconds=samples[-1]['elapsedSeconds'])
            save();print(phase,'privateMB',round(samples[0]['privateBytes']/1e6,2),'->',round(samples[-1]['privateBytes']/1e6,2),flush=True)
        result['status']='awaiting-paused-final-inspection';save()
        print('Memory isolation complete; inspect/resume/normal Quit',flush=True)
        result['exitCode']=child.wait(timeout=600)
    except Exception as error:
        result.update(status='incomplete-process-left-running',error=str(error));save();raise
result.update(status='terminal',normalExit=result['exitCode']==0,sourceInputsUnchanged=all(digest(repo/p)==h for p,h in inputs.items()),identitiesUnchanged=all(digest(runtime/p)==h for p,h in identities.items()))
logs='\n'.join((out/name).read_text(encoding='utf-8',errors='replace') for name in ['vlc.log','stderr.txt'])
result['diagnostics']=[line for line in logs.splitlines() if re.search(r'\berror:|OWNED_QT_DIAG\|(?:warning|critical|fatal)\||Assertion failed|Binding loop|ReferenceError|TypeError',line)]
result['passed']=bool(result['normalExit'] and result['sourceInputsUnchanged'] and result['identitiesUnchanged'] and not result['diagnostics'] and len(result['phases'])==1)
result['scope']='PRIVATE matching forward/dedup activation cases. Every WM_GETOBJECT request is forwarded. Link-time wrapper either forwards every Qt platform activation or skips repeated unchanged activation states. Counts/snapshots10/100s, no media, RC, GPU queries or debugger. Factor isolation, not an accepted production fix or complete accessibility/cost acceptance.'
save();print(json.dumps({k:result[k] for k in ['passed','normalExit','diagnostics']},indent=2))
