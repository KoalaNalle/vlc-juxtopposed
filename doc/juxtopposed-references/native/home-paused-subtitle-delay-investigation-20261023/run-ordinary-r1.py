from pathlib import Path
from hashlib import sha256
import argparse, ctypes, json, re, shutil, subprocess, sys

work = Path(__file__).resolve().parent
repo = work.parent.parent
parser = argparse.ArgumentParser()
parser.add_argument('--phase', choices=['initial','reopen','index-reopen'], required=True)
args = parser.parse_args()
read = lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest = lambda p:sha256(p.read_bytes()).hexdigest()
def write(p, value):
    p.write_text(json.dumps(value,indent=2)+'\n',encoding='utf-8',newline='\n')

sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime, environment, regenerate_cache
prior = repo/'build-windows/beta-subtitle-frame-resume-20261023'
accepted = read(prior/'final-verification-r9.json')
base = prior/'ui-external.srt-r4/runtime'
runtime = work/'runtime'
config = work/'vlcrc'
manifest = work/'inputs-r1.json'
media = repo/'build-windows/beta-external-subtitle-acceptance-20261023/media-r1'
if args.phase=='initial':
    assert not runtime.exists() and not config.exists() and not manifest.exists()
    names = set(read(prior/'inputs-current-hover-r7.json')['files'])
    names.add(Path(__file__).relative_to(repo).as_posix())
    inputs = {n:digest(repo/n) for n in sorted(names)}
    snapshot = work/'source-r1'
    assert not snapshot.exists()
    snapshot.mkdir()
    for n,h in inputs.items():
        p = snapshot/n
        p.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(repo/n,p)
        assert digest(p)==h
    write(manifest,dict(files=inputs,sourceCount=len(inputs),sourceSnapshot=str(snapshot),fullBetaAccepted=False))
    assert all(digest(base/n)==h for n,h in accepted['runtimeSha256'].items())
    stage_runtime(base,runtime)
    regenerate_cache(runtime,work/'cache')
    config.write_text('metadata-network-access=0\nml-network-discovery=0\none-instance=0\nqt-updates-notif=0\nqt-auto-raise=0\nqt-remember-window-geometry=0\nqt-controls-auto-hide=0\nqt-timeline-thumbnails=0\nsub-autodetect-file=0\nsub-language=eng\nsub-track=-1\nsub-track-id=\nsub-delay=0\n',encoding='utf-8',newline='\n')
    ini=runtime/'portable/vlc/vlc-qt-interface.ini'
    ini.parent.mkdir(parents=True,exist_ok=True)
    ini.write_text('[General]\ngraphics-api=4\nfiledialog-path='+media.as_uri()+'\n[MainWindow]\ncolor-scheme-index=2\ninterface-scale=1\nplayqueue-panel-docked=true\n',encoding='utf-8',newline='\n')
else:
    previous='initial' if args.phase=='reopen' else 'reopen'
    last=read(work/previous/'verification.json')
    assert last['status']=='terminal' and last['normalExit'] and last['exitCode']==0
inputs=read(manifest)['files']
assert all(digest(repo/n)==h for n,h in inputs.items())
assert all(digest(runtime/n)==h for n,h in accepted['runtimeSha256'].items())
fixtures={n:digest(media/n) for n in ['frame-marker.mkv','frame-marker-ass.mkv','frame-marker.mp4']}
out=work/args.phase
assert not out.exists()
out.mkdir()
shutil.copyfile(config,out/'config-before.txt')
env=environment(runtime,work/'state')
env.pop('VLC_PLUGIN_PATH',None)
env.pop('QT_D3D_MAX_FRAME_LATENCY',None)
command=[str(runtime/'vlc.exe'),'--intf=qt','--no-one-instance','--no-media-library','--no-crashdump',
         '--no-qt-privacy-ask','--no-qt-updates-notif','--no-metadata-network-access','--no-ml-network-discovery',
         '--config='+str(config),'--file-logging','--log-verbose=2','--verbose=2','--logfile='+str(out/'vlc.log'),
         '--restore-playback-pos=0','--start-paused','--fullscreen',str(media/'frame-marker.mkv')]
write(out/'launch.json',command)
result=dict(status='starting',passed=False,observationsRequired=True,phase=args.phase,privateCore=False,privateGui=False,
            forcedCleanupUsed=False,fullBetaAccepted=False,sourceManifestSha256=digest(manifest),sourceInputCount=len(inputs),
            runtimeSha256=accepted['runtimeSha256'],fixtureSha256=fixtures,producerSha256=digest(Path(__file__)),
            scope='Actual ordinary subtitle preferences and synchronization editors, current/next-input behavior and fresh-process saved defaults. Does not qualify whole settings, subtitle layout, all formats/platforms or full beta.')
k=ctypes.WinDLL('kernel32',use_last_error=True)
k.SetErrorMode.argtypes=[ctypes.c_uint]
k.SetErrorMode.restype=ctypes.c_uint
previous=k.SetErrorMode(0x8003)
try:
    with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
        child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
        result.update(status='running',pid=child.pid)
        write(out/'verification.json',result)
        print('Owned ordinary subtitle-preferences',args.phase,'PID',child.pid,flush=True)
        try:result['exitCode']=child.wait(timeout=1800)
        except subprocess.TimeoutExpired:
            result.update(status='observation-timeout-process-left-running')
            write(out/'verification.json',result)
            raise SystemExit('Inspect saved PID; no automatic cleanup/restart')
finally:k.SetErrorMode(previous)
shutil.copyfile(config,out/'config-after.txt')
logs='\n'.join((out/n).read_text(encoding='utf-8',errors='replace') for n in ['vlc.log','stderr.txt'])
result.update(status='terminal',normalExit=result['exitCode']==0,
              sourceInputsUnchanged=all(digest(repo/n)==h for n,h in inputs.items()),
              runtimeUnchanged=all(digest(runtime/n)==h for n,h in accepted['runtimeSha256'].items()),
              fixturesUnchanged=all(digest(media/n)==h for n,h in fixtures.items()),
              diagnostics=[s for s in logs.splitlines() if re.search(r'\berror:|Assertion failed|Binding loop|ReferenceError|TypeError',s)])
write(out/'verification.json',result)
print(json.dumps({k:result[k] for k in ['status','phase','pid','exitCode','normalExit','sourceInputsUnchanged','runtimeUnchanged','fixturesUnchanged','diagnostics']}),flush=True)
