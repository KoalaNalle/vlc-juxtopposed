from pathlib import Path
from hashlib import sha256
import argparse,ctypes,json,shutil,subprocess,sys
work=Path(__file__).resolve().parent;repo=work.parent.parent
parser=argparse.ArgumentParser();parser.add_argument('--phase',choices=['before','ordinary','candidate'],required=True);args=parser.parse_args()
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
manifest=work/'inputs-ordinary-r3.json';inputs=read(manifest)['files'];assert all(digest(repo/p)==h for p,h in inputs.items())
abi=repo/'build-windows/beta-subtitle-frame-feedback-20261023/layout-r1.json'
provenance=read(abi.parent/'layout-provenance-r1.json')
assert provenance['headerSha256']==digest(repo/'include/vlc/libvlc_media_player.h') and provenance['sourceSha256']==digest(repo/'test/libvlc/native_player_callback_layout.c')
media=repo/'build-windows/beta-external-subtitle-acceptance-20261023/media-r1';fixtures={p.name:digest(p) for p in media.iterdir() if p.is_file()}
base=repo/'build-windows/beta-external-subtitle-acceptance-20261023/ui-external.srt-r3/runtime';protected=read(base.parent/'verification.json')['runtimeSha256'];assert all(digest(base/p)==h for p,h in protected.items())
worker=repo/'test/libvlc/native_paused_subtitle_tracks.py'
cases=[dict(media=m,audio=a,external='external.srt',count=1) for m in ['frame-marker.mkv','frame-marker-ass.mkv','frame-marker.mp4'] for a in ['wasapi','none']]
cases+=[dict(media='frame-marker.mkv',audio='wasapi',external='multi-language.mks',count=2)]
root=work/'native-ordinary-r3';assert not root.exists();root.mkdir();records=[]
k=ctypes.WinDLL('kernel32',use_last_error=True);k.SetErrorMode.argtypes=[ctypes.c_uint];k.SetErrorMode.restype=ctypes.c_uint;previous=k.SetErrorMode(0x8003)
try:
    for cfg in cases:
        name='-'.join(str(cfg[p]) for p in ['media','audio','external']);out=root/name;out.mkdir();runtime=out/'runtime';stage_runtime(base,runtime)
        if args.phase=='ordinary':
            core=runtime/'libvlccore.dll';core.unlink();shutil.copyfile(work/'ordinary-core-r3/libvlccore.dll',core)
        identities={p:digest(runtime/p) for p in protected};probe=out/'probe';probe.mkdir()
        command=[sys.executable,str(worker),'--fixture-root',str(media),'--media',cfg['media'],'--audio',cfg['audio'],'--external-selection','automatic','--external-policy','ids','--external-file',cfg['external'],'--external-track-count',str(cfg['count']),'--frame-feedback-abi',str(abi),'--runtime',str(runtime),'--output',str(probe)]
        record=dict(case=name,phase=args.phase,config=cfg,status='starting',passed=False,privateCore=False,forcedCleanupUsed=False,sourceManifestSha256=digest(manifest),producerSha256=digest(worker),parentProducerSha256=digest(Path(__file__)),runtimeSha256=identities,fixtureSha256=fixtures,abiSha256=digest(abi))
        with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
            child=subprocess.Popen(command,cwd=runtime,env=environment(runtime,out),stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
            record.update(status='running',pid=child.pid);write(out/'process.json',record);print(args.phase,name,'PID',child.pid,flush=True)
            try:code=child.wait(timeout=180)
            except subprocess.TimeoutExpired:
                record.update(status='timeout-process-left-running');write(out/'process.json',record);raise SystemExit('Inspect saved PID; no cleanup/restart')
        result=read(probe/'result.json') if (probe/'result.json').exists() else None
        record.update(status='terminal',exitCode=code,normalExit=code==0,passed=bool(result and result.get('passed')),result=result,sourceInputsUnchanged=all(digest(repo/p)==h for p,h in inputs.items()),runtimeUnchanged=all(digest(base/p)==h and digest(runtime/p)==identities[p] for p,h in protected.items()),fixturesUnchanged=all(digest(media/p)==h for p,h in fixtures.items()))
        write(out/'process.json',record);records.append(record);print(args.phase,name,'exit',code,'passed',record['passed'],flush=True)
        if code!=0 or not result or result.get('status')!='terminal':raise SystemExit('Inspect execution failure')
        assert result['checks']['noUnrequestedFrameFeedback']==(args.phase=='ordinary')
        if not all(v for n,v in result['checks'].items() if n!='noUnrequestedFrameFeedback'):raise SystemExit('Inspect other failed behavior before proceeding')
finally:k.SetErrorMode(previous)
write(root/'processes.json',dict(records=records,allProcessesNormal=all(r['normalExit'] for r in records),expectedBehaviorObserved=True,fullBetaAccepted=False))
