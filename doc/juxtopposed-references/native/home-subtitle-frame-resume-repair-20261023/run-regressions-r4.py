from pathlib import Path
from hashlib import sha256
import ctypes,json,shutil,subprocess,sys
work=Path(__file__).resolve().parent;repo=work.parent.parent
old=repo/'build-windows/beta-tracks-subtitles-20261023'
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment
digest=lambda p:sha256(p.read_bytes()).hexdigest()
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
manifest=work/'inputs-ordinary-r3.json';inputs=read(manifest)['files'];assert all(digest(repo/p)==h for p,h in inputs.items())
ordinary=read(work/'native-ordinary-r3/processes.json');assert len(ordinary['records'])==7 and ordinary['allProcessesNormal'] and all(r['passed'] for r in ordinary['records'])
base=repo/'build-windows/beta-external-subtitle-acceptance-20261023/ui-external.srt-r3/runtime';protected=read(base.parent/'verification.json')['runtimeSha256']
assert all(digest(base/p)==h for p,h in protected.items())
root=work/'regressions-ordinary-r4';assert not root.exists();root.mkdir()
definitions=[]
for p in sorted((old/'native-startup-ordinary-r14').glob('*/config.json')):
    cfg=read(p);assert all(v=='--no-spu' for v in cfg['extraOptions'])
    definitions.append(('startup',cfg['name'],p,repo/'test/libvlc/native_paused_video_startup.py',cfg))
assert len(definitions)==9
for p in sorted((old/'native-subtitle-ordinary-r36c/after').glob('*/scenario.json')):
    cfg=read(p);definitions.append(('subtitle',cfg['name'],p,repo/'test/libvlc/native_paused_subtitle_seek.py',cfg))
assert len(definitions)==17,len(definitions)
records=[]
k=ctypes.WinDLL('kernel32',use_last_error=True);k.SetErrorMode.argtypes=[ctypes.c_uint];k.SetErrorMode.restype=ctypes.c_uint;previous=k.SetErrorMode(0x8003)
try:
    for family,name,sourceConfig,worker,cfg in definitions:
        out=root/(family+'-'+name);out.mkdir();runtime=out/'runtime';stage_runtime(base,runtime)
        core=runtime/'libvlccore.dll';core.unlink();shutil.copyfile(work/'ordinary-core-r3/libvlccore.dll',core)
        identities={p:digest(runtime/p) for p in protected};probe=out/'probe';probe.mkdir();config=out/'config.json';write(config,cfg)
        media=Path(cfg['media']) if family=='startup' else old/cfg['media']
        fixtureHashes={str(media):digest(media)}
        if family=='subtitle':
            for step in cfg['steps']:
                if 'referencePicture' in step:fixtureHashes[str(old/step['referencePicture'])]=digest(old/step['referencePicture'])
        command=[sys.executable,str(worker),'--config' if family=='startup' else '--scenario',str(config),'--runtime',str(runtime),'--output',str(probe)]
        if family=='subtitle':command+=['--fixture-root',str(old)]
        record=dict(family=family,case=name,status='starting',passed=False,privateCore=False,forcedCleanupUsed=False,runtimeSha256=identities,producerSha256=digest(worker),parentProducerSha256=digest(Path(__file__)),originalConfigSha256=digest(sourceConfig),sourceManifestSha256=digest(manifest),fixtureSha256=fixtureHashes)
        with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
            child=subprocess.Popen(command,cwd=runtime,env=environment(runtime,out),stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
            record.update(status='running',pid=child.pid);write(out/'process.json',record);print(family,name,'PID',child.pid,flush=True)
            try:code=child.wait(timeout=180)
            except subprocess.TimeoutExpired:
                record.update(status='timeout-process-left-running');write(out/'process.json',record);raise SystemExit('Inspect saved PID; no automatic cleanup/restart')
        result=read(probe/'result.json') if (probe/'result.json').exists() else None
        record.update(status='terminal',exitCode=code,normalExit=code==0,result=result,passed=bool(result and result.get('passed')),
            runtimeUnchanged=all(digest(base/p)==h and digest(runtime/p)==identities[p] for p,h in protected.items()),sourceInputsUnchanged=all(digest(repo/p)==h for p,h in inputs.items()),
            fixturesUnchanged=all(digest(Path(p))==h for p,h in fixtureHashes.items()),producerUnchanged=digest(worker)==record['producerSha256'])
        write(out/'process.json',record);records.append(record);print(family,name,'exit',code,'passed',record['passed'],flush=True)
        if code!=0 or not record['passed']:raise SystemExit('Inspect this regression before proceeding')
finally:k.SetErrorMode(previous)
write(root/'processes.json',dict(passed=all(r['passed'] and r['normalExit'] and r['runtimeUnchanged'] and r['sourceInputsUnchanged'] and r['fixturesUnchanged'] for r in records),records=records,fullBetaAccepted=False))
