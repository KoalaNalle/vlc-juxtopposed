from pathlib import Path
from hashlib import sha256
import ctypes,json,subprocess,sys
repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment
digest=lambda p:sha256(p.read_bytes()).hexdigest()
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
inputs=read(work/'inputs-r4.json');assert len(inputs)==2139 and all(digest(repo/p)==h for p,h in inputs.items())
root=work/'native-startup-r7';assert not root.exists();root.mkdir()
base=work/'ordinary-ui-r4/runtime';identities=read(work/'ordinary-ui-r4/verification.json')['runtimeSha256']
assert all(digest(base/p)==h for p,h in identities.items())
worker=work/'native-startup-r7.py';workerHash=digest(worker)
cases=['remux-all-wasapi','two-audio-no-subs-wasapi','one-audio-two-subs-wasapi'];records=[]
kernel=ctypes.WinDLL('kernel32',use_last_error=True);kernel.SetErrorMode.argtypes=[ctypes.c_uint];kernel.SetErrorMode.restype=ctypes.c_uint;previous=kernel.SetErrorMode(0x8003)
try:
    for name in cases:
        out=root/name;out.mkdir();runtime=out/'runtime';stage_runtime(base,runtime);env=environment(runtime,out)
        command=[sys.executable,str(worker),'--case',name,'--runtime',str(runtime),'--output',str(out)]
        record=dict(case=name,status='starting',passed=False,investigationOnly=True,forcedCleanupUsed=False,runtimeSha256=identities,producerSha256=workerHash,parentProducerSha256=digest(Path(__file__)))
        with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
            child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
            record.update(status='running',pid=child.pid);write(out/'process.json',record);print(name,'PID',child.pid,flush=True)
            try:code=child.wait(timeout=150)
            except subprocess.TimeoutExpired:
                record.update(status='timeout-process-left-running');write(out/'process.json',record);raise SystemExit('Inspect owned PID; no automatic cleanup or restart')
        record.update(status='terminal',exitCode=code,normalExit=code==0,runtimeUnchanged=all(digest(runtime/p)==h and digest(base/p)==h for p,h in identities.items()),sourceInputsUnchanged=all(digest(repo/p)==h for p,h in inputs.items()),producerUnchanged=digest(worker)==workerHash)
        write(out/'process.json',record);records.append(record)
        if code!=0:raise SystemExit('Case setup/execution failed; inspect this result before continuing')
        print(name,'normal terminal',flush=True)
finally:kernel.SetErrorMode(previous)
write(root/'processes.json',dict(passed=False,investigationOnly=True,allProcessesNormal=all(r['normalExit'] for r in records),records=records,sourceManifestSha256=digest(work/'inputs-r4.json')))
