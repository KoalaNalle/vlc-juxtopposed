from pathlib import Path
from hashlib import sha256
import argparse,ctypes,json,re,shutil,subprocess,sys
parser=argparse.ArgumentParser();parser.add_argument('--phase',choices=['before','after'],required=True);args=parser.parse_args()
work=Path(__file__).resolve().parent;repo=work.parent.parent
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment,regenerate_cache
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
phase=args.phase
manifest=work/('inputs-before-r1.json' if phase=='before' else 'inputs-after-r2.json')
inputs=read(manifest)['files'];assert all(digest(repo/p)==h for p,h in inputs.items())
assert (work/('build-'+phase+'-r1-exit.txt')).read_text().strip()=='0'
base=repo/'build-windows/beta-tracks-subtitles-20261023/ordinary-ui-r37/runtime'
protected=read(base.parent/'verification.json')['runtimeSha256']
assert all(digest(base/p)==h for p,h in protected.items())
out=work/('native-'+phase+'-r1');assert not out.exists();out.mkdir();runtime=out/'runtime'
stage_runtime(base,runtime)
qa=work/('qa-'+phase+'-r1-bin/libqt_plugin.dll')
target=runtime/'plugins/gui/libqt_plugin.dll';target.unlink();shutil.copyfile(qa,target)
identities={p:digest(runtime/p) for p in protected}
assert identities['plugins/gui/libqt_plugin.dll']==digest(qa)
cache=regenerate_cache(runtime,out/'cache')
media=repo/'build-windows/beta-tracks-subtitles-20261023/media-generator-smoke-r35/colored-long.mkv';media_hash=digest(media)
config=out/'vlcrc'
config.write_text('metadata-network-access=0\nml-network-discovery=0\none-instance=0\nqt-updates-notif=0\nqt-auto-raise=0\nqt-remember-window-geometry=0\nqt-controls-auto-hide=0\nqt-timeline-thumbnails=0\nsub-autodetect-file=0\ninput-fast-seek=0\nshort-jump-size=3\naudio-language=eng\nsub-language=eng\n',encoding='utf-8',newline='\n')
ini=runtime/'portable/vlc/vlc-qt-interface.ini';ini.parent.mkdir(parents=True,exist_ok=True)
ini.write_text('[General]\ngraphics-api=4\n[MainWindow]\ncolor-scheme-index=2\ninterface-scale=1\n',encoding='utf-8',newline='\n')
env=environment(runtime,out);env.pop('VLC_PLUGIN_PATH',None)
gate=out/'inspection-ready.txt'
env.update(VLC_OWNED_UI_REPORT=str(out/'journey.json'),VLC_OWNED_UI_BASE=str(out),VLC_OWNED_UI_START_GATE=str(gate),VLC_OWNED_SEEK_MEDIA=str(media))
command=[str(runtime/'vlc.exe'),'--intf=qt','--no-one-instance','--no-media-library','--no-crashdump','--no-qt-privacy-ask','--no-qt-updates-notif','--no-metadata-network-access','--no-ml-network-discovery','--config='+str(config),'--file-logging','--log-verbose=2','--verbose=2','--logfile='+str(out/'vlc.log'),'--no-hw-dec','--no-sub-autodetect-file','--aout=mmdevice','--mmdevice-backend=wasapi']
record=dict(phase=phase,status='starting',passed=False,privateGui=True,privateCore=False,forcedCleanupUsed=False,fullBetaAccepted=False,
    runtimeSha256=identities,sourceManifestSha256=digest(manifest),sourceCount=len(inputs),producerSha256=digest(Path(__file__)),qaSourceSha256=digest(work/'seek_accessibility_journey.cpp'),mediaSha256=media_hash,command=command)
kernel=ctypes.WinDLL('kernel32',use_last_error=True);kernel.SetErrorMode.argtypes=[ctypes.c_uint];kernel.SetErrorMode.restype=ctypes.c_uint;previous=kernel.SetErrorMode(0x8003)
try:
    with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
        child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
        record.update(status='running',pid=child.pid);write(out/'process.json',record);print(phase,'owned PID',child.pid,'inspection gate',str(gate),flush=True)
        try:code=child.wait(timeout=180)
        except subprocess.TimeoutExpired:
            record['status']='timeout-process-left-running';write(out/'process.json',record);raise SystemExit('Inspect exact owned PID; no automatic cleanup or restart')
finally:kernel.SetErrorMode(previous)
value=read(out/'journey.json') if (out/'journey.json').exists() else {}
logs='\n'.join((out/p).read_text(encoding='utf-8',errors='replace') for p in ['vlc.log','stderr.txt'])
record.update(status='terminal',exitCode=code,normalExit=code==0,passed=value.get('passed',False),journey=value,
    runtimeUnchanged=all(digest(runtime/p)==h for p,h in identities.items()) and all(digest(base/p)==h for p,h in protected.items()),
    sourceInputsUnchanged=all(digest(repo/p)==h for p,h in inputs.items()),mediaUnchanged=digest(media)==media_hash,
    diagnostics=[l for l in logs.splitlines() if re.search(r'\berror:|Assertion failed|ReferenceError|TypeError|Binding loop|Cannot assign',l)])
write(out/'process.json',record)
print(json.dumps(dict(phase=phase,normalExit=record['normalExit'],passed=record['passed'],checks=len(value.get('checks',[])),error=value.get('error'),diagnostics=record['diagnostics'])),flush=True)
assert record['normalExit'] and record['runtimeUnchanged'] and record['sourceInputsUnchanged'] and record['mediaUnchanged']
