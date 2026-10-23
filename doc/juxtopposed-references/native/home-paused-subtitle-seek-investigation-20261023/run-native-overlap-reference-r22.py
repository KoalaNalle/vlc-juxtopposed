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
configs=[dict(name='normal-playback-reference',media='media-r21/colored-overlap.mkv',language='eng',audio=True,steps=[])]
source=read(work/'mkv-spu-seek-r18b-source/source-record.json');inputs=source['files']
assert all(digest(repo/p)==h for p,h in inputs.items())
base=work/'ordinary-ui-r14/runtime';protected=read(work/'ordinary-ui-r14/verification.json')['runtimeSha256']
for p in ['plugins/demux/libmkv_plugin.dll','plugins/codec/libsubsdec_plugin.dll','plugins/text_renderer/libfreetype_plugin.dll']:protected[p]=digest(base/p)
assert all(digest(base/p)==h for p,h in protected.items())
plugins={phase:work/('mkv-spu-seek-'+phase+'-r18b-bin/libmkv_plugin.dll') for phase in ['before','after']}
hashes={phase:digest(p) for phase,p in plugins.items()}
worker=work/'native-overlap-reference-r22.py';worker_hash=digest(worker)
fixtures={c['media']:digest(work/c['media']) for c in configs}
root=work/'native-overlap-reference-r22';assert not root.exists();root.mkdir();records=[]
kernel=ctypes.WinDLL('kernel32',use_last_error=True);kernel.SetErrorMode.argtypes=[ctypes.c_uint];kernel.SetErrorMode.restype=ctypes.c_uint;previous=kernel.SetErrorMode(0x8003)
try:
    for phase in ['before']:
        phase_dir=root/phase;phase_dir.mkdir()
        for cfg in configs:
            out=phase_dir/cfg['name'];out.mkdir();write(out/'scenario.json',cfg)
            runtime=out/'runtime';stage_runtime(base,runtime)
            path=runtime/'plugins/demux/libmkv_plugin.dll';path.unlink();shutil.copyfile(plugins[phase],path)
            identities={p:digest(runtime/p) for p in protected}
            assert identities['plugins/demux/libmkv_plugin.dll']==hashes[phase]
            record=dict(phase=phase,case=cfg['name'],status='starting',passed=False,candidateOnly=True,privateMkv=True,privateCore=False,forcedCleanupUsed=False,runtimeSha256=identities,protectedRuntimeSha256=protected,producerSha256=worker_hash,parentProducerSha256=digest(Path(__file__)),fixtureProducerSha256=digest(work/'generate-subtitle-overlap-r21.sh'),sourceRecordSha256=digest(work/'mkv-spu-seek-r18b-source/source-record.json'))
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
