from pathlib import Path
from hashlib import sha256
import ctypes,json,subprocess,sys,time
work=Path(__file__).resolve().parent;repo=work.parent.parent
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
manifest=work/'inputs-r1.json';inputs=read(manifest)['files'];assert all(digest(repo/p)==h for p,h in inputs.items())
media=repo/'build-windows/beta-external-subtitle-acceptance-20261023/media-r1';fixtures={p.name:digest(p) for p in media.iterdir() if p.is_file()}
base=repo/'build-windows/beta-external-subtitle-acceptance-20261023/ui-external.srt-r3/runtime';protected=read(base.parent/'verification.json')['runtimeSha256'];assert all(digest(base/p)==h for p,h in protected.items())
abi=repo/'build-windows/beta-subtitle-frame-feedback-20261023/layout-r1.json'
worker=work/'probe-control-r1.py';root=work/'native-controls-r1';assert not root.exists();root.mkdir();records=[]
k=ctypes.WinDLL('kernel32',use_last_error=True);k.SetErrorMode.argtypes=[ctypes.c_uint];k.SetErrorMode.restype=ctypes.c_uint;previous=k.SetErrorMode(0x8003)
try:
    for mode,audio in [('hold','wasapi'),('steps','wasapi'),('hold','none'),('steps','none')]:
        name=mode+'-'+audio;out=root/name;out.mkdir();runtime=out/'runtime';stage_runtime(base,runtime)
        identities={p:digest(runtime/p) for p in protected};probe=out/'probe';probe.mkdir()
        command=[sys.executable,str(worker),'--fixture-root',str(media),'--media','frame-marker.mkv','--audio',audio,'--external-selection','automatic','--external-policy','ids','--external-file','external.srt','--external-track-count','1','--frame-feedback-abi',str(abi),'--extra-paused-mode',mode,'--runtime',str(runtime),'--output',str(probe)]
        record=dict(case=name,status='starting',passed=False,privateCore=False,privateProbe=True,forcedCleanupUsed=False,sourceManifestSha256=digest(manifest),producerSha256=digest(worker),parentProducerSha256=digest(Path(__file__)),runtimeSha256=identities,fixtureSha256=fixtures,abiSha256=digest(abi))
        with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
            started=time.monotonic();child=subprocess.Popen(command,cwd=runtime,env=environment(runtime,out),stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
            record.update(status='running',pid=child.pid);write(out/'process.json',record);print(name,'PID',child.pid,flush=True)
            try:code=child.wait(timeout=180)
            except subprocess.TimeoutExpired:
                record.update(status='timeout-process-left-running');write(out/'process.json',record);raise SystemExit('Inspect saved PID; no automatic cleanup/restart')
        result=read(probe/'result.json') if (probe/'result.json').exists() else None
        record.update(status='terminal',exitCode=code,normalExit=code==0,elapsedSeconds=time.monotonic()-started,passed=bool(result and result.get('passed')),result=result,sourceInputsUnchanged=all(digest(repo/p)==h for p,h in inputs.items()),runtimeUnchanged=all(digest(base/p)==h and digest(runtime/p)==identities[p] for p,h in protected.items()),fixturesUnchanged=all(digest(media/p)==h for p,h in fixtures.items()))
        write(out/'process.json',record);records.append(record)
        print(name,'exit',code,'caption survives resume',bool(result and result['checks']['externalDistinct']),flush=True)
        if code!=0 or not result or result.get('status')!='terminal':raise SystemExit('Inspect execution failure')
finally:k.SetErrorMode(previous)
write(root/'processes.json',dict(records=records,allProcessesNormal=all(r['normalExit'] for r in records),completed=True,fullBetaAccepted=False))
