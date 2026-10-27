from pathlib import Path
from hashlib import sha256
import json,os,re,shutil,socket,subprocess,sys

repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment,regenerate_cache
def digest(p):return sha256(p.read_bytes()).hexdigest()
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
assert (work/'build-hover-r66-exit.txt').read_text(encoding='utf-8').strip()=='0'
inputs=read(work/'inputs-resource-r64.json')
assert all(digest(repo/p)==h for p,h in inputs.items())
for name in ['hover-observer-r66.cpp','build-hover-r66.sh','run-hover-r66.py']:
 inputs[(work/name).relative_to(repo).as_posix()]=digest(work/name)
write(work/'inputs-hover-r66.json',inputs)
out=work/'ui-hover-r66';out.mkdir();runtime=out/'runtime'
stage_runtime(repo/'build-windows/beta-precise-seek-20261021/ordinary-r8/runtime',runtime)
for name,source in [('libvlccore.dll',work/'native-core-r49/libvlccore.dll'),('plugins/gui/libqt_plugin.dll',work/'native-hover-r66/.libs/libqt_plugin.dll')]:
 target=runtime/name;target.unlink();shutil.copyfile(source,target)
env=environment(runtime,out);env.pop('VLC_PLUGIN_PATH',None);env.pop('QT_D3D_MAX_FRAME_LATENCY',None)
env.update(VLC_OWNED_HOVER_TRACE=str(out/'hover-trace.jsonl'),QT_FORCE_STDERR_LOGGING='1',QT_MESSAGE_PATTERN='OWNED_QT_DIAG|%{type}|%{message}')
config=out/'vlcrc';config.write_text('metadata-network-access=0\nml-network-discovery=0\none-instance=0\nqt-updates-notif=0\nqt-auto-raise=0\nqt-remember-window-geometry=0\nqt-controls-auto-hide=0\nqt-timeline-thumbnails=1\n',encoding='utf-8',newline='\n')
qt=runtime/'portable/vlc/vlc-qt-interface.ini';qt.parent.mkdir(parents=True,exist_ok=True)
qt.write_text('[General]\ngraphics-api=4\n[MainWindow]\ncolor-scheme-index=2\ninterface-scale=1\n',encoding='utf-8',newline='\n')
regenerate_cache(runtime,out/'cache')
with socket.socket() as s:s.bind(('127.0.0.1',0));port=s.getsockname()[1]
video=work/'resource-fixtures-r45/motion-1080p50-silent.mp4'
command=[str(runtime/'vlc.exe'),'--intf=qt','--extraintf=rc','--rc-quiet','--rc-host=127.0.0.1:'+str(port),'--no-one-instance','--no-media-library','--no-crashdump','--no-qt-privacy-ask','--no-qt-updates-notif','--no-metadata-network-access','--no-ml-network-discovery','--config='+str(config),'--file-logging','--log-verbose=2','--verbose=2','--logfile='+str(out/'vlc.log'),'--no-qt-video-autoresize','--restore-playback-pos=0','--start-paused','--vout=direct3d11,none',str(video)]
result=dict(status='starting',passed=False,privateReadOnlyObserver=True,syntheticMedia=True,fullBetaAccepted=False,forcedCleanupUsed=False,identities={p:digest(runtime/p) for p in ['vlc.exe','libvlccore.dll','plugins/gui/libqt_plugin.dll','plugins/demux/libmp4_plugin.dll']},sourceInputsSha256=digest(work/'inputs-hover-r66.json'))
with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
 child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
 result.update(status='running',pid=child.pid,loopbackPort=port);write(out/'verification.json',result)
 print('Owned read-only hover observer PID',child.pid,'loopback',port,flush=True)
 try:result['exitCode']=child.wait(timeout=1200)
 except subprocess.TimeoutExpired:
  result.update(status='inspection-expired-process-left-running');write(out/'verification.json',result);raise
result.update(status='terminal',normalExit=result['exitCode']==0,sourceInputsUnchanged=all(digest(repo/p)==h for p,h in inputs.items()),identitiesUnchanged=all(digest(runtime/p)==h for p,h in result['identities'].items()))
logs='\n'.join((out/name).read_text(encoding='utf-8',errors='replace') for name in ['vlc.log','stderr.txt'])
result['diagnostics']=[line for line in logs.splitlines() if re.search(r'\berror:|OWNED_QT_DIAG\|(?:warning|critical|fatal)\||Assertion failed|Binding loop|ReferenceError|TypeError',line)]
result['passed']=bool(result['normalExit'] and result['sourceInputsUnchanged'] and result['identitiesUnchanged'] and not result['diagnostics'])
result['scope']='Read-only own-window pointer/hover/activity observer; separate causal review required. No continuous ordinary resource acceptance.'
write(out/'verification.json',result)
print(json.dumps({k:result[k] for k in ['passed','normalExit','diagnostics']},indent=2),flush=True)
