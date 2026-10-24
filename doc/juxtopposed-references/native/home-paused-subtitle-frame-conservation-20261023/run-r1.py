from pathlib import Path
from hashlib import sha256
import argparse,ctypes,json,shutil,subprocess,sys
from PySide6.QtGui import QImage
work=Path(__file__).resolve().parent;repo=work.parent.parent
parser=argparse.ArgumentParser();parser.add_argument('--phase',choices=['before','old','candidate'],required=True);args=parser.parse_args()
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment
digest=lambda p:sha256(p.read_bytes()).hexdigest()
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
assert (work/'generate-marker-r1-exit.txt').read_text().strip()=='0'
media=work/'media-r1';frames=read(media/'frames.json')['frames'];assert len(frames)==1500
probe=read(media/'probe.json');video=next(s for s in probe['streams'] if s['codec_type']=='video')
assert video['width']==960 and video['height']==540 and video['avg_frame_rate']=='25/1'
markerRefs={}
for n in [0,125,126,625]:
    path=media/('frame-'+str(n)+'.png');im=QImage(str(path));assert not im.isNull()
    value=sum((1<<bit) if im.pixelColor(32+64*bit,20).red()>128 else 0 for bit in range(11))
    assert value==n,(n,value);markerRefs[path.name]=dict(frameMarker=value,sha256=digest(path))
manifest=work/'inputs-r1.json'
if not manifest.exists():
    prior=read(repo/'build-windows/beta-subtitle-workflows-20261023/inputs-r4.json')['files']
    changed=[p for p,h in prior.items() if digest(repo/p)!=h]
    assert changed==['PROJECT_HANDOFF.md','doc/juxtopposed-beta-roadmap.md','doc/juxtopposed-settings-consumer-audit.md']
    files=set(prior);files.update(p.relative_to(repo).as_posix() for p in work.iterdir() if p.suffix in ['.py','.mk','.sh'])
    files.add((work/'candidate-r1/input.c').relative_to(repo).as_posix())
    hashes={p:digest(repo/p) for p in sorted(files)}
    source=work/'source-r1';assert not source.exists();source.mkdir()
    for p,h in hashes.items():
        target=source/p;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/p,target);assert digest(target)==h
    write(manifest,dict(sourceCount=len(hashes),files=hashes,sourceSnapshot=str(source),productionApplied=False,fixtureMarkerReferences=markerRefs,mediaSha256=digest(media/'frame-marker.mkv'),fullBetaAccepted=False))
hashes=read(manifest)['files'];assert all(digest(repo/p)==h for p,h in hashes.items())
root=work/('native-'+args.phase+'-r1');assert not root.exists();root.mkdir()
base=repo/'build-windows/beta-caption-layout-20261023/ui-after-r1/runtime'
protected=read(base.parent/'verification.json')['runtimeSha256'];assert all(digest(base/p)==h for p,h in protected.items())
worker=work/'native-tracks-r1.py';workerHash=digest(worker);records=[]
cases=['marker-wasapi','marker-noaudio']
if args.phase=='candidate':cases=['original-wasapi','preserve-wasapi','original-noaudio','colored-wasapi','colored-noaudio']+cases
k=ctypes.WinDLL('kernel32',use_last_error=True);k.SetErrorMode.argtypes=[ctypes.c_uint];k.SetErrorMode.restype=ctypes.c_uint;previous=k.SetErrorMode(0x8003)
try:
    for name in cases:
        out=root/name;out.mkdir();runtime=out/'runtime';stage_runtime(base,runtime)
        if args.phase!='before':
            sourceCore=work/'candidate-r1-bin/libvlccore.dll' if args.phase=='candidate' else repo/'build-windows/beta-subtitle-workflows-20261023/refresh-candidate-r4-bin/libvlccore.dll'
            core=runtime/'libvlccore.dll';core.unlink();shutil.copyfile(sourceCore,core)
        identities={p:digest(runtime/p) for p in protected};env=environment(runtime,out)
        record=dict(case=name,phase=args.phase,status='starting',passed=False,investigationOnly=True,forcedCleanupUsed=False,candidateCore=args.phase!='before',runtimeSha256=identities,producerSha256=workerHash,parentProducerSha256=digest(Path(__file__)),sourceManifestSha256=digest(manifest))
        command=[sys.executable,str(worker),'--case',name,'--runtime',str(runtime),'--output',str(out)]
        with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
            child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
            record.update(status='running',pid=child.pid);write(out/'process.json',record);print(args.phase,name,'PID',child.pid,flush=True)
            try:code=child.wait(timeout=150)
            except subprocess.TimeoutExpired:
                record.update(status='timeout-process-left-running');write(out/'process.json',record);raise SystemExit('Inspect saved PID; no automatic cleanup/restart')
        record.update(status='terminal',exitCode=code,normalExit=code==0,runtimeUnchanged=all(digest(base/p)==h and digest(runtime/p)==identities[p] for p,h in protected.items()),sourceInputsUnchanged=all(digest(repo/p)==h for p,h in hashes.items()),producerUnchanged=digest(worker)==workerHash,markerMediaUnchanged=digest(media/'frame-marker.mkv')==read(manifest)['mediaSha256'])
        write(out/'process.json',record);records.append(record);print(args.phase,name,'normal exit',code,flush=True)
        if code!=0:raise SystemExit('Inspect this failure before continuing')
finally:k.SetErrorMode(previous)
write(root/'processes.json',dict(passed=False,investigationOnly=True,fullBetaAccepted=False,records=records,sourceManifestSha256=digest(manifest)))
