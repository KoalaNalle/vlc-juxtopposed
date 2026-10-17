from pathlib import Path
import ctypes,hashlib,json,os,subprocess,sys,time,xml.etree.ElementTree as ET
root=Path('E:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
sys.path.insert(0,str(root/'test/libvlc'))
from native_http_runtime import stage_runtime,environment,regenerate_cache
out=work/'standard-subtitles-r2';out.mkdir(exist_ok=False)
source=work/'ordinary-runtime-r2';runtime=out/'runtime'
def sha(p):
 with p.open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()
selected=['libvlccore.dll','plugins/gui/libqt_plugin.dll','plugins/text_renderer/libfreetype_plugin.dll']
before={n:sha(source/n) for n in selected}
assert sha(root/'modules/text_renderer/freetype/freetype.c')=='b7a780a1de25bcbcc8b1e7beeab03915844ef3d7ee922a79ecdf9ac367cfcf32'
assert before[selected[2]]=='d102b09767056a9acb6ab3b007c653c9561dba17857d5a8258d07242a5856ceb'
stage_runtime(source,runtime)
program=runtime/'test_qt_subtitles_preferences.exe';os.link(work/'standard-native-r1/test_qt_subtitles_preferences.exe',program)
regenerate_cache(runtime,out/'cache')
env=environment(runtime,out)
env.update(VLC_SUBTITLES_PREFERENCES_OUTPUT=str(out/'cases'),VLC_SUBTITLES_PREFERENCES_FIXTURES=str(root/'build-windows/subtitles-generated-fixtures-20261015'),VLC_SUBTITLES_CONSUMERS_VERIFIED='1')
command=[str(program),'-o',str(out/'qtest.xml')+',xunitxml','-o',str(out/'qtest.txt')+',txt']
(out/'launch.json').write_text(json.dumps({'command':command,'scope':'Current standard native subtitle preferences and rendered-pixel regressions. Prior unchanged original/fixed consumer oracle retained; ordinary repaired renderer source/binary identities verified.'},indent=2)+'\n',encoding='utf-8')
kernel=ctypes.WinDLL('kernel32');kernel.SetErrorMode.argtypes=[ctypes.c_uint];kernel.SetErrorMode.restype=ctypes.c_uint
previous=kernel.SetErrorMode(0x8003);forced=False;started=time.monotonic()
try:
 with (out/'stdout.txt').open('wb') as stdout,(out/'stderr.txt').open('wb') as stderr:
  child=subprocess.Popen(command,cwd=runtime,env=env,stdout=stdout,stderr=stderr,creationflags=subprocess.CREATE_NO_WINDOW)
  (out/'pid.txt').write_text(str(child.pid))
  try:code=child.wait(timeout=240)
  except subprocess.TimeoutExpired:forced=True;child.kill();code=child.wait(timeout=5)
finally:kernel.SetErrorMode(previous)
counts={}
if (out/'qtest.xml').exists():
 try:
  xml=ET.parse(out/'qtest.xml').getroot();counts={k:int(xml.attrib[k]) for k in ('tests','failures','errors','skipped')}
 except ET.ParseError:pass
report={'exitCode':code,'normalExit':code==0 and not forced,'forcedCleanupUsed':forced,'elapsedSeconds':time.monotonic()-started,'counts':counts,'runtimeSha256':{n:sha(runtime/n) for n in selected},'testExeSha256':sha(program),'sourceRuntimePreserved':before=={n:sha(source/n) for n in selected},'currentSourceHead':subprocess.check_output(['E:/Apps/Git/cmd/git.exe','rev-parse','HEAD'],cwd=root,text=True).strip()}
report['passed']=report['normalExit'] and report['sourceRuntimePreserved'] and counts=={'tests':38,'failures':0,'errors':0,'skipped':0}
(out/'verification.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n')
print('Native subtitles',counts,'normal exit',report['normalExit'],'accepted',report['passed'],flush=True)
raise SystemExit(0 if report['passed'] else 1)
