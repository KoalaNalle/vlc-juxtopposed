from pathlib import Path
from hashlib import sha256
import ctypes,json,re,subprocess,sys
repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import environment
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
inputs=read(work/'inputs-r14.json');assert all(digest(repo/p)==h for p,h in inputs.items())
runtime=work/'ordinary-ui-r14/runtime';identities=read(work/'ordinary-ui-r14/verification.json')['runtimeSha256']
assert all(digest(runtime/p)==h for p,h in identities.items())
producer=repo/'test/libvlc/native_paused_seek.py'
assert digest(producer)==read(repo/'doc/juxtopposed-references/native/home-paused-audio-repair-20261023/video-review.json')['producerSha256']
media=repo/'build-windows/beta-queue-workflow-20261022/ordinary-ui-r15/media/paused-seek-colors.mp4'
out=work/'native-video-r15';assert not out.exists();out.mkdir()
record=dict(status='starting',passed=False,forcedCleanupUsed=False,runtimeSha256=identities,mediaSha256=digest(media),producerSha256=digest(producer),parentProducerSha256=digest(Path(__file__)),sourceManifestSha256=digest(work/'inputs-r14.json'))
kernel=ctypes.WinDLL('kernel32',use_last_error=True);kernel.SetErrorMode.argtypes=[ctypes.c_uint];kernel.SetErrorMode.restype=ctypes.c_uint;previous=kernel.SetErrorMode(0x8003)
try:
    with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
        child=subprocess.Popen([sys.executable,str(producer),'--package',str(runtime),'--media',str(media),'--output-dir',str(out)],cwd=out,env=environment(runtime,out),stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
        record.update(status='running',pid=child.pid);write(out/'process.json',record);print('Owned native video PID',child.pid,flush=True)
        try:code=child.wait(timeout=65)
        except subprocess.TimeoutExpired:
            record['status']='timeout-process-left-running';write(out/'process.json',record);raise SystemExit('Inspect owned PID; no automatic cleanup/restart')
finally:kernel.SetErrorMode(previous)
lines=(out/'native.log').read_text(encoding='utf-8',errors='replace').splitlines()
errors=[l for l in lines if re.search(r'\berror:|Assertion failed|ReferenceError|TypeError|Binding loop',l)]
closing=[i for i,l in enumerate(lines) if l=='main error: window closed']
close_ok=len(closing)==1 and any('main debug: exiting' in l for l in lines[closing[0]+1:closing[0]+12])
observations=read(out/'results.json') if (out/'results.json').exists() else []
record.update(status='terminal',exitCode=code,normalExit=code==0,passed=code==0 and len(observations)==6 and errors==['main error: window closed'] and close_ok,diagnostics=errors,closeNotifyAtTeardown=close_ok,warnings=[l for l in lines if 'warning:' in l],runtimeUnchanged=all(digest(runtime/p)==h for p,h in identities.items()),sourceUnchanged=all(digest(repo/p)==h for p,h in inputs.items()),mediaUnchanged=digest(media)==record['mediaSha256'])
write(out/'process.json',record)
print(json.dumps({k:record[k] for k in ['normalExit','passed','diagnostics','runtimeUnchanged','sourceUnchanged']},indent=2))
assert record['passed'] and record['runtimeUnchanged'] and record['sourceUnchanged'] and record['mediaUnchanged']
