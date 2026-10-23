from pathlib import Path
from hashlib import sha256
import ctypes,json,shutil,subprocess,sys
work=Path(__file__).resolve().parent;repo=work.parent.parent
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
manifest=work/'inputs-r3.json';inputs=read(manifest)['files'];assert all(digest(repo/p)==h for p,h in inputs.items())
matrix=read(work/'native-candidate-r3/processes.json');assert len(matrix['records'])==10 and all(r['passed'] and r['normalExit'] for r in matrix['records'])
base=repo/'build-windows/beta-subtitle-replacement-repair-20261023/ui-frame-marker.mkv-r3/runtime';protected=read(base.parent/'verification.json')['runtimeSha256']
assert all(digest(base/p)==h for p,h in protected.items())
media=work/'media-extra-r2b';fixtures={p.name:digest(p) for p in media.iterdir() if p.is_file()};worker=work/'native-tracks-extra-r3.py'
assert digest(worker)==read(work/'extra-probe-r3.json')['probeSha256']
root=work/'native-policy-r6';assert not root.exists();root.mkdir();records=[]
cases=[dict(audio=a,selection=s,policy=p) for a,s,p in [('wasapi','automatic','ids'),('wasapi','preserve','ids'),('none','preserve','language'),('none','preserve','disabled'),('none','automatic','language')]]
k=ctypes.WinDLL('kernel32',use_last_error=True);k.SetErrorMode.argtypes=[ctypes.c_uint];k.SetErrorMode.restype=ctypes.c_uint;previous=k.SetErrorMode(0x8003)
try:
    for cfg in cases:
        name='-'.join(cfg.values());out=root/name;out.mkdir();runtime=out/'runtime';stage_runtime(base,runtime)
        core=runtime/'libvlccore.dll';core.unlink();shutil.copyfile(work/'candidate-r3-bin/libvlccore.dll',core)
        identities={p:digest(runtime/p) for p in protected};probe=out/'probe';probe.mkdir()
        command=[sys.executable,str(worker),'--fixture-root',str(media),'--media','frame-marker.mkv','--audio',cfg['audio'],'--external-selection',cfg['selection'],'--external-policy',cfg['policy'],'--external-file','multi-language.mks','--external-track-count','2','--runtime',str(runtime),'--output',str(probe)]
        r=dict(case=name,config=cfg,status='starting',privateCore=True,forcedCleanupUsed=False,producerSha256=digest(worker),parentProducerSha256=digest(Path(__file__)),sourceManifestSha256=digest(manifest),runtimeSha256=identities,fixtureSha256=fixtures)
        with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
            child=subprocess.Popen(command,cwd=runtime,env=environment(runtime,out),stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
            r.update(status='running',pid=child.pid);write(out/'process.json',r);print(name,'PID',child.pid,flush=True)
            try:code=child.wait(timeout=180)
            except subprocess.TimeoutExpired:
                r.update(status='timeout-process-left-running');write(out/'process.json',r);raise SystemExit('Inspect saved PID; no cleanup/restart')
        result=read(probe/'result.json') if (probe/'result.json').exists() else None
        r.update(status='terminal',exitCode=code,normalExit=code==0,passed=bool(result and result.get('passed')),result=result,sourceInputsUnchanged=all(digest(repo/p)==h for p,h in inputs.items()),runtimeUnchanged=all(digest(base/p)==h and digest(runtime/p)==identities[p] for p,h in protected.items()),fixturesUnchanged=all(digest(media/p)==h for p,h in fixtures.items()))
        write(out/'process.json',r);records.append(r);print(name,'exit',code,'passed',r['passed'],flush=True)
        if code!=0 or not r['passed']:raise SystemExit('Inspect this policy failure before continuing')
finally:k.SetErrorMode(previous)
write(root/'processes.json',dict(records=records,passed=all(r['passed'] and r['normalExit'] for r in records),fullBetaAccepted=False))
