from pathlib import Path
from hashlib import sha256
import argparse,ctypes,json,re,shutil,subprocess,sys,xml.etree.ElementTree as ET
repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
sys.path.insert(0,str(repo/'test/libvlc'))
from native_http_runtime import stage_runtime,environment,regenerate_cache
def digest(p):return sha256(p.read_bytes()).hexdigest()
def read(p):return json.loads(p.read_text(encoding='utf-8'))
parser=argparse.ArgumentParser();parser.add_argument('--old-core',action='store_true');parser.add_argument('--case',choices=['full','startup-playing','startup-paused'],default='full');args=parser.parse_args()
inputs=read(work/'inputs-startup-r49.json')
def unchanged():return all(digest(repo/p)==h for p,h in inputs.items())
assert unchanged() and (work/'build-startup-r49-exit.txt').read_text().strip()=='0'
out=work/('backend-startup-'+('old-' if args.old_core else 'new-')+args.case+'-r49');out.mkdir();runtime=out/'runtime'
stage_runtime(repo/'build-windows/beta-precise-seek-20261021/ordinary-r8/runtime',runtime)
for name,path in [('libvlccore.dll',work/('native-core-r30' if args.old_core else 'native-core-r49')/'libvlccore.dll'),('plugins/gui/libqt_plugin.dll',work/'ordinary-r38/libqt_plugin.dll')]:
    target=runtime/name;target.unlink();shutil.copyfile(path,target)
exe='test_qt_timeline_thumbnails.exe';shutil.copyfile(work/'native-backend-r49'/exe,runtime/exe)
for p in (work/'runtime-deps').glob('*.dll'):
    if not (runtime/p.name).exists():shutil.copyfile(p,runtime/p.name)
regenerate_cache(runtime,out/'cache')
env=environment(runtime,out);env.pop('VLC_PLUGIN_PATH',None)
env.update(VLC_THUMBNAIL_GEOMETRY_DIR=str(work/'geometry-fixtures-r26'),VLC_THUMBNAIL_GEOMETRY_OUTPUT=str(out/'geometry'),VLC_THUMBNAIL_FIXTURE=str(work/'fixtures/preview-colors.mp4'),VLC_THUMBNAIL_PLAYBACK_DIR=str(work/'resource-fixtures-r46'),VLC_THUMBNAIL_STARTUP_OUTPUT=str(out/'startup'),QT_FORCE_STDERR_LOGGING='1',QT_MESSAGE_PATTERN='OWNED_QT_DIAG|%{type}|%{message}')
command=[str(runtime/exe)]
if args.case!='full':command+=['previewIgnoresPlaybackStartupPause:'+args.case]
command+=['-o',str(out/'results.xml')+',junitxml','-o',str(out/'results.txt')+',txt']
identities={n:digest(runtime/n) for n in [exe,'libvlccore.dll','plugins/gui/libqt_plugin.dll','plugins/demux/libmp4_plugin.dll']}
result=dict(status='starting',case=args.case,oldCore=args.old_core,passed=False,forcedCleanupUsed=False,identities=identities,sourceInputsSha256=digest(work/'inputs-startup-r49.json'),runnerSha256=digest(Path(__file__)),mediaSha256={p.name:digest(p) for p in (work/'resource-fixtures-r46').iterdir() if p.is_file()})
def save(): (out/'verification.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
kernel=ctypes.WinDLL('kernel32',use_last_error=True);kernel.SetErrorMode.argtypes=[ctypes.c_uint];kernel.SetErrorMode.restype=ctypes.c_uint;previous=kernel.SetErrorMode(0x8003)
try:
    with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
        child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
        result.update(status='running',pid=child.pid);save();print('Owned startup-pause test',args.case,'oldCore',args.old_core,'PID',child.pid,flush=True)
        try:result['exitCode']=child.wait(timeout=30 if args.old_core and args.case=='startup-paused' else 90)
        except subprocess.TimeoutExpired:
            text=(out/'results.txt').read_text(encoding='utf-8',errors='replace') if (out/'results.txt').exists() else ''
            expected=args.old_core and args.case=='startup-paused' and 'FAIL!' in text and '!preview->loading()' in text
            result.update(status='timeout_process_left_running',normalExit=False,expectedStartupPauseFailure=expected);save()
            if not expected:raise SystemExit('Unexpected timeout; owned PID recorded and left running')
            symbols=out/'local-symbols';symbols.mkdir()
            debugger=repo/'build-windows/homecoming-20261018/tools/WinDbg/amd64/cdb.exe'
            with (out/'debugger-output.txt').open('wb') as debug:
                d=subprocess.run([str(debugger),'-pv','-sins','-y',str(symbols),'-p',str(child.pid),'-logo',str(out/'shutdown-stack.txt'),'-c','~* kb; .detach; q'],stdout=debug,stderr=debug,timeout=20,creationflags=subprocess.CREATE_NO_WINDOW)
            assert d.returncode==0
            stack=(out/'shutdown-stack.txt').read_text(errors='replace')
            assert 'vlc_input_decoder_DecodeWithStatus' in stack and 'DecoderThread' in stack and 'vlc_preparser_Delete' in stack
            child.kill();result.update(exitCode=child.wait(timeout=10),forcedCleanupUsed=True,status='terminal_expected_negative',normalExit=False,negativeVerified=True)
finally:kernel.SetErrorMode(previous)
result.setdefault('normalExit',result['exitCode']==0)
result.update(sourceInputsUnchanged=unchanged(),identitiesUnchanged=all(digest(runtime/p)==h for p,h in identities.items()))
if result.get('negativeVerified'):
    result['scope']='Expected old-Core startup-paused failure; native assertion timed out and destructor blocked in the saved thumbnail FIFO/paused-decoder cycle. Owned process forcibly cleaned up after debugger detached. This is not a normal or passing application exit.'
else:
    root=ET.parse(out/'results.xml').getroot();suites=[root] if root.tag=='testsuite' else list(root.iter('testsuite'))
    counts={k:sum(int(s.attrib.get(k,0)) for s in suites) for k in ['tests','failures','errors','skipped']}
    result.update(status='terminal',counts=counts,passed=result['normalExit'] and counts['tests']==(19 if args.case=='full' else 3) and all(counts[k]==0 for k in ['failures','errors','skipped']) and result['sourceInputsUnchanged'] and result['identitiesUnchanged'])
save();print(json.dumps({k:result.get(k) for k in ['case','oldCore','status','passed','negativeVerified','counts','exitCode','forcedCleanupUsed']},indent=2))
assert result.get('negativeVerified') or result['passed']
