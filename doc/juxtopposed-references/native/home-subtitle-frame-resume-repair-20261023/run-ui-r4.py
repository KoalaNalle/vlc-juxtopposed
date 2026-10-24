from pathlib import Path
from hashlib import sha256
import argparse,ctypes,json,re,shutil,subprocess,sys
work=Path(__file__).resolve().parent;repo=work.parent.parent
parser=argparse.ArgumentParser();parser.add_argument('--external',choices=['external.srt','external.ass','multi-language.mks'],required=True);args=parser.parse_args()
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment,regenerate_cache
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
manifest=work/'inputs-ordinary-r3.json';inputs=read(manifest)['files'];assert all(digest(repo/p)==h for p,h in inputs.items())
native=read(work/'native-ordinary-r3/processes.json');assert len(native['records'])==7 and native['allProcessesNormal'] and all(r['passed'] for r in native['records'])
media=repo/'build-windows/beta-external-subtitle-acceptance-20261023/media-r1';fixtures={p.name:digest(p) for p in media.iterdir() if p.is_file()}
base=repo/'build-windows/beta-external-subtitle-acceptance-20261023/ui-external.srt-r3/runtime';protected=read(base.parent/'verification.json')['runtimeSha256'];assert all(digest(base/p)==h for p,h in protected.items())
out=work/('ui-'+args.external+'-r4');assert not out.exists();out.mkdir();runtime=out/'runtime';stage_runtime(base,runtime)
core=runtime/'libvlccore.dll';core.unlink();shutil.copyfile(work/'ordinary-core-r3/libvlccore.dll',core)
identities={p:digest(runtime/p) for p in protected};regenerate_cache(runtime,out/'cache')
env=environment(runtime,out);env.pop('VLC_PLUGIN_PATH',None);env.pop('QT_D3D_MAX_FRAME_LATENCY',None)
config=out/'vlcrc';config.write_text('metadata-network-access=0\nml-network-discovery=0\none-instance=0\nqt-updates-notif=0\nqt-auto-raise=0\nqt-remember-window-geometry=0\nqt-controls-auto-hide=0\nqt-pin-controls=0\nqt-timeline-thumbnails=0\nsub-autodetect-file=0\naudio-language=eng\nsub-language=eng\naudio-track=-1\nsub-track=-1\n',encoding='utf-8',newline='\n')
ini=runtime/'portable/vlc/vlc-qt-interface.ini';ini.parent.mkdir(parents=True,exist_ok=True);ini.write_text('[General]\ngraphics-api=4\nfiledialog-path='+media.as_uri()+'\n[MainWindow]\ncolor-scheme-index=2\ninterface-scale=1\nplayqueue-panel-docked=true\n',encoding='utf-8',newline='\n')
(out/'config-before.txt').write_bytes(config.read_bytes())
cmd=[str(runtime/'vlc.exe'),'--intf=qt','--no-one-instance','--no-media-library','--no-crashdump','--no-qt-privacy-ask','--no-qt-updates-notif','--no-metadata-network-access','--no-ml-network-discovery','--config='+str(config),'--file-logging','--log-verbose=2','--verbose=2','--logfile='+str(out/'vlc.log'),'--restore-playback-pos=0','--no-sub-autodetect-file','--start-paused','--fullscreen',str(media/'frame-marker.mkv')]
r=dict(status='starting',passed=False,observationsRequired=True,privateCore=False,privateGui=False,forcedCleanupUsed=False,fullBetaAccepted=False,externalFile=args.external,scope='Ordinary subtitle load/selection, genuine frame steps, resume/caption and subsequent real seeking/feedback review. No whole-subtitle/layout/platform/beta acceptance.',sourceManifestSha256=digest(manifest),sourceInputCount=len(inputs),producerSha256=digest(Path(__file__)),runtimeSha256=identities,fixtureSha256=fixtures)
k=ctypes.WinDLL('kernel32',use_last_error=True);k.SetErrorMode.argtypes=[ctypes.c_uint];k.SetErrorMode.restype=ctypes.c_uint;previous=k.SetErrorMode(0x8003)
try:
    with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
        child=subprocess.Popen(cmd,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
        r.update(status='running',pid=child.pid);write(out/'verification.json',r);print('Owned external subtitle GUI PID',child.pid,flush=True)
        try:r['exitCode']=child.wait(timeout=1800)
        except subprocess.TimeoutExpired:
            r.update(status='observation-timeout-process-left-running');write(out/'verification.json',r);raise SystemExit('Inspect saved PID; no automatic cleanup/restart')
finally:k.SetErrorMode(previous)
logs='\n'.join((out/n).read_text(encoding='utf-8',errors='replace') for n in ['vlc.log','stderr.txt'])
r.update(status='terminal',normalExit=r['exitCode']==0,sourceInputsUnchanged=all(digest(repo/p)==h for p,h in inputs.items()),runtimeUnchanged=all(digest(base/p)==h and digest(runtime/p)==identities[p] for p,h in protected.items()),fixturesUnchanged=all(digest(media/p)==h for p,h in fixtures.items()),diagnostics=[line for line in logs.splitlines() if re.search(r'\berror:|Assertion failed|Binding loop|ReferenceError|TypeError',line)])
(out/'config-after.txt').write_bytes(config.read_bytes());write(out/'verification.json',r)
print(json.dumps({p:r[p] for p in ['status','pid','exitCode','normalExit','sourceInputsUnchanged','runtimeUnchanged','fixturesUnchanged','diagnostics']}))
