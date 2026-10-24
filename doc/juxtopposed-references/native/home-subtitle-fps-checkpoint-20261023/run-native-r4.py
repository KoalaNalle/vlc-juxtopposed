from pathlib import Path
from hashlib import sha256
import argparse,ctypes,json,shutil,subprocess,sys,xml.etree.ElementTree as ET

work=Path(__file__).resolve().parent
repo=work.parent.parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
parser=argparse.ArgumentParser()
parser.add_argument('phase',choices=['before','after'])
args=parser.parse_args()
manifest=work/('inputs-'+args.phase+'-r4.json')
inputs=read(manifest)
assert all(digest(repo/p)==h and digest(Path(inputs['sourceCopies'][p]))==h for p,h in inputs['files'].items())
assert (work/('build-native-'+args.phase+'-r4-exit.txt')).read_text().strip()=='0'
preparation=read(work/'preparation-r4.json')
base=repo/'build-windows/beta-subtitle-accessibility-20261023/ui-frame-marker.mp4-r3/runtime'
assert all(digest(base/p)==h for p,h in preparation['runtimeSha256'].items())
out=work/('native-'+args.phase+'-r4')
assert not out.exists()
out.mkdir()
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment
runtime=out/'runtime'
stage_runtime(base,runtime)
program='test_qt_player_subtitle_delay.exe'
shutil.copyfile(work/('native-'+args.phase+'-r4-bin')/program,runtime/program)
env=environment(runtime,out)
env.pop('VLC_PLUGIN_PATH',None)
env.update(VLC_SUBTITLE_FIXTURE=str(work/'media-r1/frame-marker.mp4'),
           VLC_TEST_PLUGIN_PATH=str(runtime/'plugins'))
command=[str(runtime/program)]
if args.phase=='before':command+=['initialAssociatedFpsMatchesNative']
command+=['-o',str(out/'results.xml')+',xml']
record=dict(status='starting',phase=args.phase,sourceManifestSha256=digest(manifest),
            producerSha256=digest(Path(__file__)),testExecutableSha256=digest(runtime/program),
            runtimeSha256=preparation['runtimeSha256'],privateNativeFixture=True,
            ordinaryGuiAccepted=False,fullBetaAccepted=False,forcedCleanupUsed=False)
kernel=ctypes.WinDLL('kernel32',use_last_error=True)
kernel.SetErrorMode.argtypes=[ctypes.c_uint]
kernel.SetErrorMode.restype=ctypes.c_uint
previousMode=kernel.SetErrorMode(0x8003)
try:
    with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
        child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
        record.update(status='running',pid=child.pid)
        write(out/'verification.json',record)
        print('Owned native FPS',args.phase,'PID',child.pid,flush=True)
        try:record['exitCode']=child.wait(timeout=180)
        except subprocess.TimeoutExpired:
            record['status']='observation-timeout-process-left-running'
            write(out/'verification.json',record)
            raise SystemExit('Inspect owned PID; no automatic cleanup/restart')
finally:kernel.SetErrorMode(previousMode)
record.update(status='terminal',sourceInputsUnchanged=all(digest(repo/p)==h for p,h in inputs['files'].items()),
              runtimeUnchanged=all(digest(base/p)==h and digest(runtime/p)==h for p,h in preparation['runtimeSha256'].items()),
              fixturesUnchanged=all(digest(work/'media-r1'/p)==h for p,h in preparation['fixtureSha256'].items()))
if (out/'results.xml').exists():
    tree=ET.parse(out/'results.xml').getroot()
    record['functions']=[dict(name=f.attrib.get('name'),incidents=[dict(type=i.attrib.get('type'),description=i.findtext('Description','')) for i in f.findall('Incident')]) for f in tree.findall('TestFunction')]
write(out/'verification.json',record)
print(json.dumps({p:record[p] for p in ['status','exitCode','sourceInputsUnchanged','runtimeUnchanged','fixturesUnchanged','functions'] if p in record}),flush=True)
