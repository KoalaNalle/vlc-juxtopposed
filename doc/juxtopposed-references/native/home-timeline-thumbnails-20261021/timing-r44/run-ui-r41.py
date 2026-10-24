from pathlib import Path
from hashlib import sha256
import argparse,json,re,shutil,subprocess,sys
repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment,regenerate_cache
def digest(path):return sha256(path.read_bytes()).hexdigest()
def read(path):return json.loads(path.read_text(encoding='utf-8'))
def write(path,value):path.write_text(json.dumps(value,indent=2)+'\n',encoding='utf-8')
parser=argparse.ArgumentParser();parser.add_argument('--case',choices=['edit','cold-on','cold-off','extended','geometry','cold-edge'],required=True);parser.add_argument('--seed',type=Path);args=parser.parse_args()
inputs=read(work/'inputs-r41.json')
def unchanged():return all(digest(repo/p)==h for p,h in inputs.items())
assert unchanged() and (work/'build-ui-r41-exit.txt').read_text().strip()=='0'
out=work/('ui-'+args.case+'-r41');out.mkdir();runtime=out/'runtime'
stage_runtime(repo/'build-windows/beta-precise-seek-20261021/ordinary-r8/runtime',runtime)
core=runtime/'libvlccore.dll';core.unlink();shutil.copyfile(work/'native-core-r30/libvlccore.dll',core)
gui=runtime/'plugins/gui/libqt_plugin.dll';gui.unlink();shutil.copyfile(work/'native-ui-r41/.libs/libqt_plugin.dll',gui)
env=environment(runtime,out);env.pop('VLC_PLUGIN_PATH',None)
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

video=work/'fixtures/preview-motion-1080p.mp4';audio=work/'fixtures/preview-silent.wav'
env.update(VLC_OWNED_THUMBNAIL_GEOMETRY_DIR=str(work/'geometry-fixtures-r39'),QT_QPA_PLATFORM='windows',QT_FORCE_STDERR_LOGGING='1',QT_MESSAGE_PATTERN='OWNED_QT_DIAG|%{type}|%{message}',VLC_OWNED_UI_BASE=str(out),VLC_OWNED_UI_REPORT=str(out/'journey.json'),VLC_OWNED_UI_START_GATE=str(out/'continue'),VLC_OWNED_SETTINGS_CONFIG=str(config),VLC_OWNED_THUMBNAIL_MODE=args.case,VLC_OWNED_THUMBNAIL_VIDEO=str(video),VLC_OWNED_THUMBNAIL_AUDIO=str(audio))
regenerate_cache(runtime,out/'cache')
identities={p:digest(runtime/p) for p in ['libvlccore.dll','plugins/gui/libqt_plugin.dll','plugins/demux/libmp4_plugin.dll']}
command=[str(runtime/'vlc.exe'),'--intf=qt','--no-one-instance','--no-media-library','--no-crashdump','--no-qt-privacy-ask','--no-qt-updates-notif','--no-metadata-network-access','--no-ml-network-discovery','--config='+str(config),'--file-logging','--log-verbose=2','--verbose=2','--logfile='+str(out/'vlc.log'),'--no-qt-video-autoresize','--no-hw-dec','--restore-playback-pos=0','--vout=direct3d11,none']
if args.case=='cold-edge':command+=['--start-paused',str(video)]
result=dict(case=args.case,status='starting',passed=False,privateFixture=True,fullBetaAccepted=False,forcedCleanupUsed=False,identities=identities,sourceInputsSha256=digest(work/'inputs-r41.json'),runnerSha256=digest(Path(__file__)),mediaSha256={p.name:digest(p) for p in [video,audio]})
result['geometryMediaSha256']={p.name:digest(p) for p in (work/'geometry-fixtures-r39').iterdir() if p.suffix in ['.mp4','.png']}
with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
 child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
 result.update(status='running',pid=child.pid);write(out/'verification.json',result);print('Owned thumbnail GUI awaiting inspection:',args.case,'PID',child.pid,flush=True)
 try:result['exitCode']=child.wait(timeout=600)
 except subprocess.TimeoutExpired:
  result['status']='timeout_process_left_running';write(out/'verification.json',result);raise SystemExit('Owned PID remains recorded; no forced cleanup')
result.update(status='terminal',normalExit=result['exitCode']==0,sourceInputsUnchanged=unchanged(),identitiesUnchanged=all(digest(runtime/p)==h for p,h in identities.items()))
result['journey']=read(out/'journey.json') if (out/'journey.json').exists() else {}
logs='\n'.join((out/name).read_text(encoding='utf-8',errors='replace') for name in ['vlc.log','stderr.txt'] if (out/name).exists())
terms=('ReferenceError','TypeError','Binding loop','Cannot assign','Unable to assign','unregistered datatype','is not a type','failed to load component','Assertion failed','Signal uses unnamed parameter','SyntaxError','SVGColorizer:','QFSFileEngine::open: No file name specified')
result['diagnostics']=[line for line in logs.splitlines() if re.search(r'\berror:|OWNED_QT_DIAG\|(?:warning|critical|fatal)\||qt (?:warning|critical|fatal):',line) or any(t in line for t in terms)]
result['rawDiagnostics']=list(result['diagnostics']);result['expectedDiagnostics']=[]
checks={c['name']:c['passed'] for c in result['journey'].get('checks',[])}
expected='main error: cannot replace configuration file (Windows error 5)'
if args.case=='edit' and all(checks.get(name) for name in ['owned thumbnail config replacement is blocked','failed actual toggle preserves saved policy and provider state','same actual thumbnail toggle retries after storage recovers']) and result['diagnostics'].count(expected)==1:
 result['diagnostics'].remove(expected);result['expectedDiagnostics']=[expected]
result['realD3D11Opened']='using vout display module "direct3d11"' in logs
tokens={'wait':'application quit waiting for primary playback to stop','stopped':'primary playback stopped; quitting application','teardown':'removing "interface" module "qt"'}
lines=logs.splitlines();positions={k:next((i for i,line in enumerate(lines) if token in line),None) for k,token in tokens.items()};result['quitLogOrder']=positions
result['passed']=bool(result['normalExit'] and result['sourceInputsUnchanged'] and result['identitiesUnchanged'] and result['journey'].get('passed') and not result['diagnostics'] and result['realD3D11Opened'] and all(v is not None for v in positions.values()) and positions['wait']<positions['stopped']<positions['teardown'])
result['scope']='Private actual app on repaired Core: selected geometry rendered against independent references, or extended held/edge/theme/interface-scale/fullscreen regression. Cases retain separate scopes; latency/blur/physical-DPI/ordinary/full-beta gates remain open.'
write(out/'verification.json',result);print(json.dumps({k:result[k] for k in ['case','passed','exitCode','diagnostics']},indent=2));assert result['passed']
