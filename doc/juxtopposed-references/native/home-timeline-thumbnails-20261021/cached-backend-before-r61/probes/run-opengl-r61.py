from pathlib import Path
from hashlib import sha256
import argparse,json,re,shutil,subprocess,sys
repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment,regenerate_cache
def digest(path):return sha256(path.read_bytes()).hexdigest()
def read(path):return json.loads(path.read_text(encoding='utf-8'))
def write(path,value):path.write_text(json.dumps(value,indent=2)+'\n',encoding='utf-8')
parser=argparse.ArgumentParser();parser.add_argument('--case',choices=['edit','cold-on','cold-off','extended','geometry','cold-edge'],required=True);parser.add_argument('--seed',type=Path);parser.add_argument('--inline-fallback',action='store_true');parser.add_argument('--latency',choices=['default','explicit-2','explicit-empty','explicit-0'],default='default');parser.add_argument('--backend',choices=['dcomp-d3d11','win7-d3d11','opengl'],required=True);parser.add_argument('--old-gui',action='store_true');args=parser.parse_args();assert args.case=='cold-edge' and args.backend=='opengl' and args.latency=='default'
inputs=read(work/'inputs-opengl-r61.json')
def unchanged():return all(digest(repo/p)==h for p,h in inputs.items())
assert unchanged() and (work/'build-ui-r56-exit.txt').read_text().strip()=='0'
out=work/('ui-debug-'+('old' if args.old_gui else 'new')+'-opengl-r61');out.mkdir();runtime=out/'runtime'
stage_runtime(repo/'build-windows/beta-precise-seek-20261021/ordinary-r8/runtime',runtime)
core=runtime/'libvlccore.dll';core.unlink();shutil.copyfile(work/'native-core-r49/libvlccore.dll',core)
gui=runtime/'plugins/gui/libqt_plugin.dll';gui.unlink();shutil.copyfile(work/('native-ui-r42/.libs/libqt_plugin.dll' if args.old_gui else 'native-ui-r56/.libs/libqt_plugin.dll'),gui)
env=environment(runtime,out);env.pop('VLC_PLUGIN_PATH',None);env.pop('QT_D3D_MAX_FRAME_LATENCY',None)
if args.latency!='default':env['QT_D3D_MAX_FRAME_LATENCY']={'explicit-2':'2','explicit-empty':'','explicit-0':'0'}[args.latency]
config=out/'vlcrc'
if args.seed:
 assert args.case.startswith('cold');shutil.copyfile(args.seed/'vlcrc',config)
 qt=runtime/'portable/vlc/vlc-qt-interface.ini';qt.parent.mkdir(parents=True)
 shutil.copyfile(args.seed/'runtime/portable/vlc/vlc-qt-interface.ini',qt)
else:
 assert args.case in ['edit','extended','geometry','cold-edge']
 config.write_text('metadata-network-access=0\nml-network-discovery=0\none-instance=0\nqt-updates-notif=0\nqt-auto-raise=0\nqt-remember-window-geometry=0\nqt-controls-auto-hide=0\nqt-timeline-thumbnails=0\n',encoding='utf-8',newline='\n')

if args.case=='cold-edge':
 config.write_text(config.read_text().replace('qt-timeline-thumbnails=0','qt-timeline-thumbnails=1'),encoding='utf-8')
 qt=runtime/'portable/vlc/vlc-qt-interface.ini';qt.parent.mkdir(parents=True,exist_ok=True)
 qt.write_text('[General]\ngraphics-api=4\n[MainWindow]\ncolor-scheme-index=1\ninterface-scale=1.25\n',encoding='utf-8')

qt=runtime/'portable/vlc/vlc-qt-interface.ini';qt.parent.mkdir(parents=True,exist_ok=True)
qt.write_text('[General]\ngraphics-api='+('3' if args.backend=='opengl' else '4')+'\n[MainWindow]\ncolor-scheme-index=1\ninterface-scale=1.25\n',encoding='utf-8')

