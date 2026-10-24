from pathlib import Path
from hashlib import sha256
import argparse,ctypes,json,subprocess,sys
from PySide6.QtGui import QImage
work=Path(__file__).resolve().parent;repo=work.parent.parent
parser=argparse.ArgumentParser();parser.add_argument('--phase',choices=['before','candidate','ordinary'],required=True);args=parser.parse_args()
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment
digest=lambda p:sha256(p.read_bytes()).hexdigest()
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
manifest=work/('inputs-ordinary-r2.json' if args.phase=='ordinary' else 'inputs-tests-r1.json')
inputs=read(manifest)['files'];assert all(digest(repo/p)==h for p,h in inputs.items())
assert (work/'generate-fixtures-r1-exit.txt').read_text().strip()=='0'
media=work/'media-r1';mediaHashes={p.name:digest(p) for p in media.iterdir() if p.is_file()}
assert len(read(media/'frames.json')['frames'])==1500
for n in [0,125,126,625]:
    im=QImage(str(media/('frame-'+str(n)+'.png')));assert not im.isNull()
    assert sum((1<<b) if im.pixelColor(32+64*b,20).red()>128 else 0 for b in range(11))==n
root=work/('native-'+args.phase+'-r1');assert not root.exists();root.mkdir()
base=repo/'build-windows/beta-caption-layout-20261023/ui-after-r1/runtime'
protected=read(base.parent/'verification.json')['runtimeSha256'];assert all(digest(base/p)==h for p,h in protected.items())
worker=repo/'test/libvlc/native_paused_subtitle_tracks.py';workerHash=digest(worker);records=[]
k=ctypes.WinDLL('kernel32',use_last_error=True);k.SetErrorMode.argtypes=[ctypes.c_uint];k.SetErrorMode.restype=ctypes.c_uint;previous=k.SetErrorMode(0x8003)
try:
    for name in ['frame-marker.mkv','frame-marker-ass.mkv','frame-marker.mp4']:
        for audio in ['wasapi','none']:
            label=name+'-'+audio;out=root/label;out.mkdir();runtime=out/'runtime';stage_runtime(base,runtime)
            if args.phase!='before':
                import shutil
                core=runtime/'libvlccore.dll';core.unlink()
                source=repo/'build-windows/beta-subtitle-frame-conservation-20261023/candidate-r1-bin/libvlccore.dll' if args.phase=='candidate' else work/'ordinary-core-r2/libvlccore.dll'
                shutil.copyfile(source,core)
            identities={p:digest(runtime/p) for p in protected};env=environment(runtime,out);probe=out/'probe';probe.mkdir()
            command=[sys.executable,str(worker),'--fixture-root',str(media),'--media',name,'--audio',audio,'--runtime',str(runtime),'--output',str(probe)]
            record=dict(case=label,phase=args.phase,status='starting',passed=False,forcedCleanupUsed=False,privateDiagnosticCore=args.phase=='candidate',runtimeSha256=identities,producerSha256=workerHash,parentProducerSha256=digest(Path(__file__)),sourceManifestSha256=digest(manifest),fixtureSha256=mediaHashes)
            with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
                child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
                record.update(status='running',pid=child.pid);write(out/'process.json',record);print(args.phase,label,'PID',child.pid,flush=True)
                try:code=child.wait(timeout=180)
                except subprocess.TimeoutExpired:
                    record.update(status='timeout-process-left-running');write(out/'process.json',record);raise SystemExit('Inspect saved PID; no automatic cleanup/restart')
            result=read(probe/'result.json') if (probe/'result.json').exists() else None
            record.update(status='terminal',exitCode=code,normalExit=code==0,result=result,
                runtimeUnchanged=all(digest(base/p)==h and digest(runtime/p)==identities[p] for p,h in protected.items()),
                sourceInputsUnchanged=all(digest(repo/p)==h for p,h in inputs.items()),fixtureUnchanged=all(digest(media/p)==h for p,h in mediaHashes.items()))
            if result:record['passed']=result['passed']
            write(out/'process.json',record);records.append(record)
            print(args.phase,label,'exit',code,'behavior',record['passed'],flush=True)
            if code!=0 or result is None or result.get('status')!='terminal':raise SystemExit('Inspect this setup/execution failure before proceeding')
            expected=args.phase!='before'
            if record['passed']!=expected:raise SystemExit('Inspect unexpected behavior before continuing matrix')
finally:k.SetErrorMode(previous)
write(root/'processes.json',dict(phase=args.phase,allProcessesNormal=all(r['normalExit'] for r in records),expectedBehaviorObserved=True,records=records,fullBetaAccepted=False))
