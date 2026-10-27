from pathlib import Path
from hashlib import file_digest
import subprocess,json,sys
r=Path('E:/Modding/vlc-juxtopposed');w=r/'build-windows/beta-recent-history-only-20261018';base=r/'build-windows/beta-network-artwork-20261018'
assert (w/'build-private-r4-exit.txt').read_text().strip()=='0'
def sha(p):
 with p.open('rb') as f:return file_digest(f,'sha256').hexdigest()
for name in ['ordinary-inputs-r4.json','private-inputs-r4.json']:
 manifest=json.loads((w/name).read_text());assert all(sha(r/key)==value for key,value in manifest.items()),name
command=[sys.executable,str(r/'modules/gui/qt/tests/native_ui/run_sidebar_pins_journey.py'),'--package',str(r/'build-windows/beta-sidebar-shell-20261018/ordinary-runtime-r3'),'--qa-gui',str(w/'native-ui-r4/.libs/libqt_plugin.dll'),'--fixtures',str(base/'window-r1/Owned Pins Ää 日本語'),'--inputs',str(w/'private-inputs-r4.json'),'--ordinary-inputs',str(w/'ordinary-inputs-r4.json'),'--output-dir',str(w/'window-r4'),'--recent-stream','--modes','history-off-optout','history-off-shutdown','populate','reopen','disabled','replacement','history-off']
(w/'native-launch-r4.json').write_text(json.dumps(command,indent=2)+'\n',encoding='utf-8',newline='\n')
subprocess.run(command,cwd=r,check=True)
report=json.loads((w/'window-r4/verification.json').read_text());assert report['passed']
for mode in ['populate','reopen','disabled','replacement','history-off','history-off-optout','history-off-shutdown']:
 log=(w/'window-r4'/mode/'vlc.log').read_text(encoding='utf-8',errors='replace')
 warnings=[line for line in log.splitlines() if 'qt warning:' in line]
 assert not warnings,(mode,warnings)
 if mode in ['populate','reopen','history-off']:assert report['cases'][mode]['journey']['recentDecodedHttpReopen']
assert report['cases']['history-off']['journey']['recentLibraryOffAccepted']
assert report['cases']['history-off-optout']['journey']['historyOptoutAccepted']
assert report['cases']['history-off-shutdown']['journey']['historyOpeningShutdownAccepted']
print('Native sidebar/Home/Recent passes all seven processes; Qt warnings zero.')