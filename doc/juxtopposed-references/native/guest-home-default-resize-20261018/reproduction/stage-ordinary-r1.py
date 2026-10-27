from pathlib import Path
import hashlib,json,os,sys
r=Path('E:/Modding/vlc-juxtopposed');w=r/'build-windows/beta-home-edges-20261018'
assert (w/'build-ordinary-r1-exit.txt').read_text().strip()=='0'
assert (w/'build-private-r4-exit.txt').read_text().strip()=='0'
sys.path.insert(0,str(r/'test/libvlc'))
from native_http_runtime import stage_runtime,regenerate_cache
old=r/'build-windows/beta-home-content-20261017/ordinary-runtime-core-r1';new=w/'ordinary-runtime-r1'
stage_runtime(old,new);target=new/'plugins/gui/libqt_plugin.dll';target.unlink();os.link(w/'ordinary-native-r1/libqt_plugin.dll',target)
regenerate_cache(new,w/'ordinary-cache-r1')
def sha(p):
 with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
selected=['libvlccore.dll','plugins/gui/libqt_plugin.dll','plugins/misc/libmedialibrary_plugin.dll']
report=dict(runtimeSha256={n:sha(new/n) for n in selected},sourceGuiPreserved=sha(old/'plugins/gui/libqt_plugin.dll')=='800199d41ccd7814fcdd49c4233323cdf8678fc5bce43722bc877592381eabf0',ordinaryGuiStrippedDebug=True)
assert report['sourceGuiPreserved'];assert report['runtimeSha256'][selected[0]]=='0cdebe2f6950939228eac94304393ef946b1947c80d1cbb8a8f83abe546a8072'
with target.open('rb') as f:
 tail=b''
 while chunk:=f.read(1024*1024):
  value=tail+chunk;assert b'VLC_OWNED_UI_REPORT' not in value;tail=value[-128:]
(w/'ordinary-runtime-r1.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n');print('Fresh immutable ordinary candidate staged on E:')
