from pathlib import Path
from hashlib import sha256
import ctypes, json, shutil, subprocess, sys

repo = Path('C:/Modding/vlc-juxtopposed')
work = Path(__file__).resolve().parent
sys.path.insert(0, str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime, environment
read = lambda p: json.loads(p.read_text(encoding='utf-8-sig'))
digest = lambda p: sha256(p.read_bytes()).hexdigest()
def write(p, v): p.write_text(json.dumps(v, indent=2)+'\n', encoding='utf-8', newline='\n')
inputs = read(work/'inputs-r14.json')
assert len(inputs)>=2139 and all(digest(repo/p)==h for p,h in inputs.items())
base = work/'ordinary-ui-r4/runtime'
protected = read(work/'ordinary-ui-r4/verification.json')['runtimeSha256']
assert all(digest(base/p)==h for p,h in protected.items())
candidate = work/'core-r14/libvlccore.dll'
candidate_hash = digest(candidate)
worker = repo/'test/libvlc/native_paused_video_startup.py'
worker_hash = digest(worker)
root = work/'native-startup-ordinary-r14'
assert not root.exists(); root.mkdir()
definitions = [
    ('original-wasapi', 'media-r1/multi-language.mkv', 'wasapi', True, []),
    ('original-dummy', 'media-r1/multi-language.mkv', 'dummy', True, []),
    ('original-nospu-wasapi', 'media-r1/multi-language.mkv', 'wasapi', True, ['--no-spu']),
    ('preserve-wasapi', 'media-r8/preserve.mkv', 'wasapi', True, []),
    ('plain-wasapi', 'media-r1/plain.mkv', 'wasapi', True, []),
    ('original-noaudio', 'media-r1/multi-language.mkv', 'none', True, []),
    ('audio-only-wasapi', 'media-r12/audio-only.mka', 'wasapi', False, []),
    ('single-picture', 'media-r12/single-picture.mkv', 'none', True, []),
    ('delayed-video-wasapi', 'media-r12/delayed-video.mkv', 'wasapi', True, []),
]
fixtures = {p: digest(work/p) for _,p,_,_,_ in definitions}
records=[]
kernel=ctypes.WinDLL('kernel32',use_last_error=True)
kernel.SetErrorMode.argtypes=[ctypes.c_uint]; kernel.SetErrorMode.restype=ctypes.c_uint
previous=kernel.SetErrorMode(0x8003)
try:
    for name, media, audio, video, extra in definitions:
        case_dir=root/name; case_dir.mkdir()
        config=dict(name=name,media=str(work/media),audio=audio,video=video,extraOptions=extra)
        write(case_dir/'config.json',config)
        for variant in ['ordinary']:
            out=case_dir/variant; out.mkdir(); runtime=out/'runtime'
            stage_runtime(base,runtime)
            if variant=='ordinary':
                (runtime/'libvlccore.dll').unlink(); shutil.copyfile(candidate,runtime/'libvlccore.dll')
            hashes={p:digest(runtime/p) for p in protected}
            assert hashes['libvlccore.dll']==(candidate_hash if variant=='ordinary' else protected['libvlccore.dll'])
            record=dict(case=name,variant=variant,status='starting',passed=False,forcedCleanupUsed=False,runtimeSha256=hashes,producerSha256=worker_hash,parentProducerSha256=digest(Path(__file__)),sourceManifestSha256=digest(work/'inputs-r14.json'),privateCore=False)
            command=[sys.executable,str(worker),'--config',str(case_dir/'config.json'),'--runtime',str(runtime),'--output',str(out)]
            with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
                child=subprocess.Popen(command,cwd=runtime,env=environment(runtime,out),stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
                record.update(status='running',pid=child.pid); write(out/'process.json',record)
                print(name,variant,'PID',child.pid,flush=True)
                try: code=child.wait(timeout=65)
                except subprocess.TimeoutExpired:
                    record['status']='timeout-process-left-running';write(out/'process.json',record)
                    raise SystemExit('Inspect owned PID; no automatic cleanup or restart')
            result=read(out/'result.json') if (out/'result.json').exists() else {}
            record.update(status='terminal',exitCode=code,normalExit=code==0,passed=result.get('passed',False),runtimeUnchanged=all(digest(runtime/p)==h for p,h in hashes.items()))
            write(out/'process.json',record); records.append(record)
            print(name,variant,'terminal',code,'passed',record['passed'],flush=True)
            assert code==0, 'Inspect terminal child failure before continuing'
finally: kernel.SetErrorMode(previous)
unchanged=all(digest(repo/p)==h for p,h in inputs.items()) and all(digest(work/p)==h for p,h in fixtures.items()) and all(digest(base/p)==h for p,h in protected.items()) and digest(worker)==worker_hash
report=dict(candidateOnly=False,sourceRuntimeFixturesProducersUnchanged=unchanged,sourceManifestSha256=digest(work/'inputs-r14.json'),candidateCoreSha256=candidate_hash,allProcessesNormal=all(r['normalExit'] for r in records),records=records)
write(root/'comparison.json',report)
assert unchanged
