from pathlib import Path
from hashlib import sha256
import argparse,ctypes,json,shutil,subprocess,sys
work=Path(__file__).resolve().parent;repo=work.parent.parent
parser=argparse.ArgumentParser();parser.add_argument('--phase',choices=['before','candidate'],required=True);args=parser.parse_args()
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment
digest=lambda p:sha256(p.read_bytes()).hexdigest()
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
manifest=work/'inputs-r4.json'
if not manifest.exists():
    prior=read(work/'inputs-r1.json')['files'];assert all(digest(repo/p)==h for p,h in prior.items())
    files=set(prior)
    files.update(p.relative_to(repo).as_posix() for p in work.iterdir() if p.suffix in ['.py','.mk','.sh'])
    files.update((work/p).relative_to(repo).as_posix() for p in ['buffer-candidate-r2/decoder.c','refresh-candidate-r3/input.c','refresh-candidate-r4/input.c'])
    hashes={p:digest(repo/p) for p in sorted(files)}
    source=work/'source-r4';assert not source.exists();source.mkdir()
    for p,h in hashes.items():
        target=source/p;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/p,target);assert digest(target)==h
    write(manifest,dict(sourceCount=len(hashes),files=hashes,sourceSnapshot=str(source),productionApplied=False,fullBetaAccepted=False))
hashes=read(manifest)['files'];assert all(digest(repo/p)==h for p,h in hashes.items())
root=work/('native-'+args.phase+'-r4');assert not root.exists();root.mkdir()
base=repo/'build-windows/beta-caption-layout-20261023/ui-after-r1/runtime'
protected=read(base.parent/'verification.json')['runtimeSha256'];assert all(digest(base/p)==h for p,h in protected.items())
worker=work/'native-tracks-r4.py';workerHash=digest(worker);records=[]
k=ctypes.WinDLL('kernel32',use_last_error=True);k.SetErrorMode.argtypes=[ctypes.c_uint];k.SetErrorMode.restype=ctypes.c_uint;previous=k.SetErrorMode(0x8003)
try:
    for name in ['original-wasapi','preserve-wasapi','original-noaudio','colored-wasapi','colored-noaudio']:
        out=root/name;out.mkdir();runtime=out/'runtime';stage_runtime(base,runtime)
        if args.phase=='candidate':
            core=runtime/'libvlccore.dll';core.unlink();shutil.copyfile(work/'refresh-candidate-r4-bin/libvlccore.dll',core)
        identities={p:digest(runtime/p) for p in protected};env=environment(runtime,out)
        record=dict(case=name,phase=args.phase,status='starting',passed=False,investigationOnly=True,forcedCleanupUsed=False,candidateCore=args.phase=='candidate',runtimeSha256=identities,producerSha256=workerHash,parentProducerSha256=digest(Path(__file__)))
        command=[sys.executable,str(worker),'--case',name,'--runtime',str(runtime),'--output',str(out)]
        with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
            child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
            record.update(status='running',pid=child.pid);write(out/'process.json',record);print(args.phase,name,'PID',child.pid,flush=True)
            try:code=child.wait(timeout=150)
            except subprocess.TimeoutExpired:
                record.update(status='timeout-process-left-running');write(out/'process.json',record);raise SystemExit('Inspect saved PID; no automatic cleanup/restart')
        record.update(status='terminal',exitCode=code,normalExit=code==0,runtimeUnchanged=all(digest(base/p)==h and digest(runtime/p)==identities[p] for p,h in protected.items()),sourceInputsUnchanged=all(digest(repo/p)==h for p,h in hashes.items()),producerUnchanged=digest(worker)==workerHash)
        write(out/'process.json',record);records.append(record);print(args.phase,name,'normal exit',code,flush=True)
        if code!=0:raise SystemExit('Inspect this failure before continuing')
finally:k.SetErrorMode(previous)
write(root/'processes.json',dict(passed=False,investigationOnly=True,fullBetaAccepted=False,records=records,sourceManifestSha256=digest(manifest)))