video=work/'fixtures/preview-motion-1080p.mp4';audio=work/'fixtures/preview-silent.wav'
env.update(VLC_OWNED_THUMBNAIL_GEOMETRY_DIR=str(work/'geometry-fixtures-r39'),QT_QPA_PLATFORM='windows',QT_FORCE_STDERR_LOGGING='1',QT_MESSAGE_PATTERN='OWNED_QT_DIAG|%{type}|%{message}',VLC_OWNED_UI_BASE=str(out),VLC_OWNED_UI_REPORT=str(out/'journey.json'),VLC_OWNED_UI_START_GATE=str(out/'continue'),VLC_OWNED_SETTINGS_CONFIG=str(config),VLC_OWNED_THUMBNAIL_MODE=args.case,VLC_OWNED_THUMBNAIL_VIDEO=str(video),VLC_OWNED_THUMBNAIL_AUDIO=str(audio))
assert not args.inline_fallback
env.update(QSG_RENDER_TIMING='1',QSG_INFO='1',QT_LOGGING_RULES='qt.scenegraph.time.renderloop=true;qt.scenegraph.renderloop=true;qt.rhi.general=true')
regenerate_cache(runtime,out/'cache')
identities={p:digest(runtime/p) for p in ['libvlccore.dll','plugins/gui/libqt_plugin.dll','plugins/demux/libmp4_plugin.dll']}
command=[str(runtime/'vlc.exe'),'--intf=qt','--no-one-instance','--no-media-library','--no-crashdump','--no-qt-privacy-ask','--no-qt-updates-notif','--no-metadata-network-access','--no-ml-network-discovery','--config='+str(config),'--file-logging','--log-verbose=2','--verbose=2','--logfile='+str(out/'vlc.log'),'--no-qt-video-autoresize','--no-hw-dec','--restore-playback-pos=0','--vout=direct3d11,none']
command+=['--qt-compositor='+('dcomp' if args.backend=='dcomp-d3d11' else 'win7')]
if args.case=='cold-edge':command+=['--start-paused',str(video)]
result=dict(debuggerDiagnostic=True,oldGui=args.old_gui,selectedBackend=args.backend,latencyPolicy=args.latency,launchFrameLatency=env.get("QT_D3D_MAX_FRAME_LATENCY"),timingAcceptance=False,inlineFallback=args.inline_fallback,case=args.case,status='starting',passed=False,privateFixture=True,fullBetaAccepted=False,forcedCleanupUsed=False,identities=identities,sourceInputsSha256=digest(work/'inputs-opengl-r61.json'),runnerSha256=digest(Path(__file__)),mediaSha256={p.name:digest(p) for p in [video,audio]})
result['geometryMediaSha256']={p.name:digest(p) for p in (work/'geometry-fixtures-r39').iterdir() if p.suffix in ['.mp4','.png']}

import ctypes
symbols=out/'local-symbols';symbols.mkdir()
script=out/'debugger-commands.txt'
script.write_text('.printf "OWNED_DEBUGGEE_PID=%d\\n", @$tpid\nsxd -c2 ".echo OWNED_SECOND_CHANCE_AV; .exr -1; .ecxr; lm m libqt_plugin; ~* kb; gn" av\ng\n',encoding='utf-8')
debugger=repo/'build-windows/homecoming-20261018/tools/WinDbg/amd64/cdb.exe'
debugCommand=[str(debugger),'-G','-sins','-y',str(symbols),'-logo',str(out/'fault-stack.txt'),'-cf',str(script),*command]
result.update(status='starting-debugger',passed=False,forcedCleanupUsed=False,timingAcceptance=False)
k=ctypes.WinDLL('kernel32',use_last_error=True);k.SetErrorMode.argtypes=[ctypes.c_uint];k.SetErrorMode.restype=ctypes.c_uint
previous=k.SetErrorMode(0x8003)
try:
 with (out/'debugger-output.txt').open('wb') as transcript:
  child=subprocess.Popen(debugCommand,cwd=runtime,env=env,stdout=transcript,stderr=transcript,creationflags=subprocess.CREATE_NO_WINDOW)
  result.update(status='debugging',debuggerPid=child.pid);write(out/'verification.json',result);print('OpenGL debugger',('old' if args.old_gui else 'new'),'PID',child.pid,flush=True)
  try:result['debuggerExitCode']=child.wait(timeout=90)
  except subprocess.TimeoutExpired:
   result.update(status='debugger-deadline-owned-process-left-running');write(out/'verification.json',result);raise SystemExit('Debugger/owned app left running for inspection; no forced cleanup')
finally:k.SetErrorMode(previous)
text=(out/'fault-stack.txt').read_text(encoding='utf-8',errors='replace')
pid=re.search(r'OWNED_DEBUGGEE_PID=(\d+)',text)
result.update(status='terminal',sourceInputsUnchanged=unchanged(),identitiesUnchanged=all(digest(runtime/p)==h for p,h in identities.items()),debuggeePid=int(pid[1]) if pid else None,secondChanceAvObserved=bool(re.search(r'(?m)^OWNED_SECOND_CHANCE_AV\s*$',text)),normalAppExit=False)
exits=re.findall(r'exited with code ([0-9a-fA-F]+)',text,re.I)
result['debuggeeExitCodeFromLog']=int(exits[-1],16) if exits else None
result['scope']='Owned synthetic OpenGL startup fault diagnosis; captured second-chance exception is a retained failure, never passing application or timing evidence. Default candidate guard is not executed on this backend.'
write(out/'verification.json',result);print(json.dumps({k:result[k] for k in ['oldGui','status','debuggerExitCode','debuggeePid','secondChanceAvObserved','debuggeeExitCodeFromLog','sourceInputsUnchanged','identitiesUnchanged']},indent=2))
assert result['secondChanceAvObserved'] and result['sourceInputsUnchanged'] and result['identitiesUnchanged']
