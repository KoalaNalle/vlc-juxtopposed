from pathlib import Path
from hashlib import sha256
import ctypes,json,shutil,subprocess,sys
repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
record=read(work/'mkv-spu-seek-r18b-source/source-record.json');inputs=record['files']
assert record['sourceCount']==2189 and all(digest(repo/p)==h and digest(work/'source-mkv-r18b'/p)==h for p,h in inputs.items())
assert (work/'build-mkv-spu-seek-r18b-exit.txt').read_text().strip()=='0'
base=work/'ordinary-ui-r14/runtime';protected=read(work/'ordinary-ui-r14/verification.json')['runtimeSha256']
mkv_path='plugins/demux/libmkv_plugin.dll';protected[mkv_path]=digest(base/mkv_path)
assert all(digest(base/p)==h for p,h in protected.items())
worker=work/'native-mkv-seek-r19.py';worker_hash=digest(worker)
plugins={phase:work/('mkv-spu-seek-'+phase+'-r18b-bin/libmkv_plugin.dll') for phase in ['before','after']}
plugin_hashes={phase:digest(p) for phase,p in plugins.items()}
assert plugin_hashes['before']!=plugin_hashes['after']
root=work/'native-mkv-seek-r19';assert not root.exists();root.mkdir()
records=[]
kernel=ctypes.WinDLL('kernel32',use_last_error=True);kernel.SetErrorMode.argtypes=[ctypes.c_uint];kernel.SetErrorMode.restype=ctypes.c_uint;previous=kernel.SetErrorMode(0x8003)
try:
    for phase in ['before','after']:
        phase_dir=root/phase;phase_dir.mkdir()
        for name in ['original-wasapi','preserve-wasapi','original-noaudio']:
            out=phase_dir/name;out.mkdir();runtime=out/'runtime';stage_runtime(base,runtime)
            (runtime/mkv_path).unlink();shutil.copyfile(plugins[phase],runtime/mkv_path)
            hashes={p:digest(runtime/p) for p in protected};assert hashes[mkv_path]==plugin_hashes[phase]
            result=dict(phase=phase,case=name,status='starting',passed=False,candidateOnly=True,privateCore=False,privateMkv=True,forcedCleanupUsed=False,runtimeSha256=hashes,protectedOrdinaryRuntimeSha256=protected,producerSha256=worker_hash,parentProducerSha256=digest(Path(__file__)),sourceRecordSha256=digest(work/'mkv-spu-seek-r18b-source/source-record.json'))
            with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
                child=subprocess.Popen([sys.executable,str(worker),'--case',name,'--runtime',str(runtime),'--output',str(out)],cwd=runtime,env=environment(runtime,out),stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
                result.update(status='running',pid=child.pid);write(out/'process.json',result);print(phase,name,'PID',child.pid,flush=True)
                try:code=child.wait(timeout=150)
                except subprocess.TimeoutExpired:
                    result['status']='timeout-process-left-running';write(out/'process.json',result);raise SystemExit('Inspect exact owned PID; no automatic cleanup/restart')
            result.update(status='terminal',exitCode=code,normalExit=code==0,runtimeUnchanged=all(digest(runtime/p)==h for p,h in hashes.items()) and all(digest(base/p)==h for p,h in protected.items()),sourceInputsUnchanged=all(digest(repo/p)==h for p,h in inputs.items()),producerUnchanged=digest(worker)==worker_hash)
            write(out/'process.json',result);records.append(result)
            assert code==0,'Inspect terminal failure before continuing'
            observations=read(out/'investigation.json')['observations']
            print(phase,name,'normal terminal','subtitle pixels',[v.get('brightPixels') for v in observations],flush=True)
finally:kernel.SetErrorMode(previous)
write(root/'comparison.json',dict(passed=False,candidateOnly=True,repairAccepted=False,allProcessesNormal=all(r['normalExit'] for r in records),privateMkvHashes=plugin_hashes,sourceRecordSha256=digest(work/'mkv-spu-seek-r18b-source/source-record.json'),records=records))
