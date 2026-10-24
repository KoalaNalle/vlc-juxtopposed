from pathlib import Path
from hashlib import file_digest
import os,json,subprocess,sys
r=Path('E:/Modding/vlc-juxtopposed');w=r/'build-windows/beta-shared-strip-20261018';base=r/'build-windows/beta-recent-history-only-20261018'
assert (w/'build-ordinary-r1-exit.txt').read_text().strip()=='0'
def sha(p):
 with p.open('rb') as f:return file_digest(f,'sha256').hexdigest()
manifest=json.loads((w/'ordinary-inputs-r5.json').read_text());assert all(sha(r/name)==value for name,value in manifest.items())
source=base/'ordinary-runtime-r1';runtime=w/'ordinary-runtime-r1';names=['libvlccore.dll','plugins/gui/libqt_plugin.dll','plugins/misc/libmedialibrary_plugin.dll'];before={name:sha(source/name) for name in names}
sys.path.insert(0,str(r/'test/libvlc'));from native_http_runtime import stage_runtime,regenerate_cache
stage_runtime(source,runtime);target=runtime/'plugins/gui/libqt_plugin.dll';target.unlink();os.link(w/'ordinary-native-r1/libqt_plugin.dll',target)
regenerate_cache(runtime,w/'ordinary-cache-r1')
with target.open('rb') as f:
 tail=b''
 while chunk:=f.read(1024*1024):
  data=tail+chunk
  for marker in [b'VLC_OWNED_UI_REPORT',b'VLC_OWNED_RECENT_URL',b'VLC_OWNED_TITLEBAR_ASSETS',b'OWNED_EMPTY_SVG_PATH']:assert marker not in data
  tail=data[-128:]
assert all(sha(source/name)==value for name,value in before.items())
report=dict(sourceRuntimeUnchanged=True,runtimeSha256={name:sha(runtime/name) for name in names},ordinaryInputsUnchanged=True,ordinaryInputEntries=len(manifest),privateQaMarkersAbsent=True,diagnosticMarkerAbsent=True,guiBytes=target.stat().st_size)
(w/'ordinary-runtime-r1.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n')
subprocess.run([sys.executable,str(w/'run-ordinary-r1.py')],cwd=r,check=True)
warnings={p.parent.name:[line for line in p.read_text(encoding='utf-8',errors='replace').splitlines() if 'qt warning:' in line] for p in (w/'ordinary-playback-r1').glob('*/vlc.log')}
assert len(warnings)==5 and not any(warnings.values()),warnings
(w/'ordinary-qt-warnings-r1.json').write_text(json.dumps(warnings,indent=2)+'\n',encoding='utf-8',newline='\n')
assert all(sha(source/name)==value for name,value in before.items())
print('Five ordinary playback cases pass; Qt warnings zero; original runtime preserved; no test/diagnostic markers.')
