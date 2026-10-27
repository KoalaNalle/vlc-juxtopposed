from pathlib import Path
from hashlib import sha256
import ctypes,json,re,shutil,subprocess,sys,xml.etree.ElementTree as ET
repo=Path(__file__).resolve().parents[2]
work=Path(__file__).resolve().parent
revision,label,core_name,mode,binary_revision=sys.argv[1:]
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
manifest=work/f'inputs-{revision}.json'
inputs=read(manifest)
assert all(digest(repo/p)==h and digest(Path(inputs['sourceCopies'][p]))==h for p,h in inputs['files'].items())
base=work/'ui-r1/runtime'
expected=read(work/'ui-r1/verification.json')['runtimeSha256']
assert all(digest(base/p)==h for p,h in expected.items())
out=work/label
assert not out.exists()
out.mkdir()
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment
runtime=out/'runtime'
stage_runtime(base,runtime)
if core_name!='baseline':
    core=runtime/'libvlccore.dll'
    core.unlink()
    shutil.copyfile(work/core_name,core)
audio=runtime/'plugins/audio_output/libwasapi_plugin.dll'
audio.unlink()
shutil.copyfile(work/'wasapi-r15-bin/libwasapi_plugin.dll',audio)
identities={p:digest(runtime/p) for p in expected}
program='test_qt_player_subtitle_delay.exe'
shutil.copyfile(work/f'native-{binary_revision}-bin'/program,runtime/program)
env=environment(runtime,out);env.pop('VLC_PLUGIN_PATH',None)
media=repo/'build-windows/beta-subtitle-fps-20261024/media-r1'
fixtures={p.name:digest(p) for p in media.iterdir() if p.is_file()}
snapshots=out/'snapshots';snapshots.mkdir()
env.update(VLC_SUBTITLE_FIXTURE=str(media/'frame-marker.mp4'),
           VLC_TEST_PLUGIN_PATH=str(runtime/'plugins'),VLC_FPS_SNAPSHOTS=str(snapshots))
command=[str(runtime/program)]
if mode=='playing':command+=['associatedFpsRetimesPlayingCaption']
elif mode!='all':raise ValueError(mode)
command+=['-o',str(out/'results.xml')+',xml']
record=dict(status='starting',label=label,mode=mode,sourceManifestSha256=digest(manifest),
    producerSha256=digest(Path(__file__)),testExecutableSha256=digest(runtime/program),
    runtimeSha256=identities,fixtureSha256=fixtures,privateNativeFixture=True,
    ordinaryGuiAccepted=False,fullBetaAccepted=False,forcedCleanupUsed=False)
kernel=ctypes.WinDLL('kernel32',use_last_error=True)
kernel.SetErrorMode.argtypes=[ctypes.c_uint];kernel.SetErrorMode.restype=ctypes.c_uint
previousMode=kernel.SetErrorMode(0x8003)
try:
    with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
        child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
        record.update(status='running',pid=child.pid);write(out/'verification.json',record)
        print('Owned native FPS',label,'PID',child.pid,flush=True)
        try:record['exitCode']=child.wait(timeout=180)
        except subprocess.TimeoutExpired:
            record['status']='observation-timeout-process-left-running';write(out/'verification.json',record)
            raise SystemExit('Inspect owned PID; do not restart or clean up automatically')
finally:kernel.SetErrorMode(previousMode)
record.update(status='terminal',normalExit=record['exitCode']==0,
    sourceInputsUnchanged=all(digest(repo/p)==h for p,h in inputs['files'].items()),
    runtimeUnchanged=all(digest(base/p)==h and digest(runtime/p)==identities[p] for p,h in expected.items()),
    fixturesUnchanged=all(digest(media/p)==h for p,h in fixtures.items()),
    producerUnchanged=digest(Path(__file__))==record['producerSha256'])
if (out/'results.xml').exists():
    tree=ET.parse(out/'results.xml').getroot()
    record['functions']=[dict(name=f.attrib.get('name'),incidents=[dict(type=i.attrib.get('type'),
        description=i.findtext('Description','')) for i in f.findall('Incident')]) for f in tree.findall('TestFunction')]
write(out/'verification.json',record)
print(json.dumps({p:record[p] for p in ['status','exitCode','sourceInputsUnchanged','runtimeUnchanged','fixturesUnchanged','functions'] if p in record}),flush=True)
