from pathlib import Path
import ctypes,hashlib,json,os,subprocess,sys,time
root=Path('E:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
sys.path.insert(0,str(root/'test/libvlc'))
from native_http_runtime import stage_runtime,environment,regenerate_cache
out=work/'player-relocated-r3';out.mkdir(exist_ok=False)
assert (work/'build-player-relocated-r1-exit.txt').read_text().strip()=='0'
def digest(p):
 with p.open('rb') as s:return hashlib.file_digest(s,'sha256').hexdigest()
frozen=json.loads((work/'player-relocation-inputs-r1.json').read_text())
assert all(digest(root/p)==sha for p,sha in frozen.items())
report=dict(passed=False,activePid=None,cases={},oneHeavyTaskAtATime=True,mockDemuxSha256=digest(work/'player-mock-r1/libdemux_mock_plugin.dll'))
def save():(out/'verification.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n')
kernel=ctypes.WinDLL('kernel32');kernel.SetErrorMode.argtypes=[ctypes.c_uint];kernel.SetErrorMode.restype=ctypes.c_uint
previous=kernel.SetErrorMode(0x8003)
try:
 for variant in ['candidate','prior']:
  base=out/variant;base.mkdir();runtime=base/'runtime';stage_runtime(work/'ordinary-runtime-core-r1',runtime)
  if variant=='prior':
   (runtime/'libvlccore.dll').unlink();os.link(root/'build-windows/beta-home-20261017/ordinary-runtime-r2/libvlccore.dll',runtime/'libvlccore.dll')
  os.link(work/'test-runtime-deps/libssp-0.dll',runtime/'libssp-0.dll')
  for name in ['lifecycle','pause','seeks','media','timers']:
   file='test_src_player_'+name+'.exe';os.link(work/'player-native-relocated-r1'/file,runtime/file)
  os.link(work/'player-mock-r1/libdemux_mock_plugin.dll',runtime/'plugins/demux/libdemux_mock_plugin.dll')
  os.link(work/'player-modules-r2/.libs/libplayer_fixture_plugin.dll',runtime/'plugins/audio_output/libplayer_fixture_plugin.dll')
  regenerate_cache(runtime,base/'cache')
  for name in (['pause','timers'] if variant=='prior' else ['pause','lifecycle','seeks','media','timers']):
   case=base/name;case.mkdir();env=environment(runtime,case);env.update(VLC_TEST_PLUGIN_PATH=str(runtime/'plugins'),QT_ENABLE_REGEXP_JIT='1')
   command=[str(runtime/('test_src_player_'+name+'.exe'))];result=dict(command=command,forcedCleanupUsed=False,coreSha256=digest(runtime/'libvlccore.dll'));report['cases'][variant+'-'+name]=result
   with (case/'stdout.txt').open('wb') as stdout,(case/'stderr.txt').open('wb') as stderr:
    start=time.monotonic();child=subprocess.Popen(command,cwd=case,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW);report['activePid']=child.pid;report['activeCase']=str(case);save()
    try:code=child.wait(timeout=75)
    except subprocess.TimeoutExpired:result['forcedCleanupUsed']=True;child.kill();code=child.wait(timeout=5)
    result.update(exitCode=code,normalExit=code==0 and not result['forcedCleanupUsed'],elapsedSeconds=time.monotonic()-start);report['activePid']=None;save()
   print(variant,name,'PASS' if result['normalExit'] else 'FAIL',flush=True)
   if not result['normalExit']:break
 report['passed']=len(report['cases'])==7 and all(c['normalExit'] for c in report['cases'].values());save()
finally:kernel.SetErrorMode(previous)
raise SystemExit(0 if report['passed'] else 1)
