from pathlib import Path
from hashlib import sha256
import argparse,json,os,shutil,subprocess,sys
import xml.etree.ElementTree as ET
repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment
parser=argparse.ArgumentParser();parser.add_argument('--phase',choices=['before','after'],required=True);args=parser.parse_args()
def digest(p):return sha256(p.read_bytes()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
out=work/('navigation-'+args.phase+'-r3b');assert not out.exists();out.mkdir()
source=work/'preimage-r3' if args.phase=='before' else repo/'modules/gui/qt/player/qml'
sourceHashes={n:digest(source/n) for n in ['TracksListPage.qml','TracksPage.qml']}
base=repo/'build-windows/beta-queue-workflow-20261022/candidate-runtime-r23';runtime=out/'runtime'
stage_runtime(base,runtime)
exe=runtime/'test_tracks_cancel_navigation.exe';shutil.copyfile(work/'navigation-r3b-bin/test_tracks_cancel_navigation.exe',exe)
env=environment(runtime,out);env['VLC_TRACKS_SOURCE_DIR']=str(source);env.pop('VLC_PLUGIN_PATH',None);env['QT_QPA_PLATFORM']='offscreen';env['QT_QPA_FONTDIR']=str(repo/'modules/gui/qt/pixmaps/juxtopposed')
selected=['modules/gui/qt/tests/test_tracks_cancel_navigation.cpp','modules/gui/qt/tests/native_library/tracks-navigation.mk','modules/gui/qt/widgets/native/navigation_attached.cpp','modules/gui/qt/widgets/native/navigation_attached.hpp','modules/gui/qt/util/keyhelper.hpp']
result=dict(phase=args.phase,passed=False,status='starting',exeSha256=digest(exe),sourceDirectory=str(source),sourceSha256=sourceHashes,compiledInputsSha256={p:digest(repo/p) for p in selected},compiledScope='Actual NavigationAttached plus exact source cancellation/parent bindings; supplied hierarchy/event object/requestBack recipient. No popup lifetime, native models, media or ordinary desktop acceptance.',producerSha256=digest(Path(__file__)),forcedCleanupUsed=False)
write(out/'verification.json',result)
command=[str(exe),'-o',str(out/'result.xml')+',junitxml','-o',str(out/'result.txt')+',txt']
with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
    child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
    result.update(status='running',pid=child.pid);write(out/'verification.json',result)
    try:code=child.wait(timeout=60)
    except subprocess.TimeoutExpired:
        result.update(status='timeout-process-left-running');write(out/'verification.json',result);raise SystemExit('Inspect owned PID; no automatic cleanup or replay')
assert (out/'result.xml').exists(), f'Native setup failed: exit {code}; see preserved stderr';suite=ET.parse(out/'result.xml').getroot();tests=suite.findall('.//testcase');failures=[t.get('name') for t in tests if t.find('failure') is not None]
expectedNegative=args.phase=='before' and code==4 and len(tests)==6 and len(failures)==4
actualPass=code==0 and len(tests)==6 and not failures
result.update(status='terminal',exitCode=code,tests=len(tests),failures=failures,expectedNegativeVerified=expectedNegative,passed=actualPass,sourceInputsUnchanged=all(digest(source/n)==h for n,h in sourceHashes.items()),compiledInputsUnchanged=all(digest(repo/p)==h for p,h in result['compiledInputsSha256'].items()),exeUnchanged=digest(exe)==result['exeSha256'])
write(out/'verification.json',result);print(json.dumps(result,indent=2))
raise SystemExit(0 if expectedNegative or actualPass else 1)
