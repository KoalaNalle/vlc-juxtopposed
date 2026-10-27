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
assert case=='fixed'
inputs=read(work/'inputs-resource-r67.json')
assert all(digest(repo/p)==h for p,h in inputs.items())
inputs[Path(__file__).relative_to(repo).as_posix()]=digest(Path(__file__))
for name in ['scrollbar-regression-r92.cpp','object-observer-r88.mk','build-scrollbar-fixed-r93.sh']:
    inputs[(work/name).relative_to(repo).as_posix()]=digest(work/name)
for name in ['modules/gui/qt/tests/native_ui/scrollbar_accessibility_journey.cpp','contrib/src/qtdeclarative/rules.mak','contrib/src/qtdeclarative/0003-ScrollBar-keep-accessibility-action-connections-unique.patch']:
    inputs[name]=digest(repo/name)
write(work/('inputs-scrollbar-fixed-'+case+'-r93.json'),inputs)
out=work/('private-scrollbar-'+case+'-r93');out.mkdir();runtime=out/'runtime'
stage_runtime(repo/'build-windows/beta-precise-seek-20261021/ordinary-r8/runtime',runtime)
for name,path in [('libvlccore.dll',work/'native-core-r49/libvlccore.dll'),('plugins/gui/libqt_plugin.dll',work/('native-scrollbar-r92' if case=='baseline' else 'native-scrollbar-fixed-r93')/'libqt_plugin.dll')]:
    target=runtime/name;target.unlink();shutil.copyfile(path,target)
gui=(runtime/'plugins/gui/libqt_plugin.dll').read_bytes()
assert all(s not in gui for s in [b'VLC_OWNED_HOVER_TRACE',b'VLC_OWNED_UI_REPORT',b'VLC_OWNED_THUMBNAIL_MODE'])
env=environment(runtime,out);env.pop('VLC_PLUGIN_PATH',None);env.pop('QT_D3D_MAX_FRAME_LATENCY',None)
env['VLC_OWNED_SCROLLBAR_REGRESSION']=str(out/'regression.json')
env.update(QT_LOGGING_RULES='qt.rhi.general=true;qt.quick.handler.hover.debug=true;qt.quick.focus.debug=true',QT_FORCE_STDERR_LOGGING='1',QT_MESSAGE_PATTERN='OWNED_QT_DIAG|%{type}|%{category}|%{time process}|%{message}')
config=out/'vlcrc';config.write_text('metadata-network-access=0\nml-network-discovery=0\none-instance=0\nqt-updates-notif=0\nqt-auto-raise=0\nqt-remember-window-geometry=0\nqt-controls-auto-hide=0\nqt-timeline-thumbnails=0\n',encoding='utf-8',newline='\n')
qt=runtime/'portable/vlc/vlc-qt-interface.ini';qt.parent.mkdir(parents=True,exist_ok=True)
qt.write_text('[General]\ngraphics-api=4\n[MainWindow]\ncolor-scheme-index=2\ninterface-scale=1\n',encoding='utf-8',newline='\n')
regenerate_cache(runtime,out/'cache')
with socket.socket() as s:s.bind(('127.0.0.1',0));port=s.getsockname()[1]
video=work/'resource-fixtures-r45/motion-1080p50-silent.mp4'
command=[str(runtime/'vlc.exe'),'--intf=qt','--no-one-instance','--no-media-library','--no-crashdump','--no-qt-privacy-ask','--no-qt-updates-notif','--no-metadata-network-access','--no-ml-network-discovery','--config='+str(config),'--file-logging','--log-verbose=2','--verbose=2','--logfile='+str(out/'vlc.log'),'--no-qt-video-autoresize','--restore-playback-pos=0','--start-paused','--vout=direct3d11,none']
identities={p:digest(runtime/p) for p in ['vlc.exe','libvlccore.dll','plugins/gui/libqt_plugin.dll','plugins/demux/libmp4_plugin.dll']}
result=dict(status='starting',passed=False,privateFixture=True,syntheticMedia=True,forcedCleanupUsed=False,fullBetaAccepted=False,identities=identities,sourceInputsSha256=digest(work/('inputs-scrollbar-fixed-'+case+'-r93.json')),phases={})
def save():write(out/'verification.json',result)
k.GetProcessHandleCount.argtypes=[W.HANDLE,C.POINTER(W.DWORD)];k.GetProcessHandleCount.restype=W.BOOL
with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
    child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
    result.update(status='warming-before-first-observation',pid=child.pid,loopbackPort=port);save()
    print('Owned unobserved Home PID',child.pid,'starting automatically; do not use Computer Use yet',flush=True)
    try:
        deadline=time.monotonic()+90
        while not (out/'regression.json').exists():
            assert child.poll() is None and time.monotonic()<deadline,'Native regression did not complete; process left available'
            time.sleep(.1)
        result['nativeRegression']=read(out/'regression.json')
        result['nativeRegressionExpectedPass']=case=='fixed'
        result['status']='awaiting-regression-final-inspection';save()
        print('Native scrollbar regression allPassed',result['nativeRegression']['allPassed'],'expected',case=='fixed','inspect Home then normal Quit',flush=True)
        result['exitCode']=child.wait(timeout=600)
    except Exception as error:
        result.update(status='incomplete-process-left-running',error=str(error));save();raise
result.update(status='terminal',normalExit=result['exitCode']==0,sourceInputsUnchanged=all(digest(repo/p)==h for p,h in inputs.items()),identitiesUnchanged=all(digest(runtime/p)==h for p,h in identities.items()))
logs='\n'.join((out/name).read_text(encoding='utf-8',errors='replace') for name in ['vlc.log','stderr.txt'])
result['diagnostics']=[line for line in logs.splitlines() if re.search(r'\berror:|OWNED_QT_DIAG\|(?:warning|critical|fatal)\||Assertion failed|Binding loop|ReferenceError|TypeError',line)]
result['passed']=bool(result['normalExit'] and result['sourceInputsUnchanged'] and result['identitiesUnchanged'] and not result['diagnostics'] and not result['nativeRegression'].get('setupError') and result['nativeRegression']['allPassed']==(case=='fixed'))
result['scope']='PRIVATE native scrollbar regression, real Qt hidden component, repeated activation/action/deactivation/reactivation. Runner passed means lifecycle and expected regression outcome, including expected baseline failure; nativeRegression.allPassed is the actual functional oracle. No production fix acceptance, resource interval, media or debugger.'
save();print(json.dumps({k:result[k] for k in ['passed','normalExit','diagnostics']},indent=2))
