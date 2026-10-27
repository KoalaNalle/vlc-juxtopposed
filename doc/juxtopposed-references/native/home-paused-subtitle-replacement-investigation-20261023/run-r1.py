from pathlib import Path
from hashlib import sha256
import ctypes,json,shutil,subprocess,sys
work=Path(__file__).resolve().parent;repo=work.parent.parent
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment
digest=lambda p:sha256(p.read_bytes()).hexdigest()
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
prior=read(repo/'build-windows/beta-caption-layout-20261023/inputs-after-r1.json')['files']
changed=[p for p,h in prior.items() if digest(repo/p)!=h]
assert changed==['PROJECT_HANDOFF.md','doc/juxtopposed-beta-roadmap.md','doc/juxtopposed-settings-consumer-audit.md']
files=set(prior);files.update(p.relative_to(repo).as_posix() for p in work.iterdir() if p.suffix=='.py')
hashes={p:digest(repo/p) for p in sorted(files)}
snapshot=work/'source-r1';assert not snapshot.exists();snapshot.mkdir()
for p,h in hashes.items():
    target=snapshot/p;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/p,target);assert digest(target)==h
write(work/'inputs-r1.json',dict(sourceCount=len(hashes),files=hashes,sourceSnapshot=str(snapshot),compiledRuntimeManifest='build-windows/beta-caption-layout-20261023/inputs-after-r1.json',postBuildDocumentationChanges=changed,investigationOnly=True,fullBetaAccepted=False))
root=work/'native-r1';assert not root.exists();root.mkdir()
base=repo/'build-windows/beta-caption-layout-20261023/ui-after-r1/runtime'
protected=read(base.parent/'verification.json')['runtimeSha256']
assert protected['plugins/gui/libqt_plugin.dll']=='d0ff14c66da4287a36a769754deead89c4ba616fb31fb8e1262c62a9a35e2678'
assert all(digest(base/p)==h for p,h in protected.items())
worker=work/'native-tracks-r1.py';workerHash=digest(worker)
records=[]
k=ctypes.WinDLL('kernel32',use_last_error=True);k.SetErrorMode.argtypes=[ctypes.c_uint];k.SetErrorMode.restype=ctypes.c_uint;previous=k.SetErrorMode(0x8003)
try:
    for name in ['original-wasapi','preserve-wasapi','original-noaudio']:
        out=root/name;out.mkdir();runtime=out/'runtime';stage_runtime(base,runtime)
        identities={p:digest(runtime/p) for p in protected};env=environment(runtime,out)
        command=[sys.executable,str(worker),'--case',name,'--runtime',str(runtime),'--output',str(out)]
        record=dict(case=name,status='starting',passed=False,investigationOnly=True,forcedCleanupUsed=False,privateCore=False,runtimeSha256=identities,producerSha256=workerHash,parentProducerSha256=digest(Path(__file__)))
        with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
            child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
            record.update(status='running',pid=child.pid);write(out/'process.json',record);print(name,'PID',child.pid,flush=True)
            try:code=child.wait(timeout=150)
            except subprocess.TimeoutExpired:
                record.update(status='timeout-process-left-running');write(out/'process.json',record);raise SystemExit('Inspect saved owned PID; no automatic cleanup/restart')
        record.update(status='terminal',exitCode=code,normalExit=code==0,runtimeUnchanged=all(digest(base/p)==h and digest(runtime/p)==h for p,h in protected.items()),sourceInputsUnchanged=all(digest(repo/p)==h for p,h in hashes.items()),producerUnchanged=digest(worker)==workerHash)
        write(out/'process.json',record);records.append(record)
        print(name,'normal exit',code,flush=True)
        if code!=0:raise SystemExit('Inspect this execution failure before proceeding')
finally:k.SetErrorMode(previous)
write(root/'processes.json',dict(investigationOnly=True,passed=False,fullBetaAccepted=False,allProcessesNormal=all(r['normalExit'] for r in records),records=records,sourceManifestSha256=digest(work/'inputs-r1.json')))
