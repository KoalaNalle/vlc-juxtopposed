from pathlib import Path
from hashlib import sha256
import ctypes, json, re, shutil, subprocess, sys
work=Path(__file__).resolve().parent
repo=work.parents[1]
label, core=sys.argv[1:]
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
inputs=read(work/'inputs-r15.json')['files']
extras=read(work/'controls-inputs-r15.json')
assert all(digest(repo/p)==h for p,h in {**inputs,**extras}.items())
assert (work/'build-controls-r12-exit.txt').read_text().strip()=='0'
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime, environment
out=work/label
assert not out.exists()
out.mkdir()
shutil.copyfile(Path(__file__),out/'producer.py')
runtime=out/'runtime'
stage_runtime(work/'native-final-r15/runtime',runtime)
if core=='before':
    (runtime/'libvlccore.dll').unlink()
    shutil.copyfile(work/'core-r4-bin/libvlccore.dll',runtime/'libvlccore.dll')
else: assert core=='after'
fixture=runtime/'plugins/controls-fixture'
fixture.mkdir()
shutil.copyfile(work/'controls-r13-bin/libcontrols_fixture_plugin.dll',fixture/'libcontrols_fixture_plugin.dll')
exe=runtime/'test_controls_input.exe'
shutil.copyfile(work/'controls-r13-bin/test_controls_input.exe',exe)
media=repo/'build-windows/beta-subtitle-fps-20261024/media-r1/frame-marker.mp4'
protected=read(work/'native-final-r15/verification.json')['runtimeSha256']
identities={p:digest(runtime/p) for p in protected}
identities['plugins/controls-fixture/libcontrols_fixture_plugin.dll']=digest(fixture/'libcontrols_fixture_plugin.dll')
identities['test_controls_input.exe']=digest(exe)
args=[str(exe),'bare',media.as_uri()]
(out/'launch.json').write_text(json.dumps(args,indent=2)+'\n',encoding='utf-8')
record=dict(status='starting',phase=core,privateCore=False,privateGui=False,
            privateWindowFixture=True,fullBetaAccepted=False,forcedCleanupUsed=False,
            runtimeSha256=identities,fixtureSha256=digest(media),sourceInputCount=len(inputs),
            sourceManifestSha256=digest(work/'inputs-r15.json'),
            extraInputsSha256=digest(work/'controls-inputs-r15.json'),producerSha256=digest(Path(__file__)))
kernel=ctypes.WinDLL('kernel32',use_last_error=True)
kernel.SetErrorMode.argtypes=[ctypes.c_uint]
kernel.SetErrorMode.restype=ctypes.c_uint
previous=kernel.SetErrorMode(0x8003)
try:
    with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
        child=subprocess.Popen(args,cwd=runtime,env=environment(runtime,out),
                               stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
        record.update(status='running',pid=child.pid)
        (out/'verification.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8')
        print(label,'PID',child.pid,flush=True)
        try: code=child.wait(timeout=90)
        except subprocess.TimeoutExpired:
            raise SystemExit('Inspect recorded process; no automatic cleanup/restart')
finally:
    kernel.SetErrorMode(previous)
lines=(out/'stdout.txt').read_text(errors='replace').splitlines()
checks=[line for line in lines if re.match(r'^ok \d+ ',line)]
record.update(status='terminal',exitCode=code,normalExit=code==0,checks=checks,
              sourceInputsUnchanged=all(digest(repo/p)==h for p,h in {**inputs,**extras}.items()),
              runtimeUnchanged=all(digest(runtime/p)==h for p,h in identities.items()),
              fixturesUnchanged=digest(media)==record['fixtureSha256'],
              producerUnchanged=digest(Path(__file__))==record['producerSha256'])
record['passed']=code==0 and len(checks)==13 and lines[-1]=='1..13'
(out/'verification.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8')
print(json.dumps({k:record[k] for k in ['status','exitCode','passed','sourceInputsUnchanged','runtimeUnchanged','fixturesUnchanged']}),flush=True)
if core=='after' and not record['passed']: raise SystemExit(code or 1)
