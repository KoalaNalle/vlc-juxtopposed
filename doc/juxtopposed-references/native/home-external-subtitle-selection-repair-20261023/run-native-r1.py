from pathlib import Path
from hashlib import sha256
import argparse,ctypes,json,shutil,subprocess,sys
from PySide6.QtGui import QImage
work=Path(__file__).resolve().parent;repo=work.parent.parent
parser=argparse.ArgumentParser();parser.add_argument('--phase',choices=['before','ordinary'],required=True);args=parser.parse_args()
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
manifest=work/('inputs-before-r1.json' if args.phase=='before' else 'inputs-ordinary-r2.json');inputs=read(manifest)['files']
assert all(digest(repo/p)==h for p,h in inputs.items())
assert (work/'generate-fixtures-r1-exit.txt').read_text().strip()=='0'
media=work/'media-r1';fixtures={p.name:digest(p) for p in media.iterdir() if p.is_file()}
frames=read(media/'frames.json')['frames'];assert len(frames)==1500
assert all(abs(float(r['best_effort_timestamp_time'])-i/25)<.000001 for i,r in enumerate(frames))
for n in [0,125,126,625]:
    im=QImage(str(media/('frame-'+str(n)+'.png')));assert not im.isNull()
    assert sum((1<<bit) if im.pixelColor(32+64*bit,20).red()>128 else 0 for bit in range(11))==n
base=repo/'build-windows/beta-subtitle-replacement-repair-20261023/ui-frame-marker.mkv-r3/runtime';protected=read(base.parent/'verification.json')['runtimeSha256']
assert all(digest(base/p)==h for p,h in protected.items())
worker=repo/'test/libvlc/native_paused_subtitle_tracks.py'
cases=[dict(media=m,audio=a,selection='automatic',policy='ids',external='external.srt',count=1,missing=False,beforePass=False) for m in ['frame-marker.mkv','frame-marker-ass.mkv','frame-marker.mp4'] for a in ['wasapi','none']]
cases += [dict(media='frame-marker.mkv',audio='none',selection=s,policy=p,external='external.srt',count=1,missing=False,beforePass=s=='preserve') for s,p in [('preserve','ids'),('preserve','disabled'),('automatic','language'),('automatic','disabled')]]
cases += [dict(media='frame-marker.mkv',audio='none',selection='automatic',policy='ids',external=e,count=c,missing=True,beforePass=False) for e,c in [('external.ass',1),('multi-language.mks',2)]]
cases += [dict(media='frame-marker.mkv',audio=a,selection=s,policy=p,external='multi-language.mks',count=2,missing=True,beforePass=False) for a,s,p in [('wasapi','automatic','ids'),('wasapi','preserve','ids'),('none','preserve','language'),('none','preserve','disabled'),('none','automatic','language')]]
assert len(cases)==17
root=work/('native-'+args.phase+'-r1');assert not root.exists();root.mkdir();records=[]
k=ctypes.WinDLL('kernel32',use_last_error=True);k.SetErrorMode.argtypes=[ctypes.c_uint];k.SetErrorMode.restype=ctypes.c_uint;previous=k.SetErrorMode(0x8003)
try:
    for cfg in cases:
        name='-'.join(str(cfg[p]) for p in ['media','audio','selection','policy','external']);out=root/name;out.mkdir();runtime=out/'runtime';stage_runtime(base,runtime)
        if args.phase=='ordinary':
            core=runtime/'libvlccore.dll';core.unlink();shutil.copyfile(work/'ordinary-core-r2/libvlccore.dll',core)
        identities={p:digest(runtime/p) for p in protected};probe=out/'probe';probe.mkdir()
        command=[sys.executable,str(worker),'--fixture-root',str(media),'--media',cfg['media'],'--audio',cfg['audio'],'--external-selection',cfg['selection'],'--external-policy',cfg['policy'],'--external-file',cfg['external'],'--external-track-count',str(cfg['count']),'--runtime',str(runtime),'--output',str(probe)]
        if cfg['missing']:command+=['--check-missing-external']
        r=dict(case=name,phase=args.phase,config=cfg,status='starting',passed=False,privateCore=False,forcedCleanupUsed=False,sourceManifestSha256=digest(manifest),producerSha256=digest(worker),parentProducerSha256=digest(Path(__file__)),fixtureProducerSha256=digest(repo/'test/libvlc/generate_paused_subtitle_tracks.sh'),runtimeSha256=identities,fixtureSha256=fixtures)
        with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
            child=subprocess.Popen(command,cwd=runtime,env=environment(runtime,out),stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
            r.update(status='running',pid=child.pid);write(out/'process.json',r);print(args.phase,name,'PID',child.pid,flush=True)
            try:code=child.wait(timeout=180)
            except subprocess.TimeoutExpired:
                r.update(status='timeout-process-left-running');write(out/'process.json',r);raise SystemExit('Inspect saved PID; no cleanup/restart')
        result=read(probe/'result.json') if (probe/'result.json').exists() else None
        r.update(status='terminal',exitCode=code,normalExit=code==0,passed=bool(result and result.get('passed')),result=result,sourceInputsUnchanged=all(digest(repo/p)==h for p,h in inputs.items()),runtimeUnchanged=all(digest(base/p)==h and digest(runtime/p)==identities[p] for p,h in protected.items()),fixturesUnchanged=all(digest(media/p)==h for p,h in fixtures.items()))
        write(out/'process.json',r);records.append(r);print(args.phase,name,'exit',code,'passed',r['passed'],flush=True)
        if code!=0 or not result or result.get('status')!='terminal':raise SystemExit('Inspect execution failure')
        if r['passed']!=(args.phase=='ordinary' or cfg['beforePass']):raise SystemExit('Inspect unexpected behavior before proceeding')
finally:k.SetErrorMode(previous)
write(root/'processes.json',dict(records=records,allProcessesNormal=all(r['normalExit'] for r in records),expectedBehaviorObserved=True,fullBetaAccepted=False))
