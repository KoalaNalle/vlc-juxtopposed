from pathlib import Path
from hashlib import file_digest
import json,os,subprocess,sys
r=Path('E:/Modding/vlc-juxtopposed');w=r/'build-windows/beta-network-artwork-20261018';old=r/'build-windows/beta-sidebar-pins-20261018'
assert (w/'build-fix-r1-exit.txt').read_text().strip()=='0'
def sha(p):
 with p.open('rb') as f:return file_digest(f,'sha256').hexdigest()
for manifest in ['ordinary-inputs-r1.json','private-inputs-r1.json']:
 data=json.loads((w/manifest).read_text());assert all(sha(r/name)==value for name,value in data.items())
sys.path.insert(0,str(r/'test/libvlc'))
from native_http_runtime import stage_runtime,regenerate_cache
source=old/'ordinary-runtime-r6';runtime=w/'ordinary-runtime-r1'
names=['libvlccore.dll','plugins/gui/libqt_plugin.dll','plugins/misc/libmedialibrary_plugin.dll']
before={name:sha(source/name) for name in names}
stage_runtime(source,runtime)
target=runtime/'plugins/gui/libqt_plugin.dll';target.unlink();os.link(w/'ordinary-native-r1/libqt_plugin.dll',target)
regenerate_cache(runtime,w/'ordinary-cache-r1')
for binary in [target,w/'native-ui-r1/.libs/libqt_plugin.dll']:
 with binary.open('rb') as f:
  tail=b''
  while chunk:=f.read(1024*1024):
   data=tail+chunk;assert b'OWNED_EMPTY_SVG_PATH' not in data
   if binary==target:assert b'VLC_OWNED_UI_REPORT' not in data
   tail=data[-128:]
assert all(sha(source/name)==value for name,value in before.items())
staging=dict(sourceRuntimeUnchanged=True,runtimeSha256={name:sha(runtime/name) for name in names},privateQaMarkerAbsent=True,diagnosticMarkerAbsent=True,guiBytes=target.stat().st_size)
(w/'ordinary-runtime-r1.json').write_text(json.dumps(staging,indent=2)+'\n')
command=[sys.executable,str(r/'modules/gui/qt/tests/native_ui/run_sidebar_pins_journey.py'),'--package',str(runtime),'--qa-gui',str(w/'native-ui-r1/.libs/libqt_plugin.dll'),'--fixtures',str(old/'window-r4/Owned Pins Ää 日本語'),'--inputs',str(w/'private-inputs-r1.json'),'--ordinary-inputs',str(w/'ordinary-inputs-r1.json'),'--output-dir',str(w/'window-r1')]
(w/'positive-launch.json').write_text(json.dumps(command,indent=2)+'\n',encoding='utf-8')
subprocess.run(command,cwd=r,check=True)
warnings={}
for mode in ['populate','reopen','disabled','replacement']:
 log=(w/'window-r1'/mode/'vlc.log').read_text(encoding='utf-8',errors='replace')
 warnings[mode]=[line for line in log.splitlines() if 'qt warning:' in line]
assert not any(warnings.values()),warnings
(w/'native-qt-warnings.json').write_text(json.dumps(warnings,indent=2)+'\n')
print('Four native processes: all Qt warnings zero.',flush=True)
smoke=(old/'run-ordinary-r2.py').read_text()
for a,b in [('beta-sidebar-pins-20261018/ordinary-playback-r2','beta-network-artwork-20261018/ordinary-playback-r1'),('beta-sidebar-pins-20261018/ordinary-runtime-r6','beta-network-artwork-20261018/ordinary-runtime-r1'),('beta-sidebar-pins-20261018/ordinary-native-r6','beta-network-artwork-20261018/ordinary-native-r1')]:
 assert a in smoke;smoke=smoke.replace(a,b)
(w/'run-ordinary-r1.py').write_text(smoke,encoding='utf-8',newline='\n')
subprocess.run([sys.executable,str(w/'run-ordinary-r1.py')],cwd=r,check=True)
ordinary_warnings={p.parent.name:[line for line in p.read_text(encoding='utf-8',errors='replace').splitlines() if 'qt warning:' in line] for p in (w/'ordinary-playback-r1').glob('*/vlc.log')}
assert len(ordinary_warnings)==5 and not any(ordinary_warnings.values()),ordinary_warnings
(w/'ordinary-qt-warnings.json').write_text(json.dumps(ordinary_warnings,indent=2)+'\n')
assert all(sha(source/name)==value for name,value in before.items())
print('Ordinary five-case replay: all Qt warnings zero, previous runtime unchanged.',flush=True)