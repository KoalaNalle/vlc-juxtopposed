from pathlib import Path
from hashlib import sha256
import ctypes,json,os,re,shutil,subprocess,sys,wave
repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment,regenerate_cache
def digest(p):return sha256(p.read_bytes()).hexdigest()
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
inputs=read(work/'inputs-r17.json')
assert all(digest(repo/p)==h for p,h in inputs.items())
out=work/'ordinary-ui-r18';assert not out.exists();out.mkdir();runtime=out/'runtime'
base=repo/'build-windows/beta-media-role-20261022/ui-r4/runtime'
names=['vlc.exe','libvlc.dll','libvlccore.dll','plugins/gui/libqt_plugin.dll','plugins/audio_output/libwasapi_plugin.dll']
protected={p:digest(base/p) for p in names};stage_runtime(base,runtime)
qt=runtime/'plugins/gui/libqt_plugin.dll';qt.unlink();shutil.copyfile(work/'ordinary-r17/libqt_plugin.dll',qt)
identities={p:digest(runtime/p) for p in names}
assert all(flag not in qt.read_bytes() for flag in [b'VLC_OWNED_HOVER_TRACE',b'VLC_OWNED_UI_REPORT',b'VLC_OWNED_THUMBNAIL_MODE',b'VLC_OWNED_OBJECT_TRACE',b'VLC_OWNED_DEDUP_ACTIVATION',b'VLC_OWNED_BLOCK_UIA',b'VLC_OWNED_SCROLLBAR_REGRESSION',b'VLC_OWNED_QUEUE_FOCUS_REPORT',b'VLC_OWNED_QUEUE_DRAG_REPORT'])
env=environment(runtime,out);env.pop('VLC_PLUGIN_PATH',None);env.pop('QT_D3D_MAX_FRAME_LATENCY',None)
regenerate_cache(runtime,out/'cache')
media=out/'media';media.mkdir()
with wave.open(str(media/'Cedar.wav'),'wb') as stream:
    stream.setnchannels(2);stream.setsampwidth(2);stream.setframerate(48000);stream.writeframes(b'\0'*4*48000*30)
for name in ['Birch','Aspen','Dune','Echo','Fir']:os.link(media/'Cedar.wav',media/(name+'.wav'))
config=out/'vlcrc';config.write_text('metadata-network-access=0\nml-network-discovery=0\none-instance=0\nqt-updates-notif=0\nqt-auto-raise=0\nqt-remember-window-geometry=0\nqt-controls-auto-hide=0\nqt-timeline-thumbnails=0\n',encoding='utf-8',newline='\n')
ini=runtime/'portable/vlc/vlc-qt-interface.ini';ini.parent.mkdir(parents=True,exist_ok=True)
ini.write_text('[General]\ngraphics-api=4\nfiledialog-path='+media.as_uri()+'\n[MainWindow]\ncolor-scheme-index=2\ninterface-scale=1\nplayqueue-panel-docked=true\n',encoding='utf-8',newline='\n')
(out/'config-before.txt').write_bytes(config.read_bytes())
command=[str(runtime/'vlc.exe'),'--intf=qt','--no-one-instance','--no-media-library','--no-crashdump','--no-qt-privacy-ask','--no-qt-updates-notif','--no-metadata-network-access','--no-ml-network-discovery','--config='+str(config),'--file-logging','--log-verbose=2','--verbose=2','--logfile='+str(out/'vlc.log'),'--restore-playback-pos=0','--start-paused','--start-time=7',*[str(media/(n+'.wav')) for n in ['Cedar','Birch']]]
result=dict(ordinaryGuiSourceManifestSha256=digest(work/'inputs-r17.json'),testOnlySourceDelta=False,status='starting',passed=False,observationsRequired=True,privateGui=False,forcedCleanupUsed=False,fullBetaAccepted=False,
    sourceManifest='build-windows/beta-queue-workflow-20261022/inputs-r17.json',runtimeSha256=identities,initialOrder=['Cedar','Birch'],startTimeSeconds=7,producerSha256=digest(Path(__file__)),mediaSha256=digest(media/'Cedar.wav'))
def save():write(out/'verification.json',result)
k=ctypes.WinDLL('kernel32',use_last_error=True);k.SetErrorMode.argtypes=[ctypes.c_uint];k.SetErrorMode.restype=ctypes.c_uint;previous=k.SetErrorMode(0x8003)
try:
    with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
        child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
        result.update(status='running',pid=child.pid);save();print('Owned queue workflow PID',child.pid,flush=True)
        try:result['exitCode']=child.wait(timeout=1800)
        except subprocess.TimeoutExpired:
            result.update(status='observation-timeout-process-left-running');save();raise SystemExit('Inspect exact saved PID; no automatic restart/cleanup')
finally:k.SetErrorMode(previous)
logs='\n'.join((out/p).read_text(encoding='utf-8',errors='replace') for p in ['vlc.log','stderr.txt'])
diagnostics=[line for line in logs.splitlines() if re.search(r'\berror:|Assertion failed|Binding loop|ReferenceError|TypeError',line)]
result.update(status='terminal',normalExit=result['exitCode']==0,diagnostics=diagnostics,
    sourceInputsUnchanged=all(digest(repo/p)==h for p,h in inputs.items()),
    runtimeUnchanged=all(digest(base/p)==h and digest(runtime/p)==identities[p] for p,h in protected.items()))
(out/'config-after.txt').write_bytes(config.read_bytes());save();print(json.dumps(result,indent=2))
