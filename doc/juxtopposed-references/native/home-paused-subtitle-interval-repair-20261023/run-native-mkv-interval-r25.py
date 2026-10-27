from pathlib import Path
from hashlib import sha256
import ctypes,json,shutil,subprocess,sys
repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
def step(name,target,color,subtitle):return dict(name=name,target=target,color=color,subtitle=subtitle)
configs=[
    dict(name='cold-long-english',media='media-r20/colored-long.mkv',language='eng',audio=True,steps=[step('last',45,1,True),step('first',5,0,True),step('middle',25,2,True),step('first-again',5,0,True)]),
    dict(name='cold-long-finnish',media='media-r20/colored-long.mkv',language='fin',audio=True,steps=[step('middle',25,2,True),step('first',5,0,True),step('last',45,1,True),step('middle-again',25,2,True)]),
    dict(name='cold-delayed-first-cue',media='media-r20/colored-delayed.mkv',language='eng',audio=True,steps=[step('first',15,1,True),step('gap',25,2,False),step('middle',35,0,True),step('gap-again',45,1,False),step('last',55,2,True),step('first-again',15,1,True)]),
    dict(name='short-cue-gaps',media='media-r20/colored-gaps.mkv',language='eng',audio=False,steps=[step('first-gap',5,0,False),step('middle-gap',25,2,False),step('last-gap',45,1,False),step('middle',21,2,True),step('last',41,1,True),step('first',1,0,True)]),
]
overlap=dict(name='overlapping-cues',media='media-r21/colored-overlap.mkv',language='eng',audio=True,steps=[step('overlap-first',11,1,True),step('long-alone',15,1,True),step('overlap-second',31,0,True),step('long-alone-again',35,0,True),step('long-at-first',5,0,True)])
refs=[
    'native-mkv-overlap-r21/after/overlapping-cues/seek-0-overlap-first.png',
    'native-overlap-reference-r22/before/normal-playback-reference/normal-playback-after-short-cue-ended.png',
    'native-overlap-mask-reference-r24/before/normal-playback-reference/normal-overlap-31s.png',
    'native-overlap-mask-reference-r24/before/normal-playback-reference/normal-long-alone-35s.png',
    'native-overlap-mask-reference-r24/before/normal-playback-reference/normal-long-alone-35s.png',
]
for item,reference in zip(overlap['steps'],refs):item['referencePicture']=reference
configs.append(overlap)
for kind,start,duration in [('delayed',10,9.999),('gaps',0,2)]:
    packets=read(work/'media-r20'/(kind+'-subtitle-packets.json'))['packets'];assert len(packets)==3
    assert float(packets[0]['pts_time'])==start and float(packets[0]['duration_time'])==duration
source=read(work/'mkv-interval-r23d-source/source-record.json');inputs=source['files']
assert all(digest(repo/p)==h for p,h in inputs.items())
assert source['sourceCount']==2196 and all(digest(work/'source-mkv-interval-r23d'/p)==h for p,h in inputs.items())
for phase,files in source['privateSources'].items():
    assert all(digest(work/'mkv-interval-r23d-source'/phase/p)==h for p,h in files.items())
assert (work/'build-mkv-interval-r23d-exit.txt').read_text().strip()=='0'
base=work/'ordinary-ui-r14/runtime';protected=read(work/'ordinary-ui-r14/verification.json')['runtimeSha256']
for p in ['plugins/demux/libmkv_plugin.dll','plugins/codec/libsubsdec_plugin.dll','plugins/text_renderer/libfreetype_plugin.dll']:protected[p]=digest(base/p)
assert all(digest(base/p)==h for p,h in protected.items())
plugins={phase:work/('mkv-interval-'+phase+'-r23d-bin/libmkv_plugin.dll') for phase in ['before','after']}
hashes={phase:digest(p) for phase,p in plugins.items()}
worker=work/'native-mkv-interval-r25.py';worker_hash=digest(worker)
fixtures={c['media']:digest(work/c['media']) for c in configs}
root=work/'native-mkv-interval-r25';assert not root.exists();root.mkdir();records=[]
kernel=ctypes.WinDLL('kernel32',use_last_error=True);kernel.SetErrorMode.argtypes=[ctypes.c_uint];kernel.SetErrorMode.restype=ctypes.c_uint;previous=kernel.SetErrorMode(0x8003)
try:
    for phase in ['before','after']:
        phase_dir=root/phase;phase_dir.mkdir()
        for cfg in configs:
            out=phase_dir/cfg['name'];out.mkdir();write(out/'scenario.json',cfg)
            runtime=out/'runtime';stage_runtime(base,runtime)
            path=runtime/'plugins/demux/libmkv_plugin.dll';path.unlink();shutil.copyfile(plugins[phase],path)
            identities={p:digest(runtime/p) for p in protected}
            assert identities['plugins/demux/libmkv_plugin.dll']==hashes[phase]
            record=dict(phase=phase,case=cfg['name'],status='starting',passed=False,candidateOnly=True,privateMkv=True,privateCore=False,forcedCleanupUsed=False,runtimeSha256=identities,protectedRuntimeSha256=protected,producerSha256=worker_hash,parentProducerSha256=digest(Path(__file__)),fixtureProducerSha256=digest(work/'generate-subtitle-seek-edges-r20.sh'),sourceRecordSha256=digest(work/'mkv-interval-r23d-source/source-record.json'))
            with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
                child=subprocess.Popen([sys.executable,str(worker),'--scenario',str(out/'scenario.json'),'--runtime',str(runtime),'--output',str(out)],cwd=runtime,env=environment(runtime,out),stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
                record.update(status='running',pid=child.pid);write(out/'process.json',record);print(phase,cfg['name'],'PID',child.pid,flush=True)
                try:code=child.wait(timeout=150)
                except subprocess.TimeoutExpired:
                    record['status']='timeout-process-left-running';write(out/'process.json',record);raise SystemExit('Inspect exact owned PID; no automatic cleanup/restart')
            value=read(out/'result.json') if (out/'result.json').exists() else {}
            record.update(status='terminal',exitCode=code,normalExit=code==0,passed=value.get('passed',False),runtimeUnchanged=all(digest(runtime/p)==h for p,h in identities.items()) and all(digest(base/p)==h for p,h in protected.items()),sourceInputsUnchanged=all(digest(repo/p)==h for p,h in inputs.items()),producerUnchanged=digest(worker)==worker_hash,fixturesUnchanged=all(digest(work/p)==h for p,h in fixtures.items()))
            write(out/'process.json',record);records.append(record)
            assert code==0,'Inspect terminal setup/execution failure before proceeding'
            print(phase,cfg['name'],'normal terminal','passed',record['passed'],'steps',[(r['step'],r.get('passed'),r.get('brightPixels')) for r in value['observations']],flush=True)
finally:kernel.SetErrorMode(previous)
write(root/'comparison.json',dict(passed=False,candidateOnly=True,repairAccepted=False,allProcessesNormal=all(r['normalExit'] for r in records),records=records))
