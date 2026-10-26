from pathlib import Path
from hashlib import sha256
import ctypes,json,shutil,subprocess,sys
work=Path(__file__).resolve().parent
repo=work.parents[1]
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
inputs=read(work/'inputs-r15.json')['files']
assert all(digest(repo/p)==h for p,h in inputs.items())
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment
out=work/'wasapi-startup-r15'
assert not out.exists()
out.mkdir()
shutil.copyfile(Path(__file__),out/'producer.py')
base=work/'native-final-r15/runtime'
runtime=out/'runtime'
stage_runtime(base,runtime)
identities=read(work/'native-final-r15/verification.json')['runtimeSha256']
cfg=read(work/'regressions-ordinary-r12/startup-delayed-video-wasapi/config.json')
media=Path(cfg['media'])
media_hash=digest(media)
worker=repo/'test/libvlc/native_paused_video_startup.py'
worker_hash=digest(worker)
records=[]
kernel=ctypes.WinDLL('kernel32',use_last_error=True)
kernel.SetErrorMode.argtypes=[ctypes.c_uint];kernel.SetErrorMode.restype=ctypes.c_uint
previous=kernel.SetErrorMode(0x8003)
try:
    for index in range(1,11):
        case=out/f'{index:02}'
        case.mkdir();probe=case/'probe';probe.mkdir()
        config=case/'config.json'
        config.write_text(json.dumps(cfg,indent=2)+'\n',encoding='utf-8')
        command=[sys.executable,str(worker),'--config',str(config),'--runtime',str(runtime),'--output',str(probe)]
        record=dict(status='starting',case=index,forcedCleanupUsed=False)
        with (case/'stdout.txt').open('wb') as stdout,(case/'stderr.txt').open('wb') as stderr:
            child=subprocess.Popen(command,cwd=runtime,env=environment(runtime,case),
                                   stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
            record.update(status='running',pid=child.pid)
            (case/'process.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8')
            print('Repeated WASAPI startup',index,'PID',child.pid,flush=True)
            try:code=child.wait(timeout=90)
            except subprocess.TimeoutExpired:
                raise SystemExit('Inspect recorded child; no automatic cleanup/restart')
        result=read(probe/'result.json') if (probe/'result.json').exists() else {}
        unexpected=[line for line in result.get('diagnostics',[]) if not line.endswith('main error: window closed')]
        record.update(status='terminal',exitCode=code,normalExit=code==0,result=result,
                      unexpectedDiagnostics=unexpected,
                      passed=bool(code==0 and result.get('passed') and result.get('actualWasapiSelected') and not unexpected))
        records.append(record)
        (case/'process.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8')
        (out/'verification.json').write_text(json.dumps(dict(status='running',records=records),indent=2)+'\n',encoding='utf-8')
        if not record['passed']:
            raise SystemExit('Repeated WASAPI case failed; inspect before proceeding')
finally:kernel.SetErrorMode(previous)
summary=dict(status='terminal',passed=True,records=records,fullBetaAccepted=False,
             sourceInputsUnchanged=all(digest(repo/p)==h for p,h in inputs.items()),
             runtimeUnchanged=all(digest(runtime/p)==h and digest(base/p)==h for p,h in identities.items()),
             fixturesUnchanged=digest(media)==media_hash,producerUnchanged=digest(worker)==worker_hash,
             runtimeSha256=identities,mediaSha256=media_hash,workerSha256=worker_hash)
(out/'verification.json').write_text(json.dumps(summary,indent=2)+'\n',encoding='utf-8')
assert all(summary[k] for k in ['sourceInputsUnchanged','runtimeUnchanged','fixturesUnchanged','producerUnchanged'])
print('Ten repeated WASAPI startups passed with normal shutdown',flush=True)
