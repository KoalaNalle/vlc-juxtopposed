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
manifest=work/'inputs-r1.json';inputs=read(manifest)['files'];assert all(digest(repo/p)==h for p,h in inputs.items())
base=repo/'build-windows/beta-subtitle-replacement-repair-20261023/ui-frame-marker.mkv-r3/runtime'
protected=read(base.parent/'verification.json')['runtimeSha256'];assert all(digest(base/p)==h for p,h in protected.items())
media=repo/'build-windows/beta-subtitle-replacement-repair-20261023/media-r1'
fixtures={p.name:digest(p) for p in media.iterdir() if p.is_file()}
worker=repo/'test/libvlc/native_paused_subtitle_tracks.py';records=[]
cases=[dict(media=m,audio=a,selection='automatic',policy='ids') for m in ['frame-marker.mkv','frame-marker-ass.mkv','frame-marker.mp4'] for a in ['wasapi','none']]
cases += [dict(media='frame-marker.mkv',audio='none',selection=s,policy=p) for s,p in [('preserve','ids'),('preserve','disabled'),('automatic','language'),('automatic','disabled')]]
root=work/('native-'+args.phase+'-r1');assert not root.exists();root.mkdir()
k=ctypes.WinDLL('kernel32',use_last_error=True);k.SetErrorMode.argtypes=[ctypes.c_uint];k.SetErrorMode.restype=ctypes.c_uint;previous=k.SetErrorMode(0x8003)
try:
    for cfg in cases:
        name='-'.join(cfg.values());out=root/name;out.mkdir();runtime=out/'runtime';stage_runtime(base,runtime)
        if args.phase=='candidate':
            core=runtime/'libvlccore.dll';core.unlink();shutil.copyfile(work/'candidate-r1-bin/libvlccore.dll',core)
        identities={p:digest(runtime/p) for p in protected};probe=out/'probe';probe.mkdir()
        command=[sys.executable,str(worker),'--fixture-root',str(media),'--media',cfg['media'],'--audio',cfg['audio'],'--external-selection',cfg['selection'],'--external-policy',cfg['policy'],'--runtime',str(runtime),'--output',str(probe)]
        r=dict(status='starting',case=name,config=cfg,passed=False,phase=args.phase,privateCore=args.phase=='candidate',forcedCleanupUsed=False,sourceManifestSha256=digest(manifest),producerSha256=digest(worker),parentProducerSha256=digest(Path(__file__)),runtimeSha256=identities,fixtureSha256=fixtures)
        with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
            child=subprocess.Popen(command,cwd=runtime,env=environment(runtime,out),stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
            r.update(status='running',pid=child.pid);write(out/'process.json',r);print(args.phase,name,'PID',child.pid,flush=True)
            try:code=child.wait(timeout=180)
            except subprocess.TimeoutExpired:
                r.update(status='timeout-process-left-running');write(out/'process.json',r);raise SystemExit('Inspect saved PID; no cleanup/restart')
        result=read(probe/'result.json') if (probe/'result.json').exists() else None
        r.update(status='terminal',exitCode=code,normalExit=code==0,result=result,passed=bool(result and result.get('passed')),sourceInputsUnchanged=all(digest(repo/p)==h for p,h in inputs.items()),runtimeUnchanged=all(digest(base/p)==h and digest(runtime/p)==identities[p] for p,h in protected.items()),fixturesUnchanged=all(digest(media/p)==h for p,h in fixtures.items()))
        write(out/'process.json',r);records.append(r);print(args.phase,name,'exit',code,'passed',r['passed'],flush=True)
        if code!=0 or not result or result.get('status')!='terminal':raise SystemExit('Inspect execution failure before continuing')
        expected=args.phase=='candidate' or cfg['selection']=='preserve'
        if r['passed']!=expected:raise SystemExit('Inspect unexpected behavior before continuing')
finally:k.SetErrorMode(previous)
write(root/'processes.json',dict(records=records,allProcessesNormal=all(r['normalExit'] for r in records),expectedBehaviorObserved=True,fullBetaAccepted=False))
