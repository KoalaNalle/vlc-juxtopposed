from pathlib import Path
from hashlib import sha256
import argparse,ctypes,json,re,shutil,subprocess,sys
work=Path(__file__).resolve().parent;repo=work.parent.parent
parser=argparse.ArgumentParser();parser.add_argument('--media',choices=['frame-marker.mkv','frame-marker-ass.mkv','frame-marker.mp4'],required=True);args=parser.parse_args()
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment,regenerate_cache
digest=lambda p:sha256(p.read_bytes()).hexdigest()
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
manifest=work/'inputs-ordinary-r2.json';inputs=read(manifest)['files'];assert all(digest(repo/p)==h for p,h in inputs.items())
native=read(work/'native-ordinary-r1/processes.json');assert len(native['records'])==6 and native['allProcessesNormal'] and all(r['passed'] for r in native['records'])
media=work/'media-r1';mediaHashes={p.name:digest(p) for p in media.iterdir() if p.is_file()}
out=work/('ui-'+args.media+'-r3');assert not out.exists();out.mkdir();runtime=out/'runtime'
base=repo/'build-windows/beta-caption-layout-20261023/ui-after-r1/runtime';protected=read(base.parent/'verification.json')['runtimeSha256'];assert all(digest(base/p)==h for p,h in protected.items())
stage_runtime(base,runtime)
core=runtime/'libvlccore.dll';core.unlink();shutil.copyfile(work/'ordinary-core-r2/libvlccore.dll',core)
assert b'candidate subtitle frame refresh' not in core.read_bytes()
identities={p:digest(runtime/p) for p in protected}
env=environment(runtime,out);env.pop('VLC_PLUGIN_PATH',None);env.pop('QT_D3D_MAX_FRAME_LATENCY',None)
regenerate_cache(runtime,out/'cache')
config=out/'vlcrc';config.write_text('metadata-network-access=0\nml-network-discovery=0\none-instance=0\nqt-updates-notif=0\nqt-auto-raise=0\nqt-remember-window-geometry=0\nqt-controls-auto-hide=0\nqt-pin-controls=0\nqt-timeline-thumbnails=0\nsub-autodetect-file=0\naudio-language=eng\nsub-language=eng\naudio-track=-1\nsub-track=-1\n',encoding='utf-8',newline='\n')
ini=runtime/'portable/vlc/vlc-qt-interface.ini';ini.parent.mkdir(parents=True,exist_ok=True)
ini.write_text('[General]\ngraphics-api=4\nfiledialog-path='+media.as_uri()+'\n[MainWindow]\ncolor-scheme-index=2\ninterface-scale=1\nplayqueue-panel-docked=true\n',encoding='utf-8',newline='\n')
(out/'config-before.txt').write_bytes(config.read_bytes())
command=[str(runtime/'vlc.exe'),'--intf=qt','--no-one-instance','--no-media-library','--no-crashdump','--no-qt-privacy-ask','--no-qt-updates-notif','--no-metadata-network-access','--no-ml-network-discovery','--config='+str(config),'--file-logging','--log-verbose=2','--verbose=2','--logfile='+str(out/'vlc.log'),'--restore-playback-pos=0','--no-sub-autodetect-file','--start-paused','--fullscreen',str(media/args.media)]
result=dict(status='starting',passed=False,observationsRequired=True,privateGui=False,privateCore=False,forcedCleanupUsed=False,fullBetaAccepted=False,sourceManifest=manifest.relative_to(repo).as_posix(),sourceManifestSha256=digest(manifest),sourceInputCount=len(inputs),runtimeSha256=identities,producerSha256=digest(Path(__file__)),fixtureProducerSha256=digest(repo/'test/libvlc/generate_paused_subtitle_tracks.sh'),mediaSha256=mediaHashes,scope='Ordinary visible paused track/subtitle selection, sampled UI position, marker observations and external file loading. No full subtitle/layout/beta or physical-audibility acceptance.',media=args.media)
def save():write(out/'verification.json',result)
k=ctypes.WinDLL('kernel32',use_last_error=True);k.SetErrorMode.argtypes=[ctypes.c_uint];k.SetErrorMode.restype=ctypes.c_uint;previous=k.SetErrorMode(0x8003)
try:
    with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
        child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
        result.update(status='running',pid=child.pid);save();print('Owned subtitle replacement GUI PID',child.pid,flush=True)
        try:result['exitCode']=child.wait(timeout=1800)
        except subprocess.TimeoutExpired:
            result.update(status='observation-timeout-process-left-running');save();raise SystemExit('Inspect saved PID; no automatic cleanup/restart')
finally:k.SetErrorMode(previous)
logs='\n'.join((out/p).read_text(encoding='utf-8',errors='replace') for p in ['vlc.log','stderr.txt'])
result.update(status='terminal',normalExit=result['exitCode']==0,diagnostics=[l for l in logs.splitlines() if re.search(r'\berror:|Assertion failed|Binding loop|ReferenceError|TypeError',l)],sourceInputsUnchanged=all(digest(repo/p)==h for p,h in inputs.items()),runtimeUnchanged=all(digest(base/p)==h and digest(runtime/p)==identities[p] for p,h in protected.items()),mediaUnchanged=all(digest(media/p)==h for p,h in mediaHashes.items()))
(out/'config-after.txt').write_bytes(config.read_bytes());save();print(json.dumps({k:result[k] for k in ['status','pid','exitCode','normalExit','diagnostics','sourceInputsUnchanged','runtimeUnchanged','mediaUnchanged']}))
