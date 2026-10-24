from pathlib import Path
from hashlib import sha256
import argparse,json,subprocess,sys,shutil,re

repo=Path(__file__).resolve().parents[5]
parser=argparse.ArgumentParser(description='Opt-in native application-Quit regression; never the ordinary GUI.')
parser.add_argument('--mode',required=True,choices=['idle','rapid','playing','paused','repeat','restart'])
parser.add_argument('--package',type=Path,required=True)
parser.add_argument('--qa-gui',type=Path,required=True)
parser.add_argument('--inputs',type=Path,required=True)
parser.add_argument('--media',type=Path,required=True)
parser.add_argument('--expected-media',choices=['audio','video','av'],default='audio')
parser.add_argument('--output-dir',type=Path,required=True)
parser.add_argument('--fullscreen',action='store_true')
parser.add_argument('--startup-gate',action='store_true')
args=parser.parse_args();out=args.output_dir.resolve();out.mkdir(parents=True)
label=args.mode+('-fullscreen' if args.fullscreen else '')
def digest(p):return sha256(p.read_bytes()).hexdigest()
def read(p):return json.loads(p.read_text(encoding='utf-8'))
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
inputs=read(args.inputs);assert all(digest(repo/p)==h for p,h in inputs.items())
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment,regenerate_cache
runtime=out/'runtime';stage_runtime(args.package,runtime)
target=runtime/'plugins/gui/libqt_plugin.dll';target.unlink();shutil.copyfile(args.qa_gui,target)
regenerate_cache(runtime,out/'cache')
media=args.media.resolve();assert media.is_file()
config=out/'vlcrc';config.write_text('metadata-network-access=0\nml-network-discovery=0\none-instance=0\nqt-updates-notif=0\nqt-remember-window-geometry=0\nqt-controls-auto-hide=0\nqt-system-tray=0\nqt-start-minimized=0\nqt-notification=0\nqt-auto-raise=0\n',encoding='utf-8')
env=environment(runtime,out);env.pop('VLC_PLUGIN_PATH',None)
env.update(QT_QPA_PLATFORM='windows',QT_FORCE_STDERR_LOGGING='1',QT_MESSAGE_PATTERN='OWNED_QT_DIAG|%{type}|%{message}',
    VLC_OWNED_UI_REPORT=str(out/'journey.json'),VLC_OWNED_UI_START_GATE=str(out/'continue') if args.startup_gate else '',
    VLC_OWNED_QUIT_MODE=args.mode,VLC_OWNED_QUIT_FULLSCREEN=str(int(args.fullscreen)),VLC_OWNED_QUIT_MEDIA=str(media))
command=[str(runtime/'vlc.exe'),'--intf=qt','--no-one-instance','--no-media-library','--no-crashdump','--no-qt-privacy-ask',
    '--no-qt-updates-notif','--no-metadata-network-access','--no-ml-network-discovery','--http-proxy=direct://',
    '--config='+str(config),'--file-logging','--log-verbose=2','--verbose=2','--logfile='+str(out/'vlc.log'),
    '--aout=mmdevice,none','--mmdevice-backend=wasapi,none','--no-qt-video-autoresize','--no-hw-dec','--gain=0','--restore-playback-pos=0']
record=dict(status='running',mode=label,privateFixture=True,fullBetaAccepted=False,forcedCleanupUsed=False,
    expectedMedia=args.expected_media,
    coreSha256=digest(runtime/'libvlccore.dll'),privateGuiSha256=digest(target),mediaSha256=digest(media),
    sourceInputsSha256=digest(args.inputs),runnerSha256=digest(Path(__file__)))
with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
    child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
    record['pid']=child.pid;write(out/'process.json',record)
    print('Owned Quit test awaits native window inspection:',label,'PID',child.pid,flush=True)
    try:
        code=child.wait(timeout=600)
    except subprocess.TimeoutExpired:
        record.update(status='timeout_process_left_running',passed=False);write(out/'process.json',record)
        raise SystemExit('Quit timed out; owned PID remains recorded, no forced cleanup.')
logs='\n'.join((out/name).read_text(encoding='utf-8',errors='replace') for name in ['vlc.log','stderr.txt'])
journey=read(out/'journey.json');lines=(out/'vlc.log').read_text(encoding='utf-8',errors='replace').splitlines()
# Native enum order is STOPPED=0, STARTED=1, PLAYING=2, PAUSED=3.
# Rapid specifically holds the Qt event loop between Start and Quit.
qualified=(journey.get('cachedStateBeforeQuit')==0 if args.mode=='rapid'
    else journey.get('nativeStateBeforeQuit')==({'idle':0,'paused':3}.get(args.mode,2)))
def first(token):return next((i for i,s in enumerate(lines) if token in s),None)
wait=first('application quit waiting for primary playback to stop')
stopped=first('primary playback stopped; quitting application')
teardown=first('removing "interface" module "qt"')
real_audio=('using audio output module "mmdevice"' in logs and 'using aout stream module "wasapi"' in logs)
real_video=('using vout display module "direct3d11"' in logs and "ES track selected: 'video/" in logs)
backend_qualified=(args.mode in ['idle','rapid'] or (real_audio if args.expected_media=='audio'
    else real_video if args.expected_media=='video' else real_audio and real_video))
qt_terms=('ReferenceError','TypeError','Binding loop','Cannot assign','Unable to assign','unregistered datatype',
    'is not a type','failed to load component','Assertion failed','Signal uses unnamed parameter','SyntaxError',
    'QSignalSpy:','SVGColorizer:','QFSFileEngine::open: No file name specified')
record.update(status='terminal',exitCode=code,normalExit=code==0,journey=journey,
    runtimeIdentitiesUnchanged=(digest(runtime/'libvlccore.dll')==record['coreSha256']
        and digest(target)==record['privateGuiSha256']),
    sourceInputsUnchanged=all(digest(repo/p)==h for p,h in inputs.items()),mediaUnchanged=digest(media)==record['mediaSha256'],
    qtDiagnostics=[s for s in logs.splitlines() if re.search(r'OWNED_QT_DIAG\|(?:warning|critical|fatal)\||qt (?:warning|critical|fatal):',s)
        or any(term in s for term in qt_terms)],
    nativeErrors=[s for s in logs.splitlines() if re.search(r'\berror:',s)],
    triggerPreconditionQualified=qualified,
    realWindowsAudioOpened=real_audio,
    realWindowsVideoOpened=real_video,
    logOrder=dict(waitLine=wait,stopAcknowledgedLine=stopped,qtTeardownLine=teardown))
record['passed']=bool(record['normalExit'] and journey.get('passed') and qualified and record['runtimeIdentitiesUnchanged'] and record['sourceInputsUnchanged'] and record['mediaUnchanged'] and not record['qtDiagnostics'] and not record['nativeErrors']
    and backend_qualified
    and teardown is not None and (args.mode=='idle' or (wait is not None and stopped is not None and wait<stopped<teardown)))
write(out/'process.json',record)
print(json.dumps({k:record[k] for k in ['mode','passed','exitCode','qtDiagnostics','nativeErrors','logOrder']}),flush=True)
raise SystemExit(0 if record['passed'] else 1)
