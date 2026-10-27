from pathlib import Path
from hashlib import file_digest
import json,subprocess,sys
r=Path('E:/Modding/vlc-juxtopposed');w=r/'build-windows/beta-shared-strip-20261018'
assert (w/'build-private-r5-exit.txt').read_text().strip()=='0'
def sha(p):
 with p.open('rb') as f:return file_digest(f,'sha256').hexdigest()
for name in ['ordinary-inputs-r5.json','private-inputs-r5.json','component-inputs-r5.json']:
 m=json.loads((w/name).read_text());assert all(sha(r/k)==v for k,v in m.items()),name
modes=['titlebar-traditional','titlebar-media-title','titlebar-empty','titlebar-media','titlebar-video','titlebar-close','history-off-optout','history-off-shutdown','populate','reopen','disabled','replacement','history-off']
command=[sys.executable,str(r/'modules/gui/qt/tests/native_ui/run_sidebar_pins_journey.py'),'--package',str(r/'build-windows/beta-recent-history-only-20261018/ordinary-runtime-r1'),'--qa-gui',str(w/'native-ui-r5/.libs/libqt_plugin.dll'),'--fixtures',str(r/'build-windows/beta-network-artwork-20261018/window-r1/Owned Pins Ää 日本語'),'--inputs',str(w/'private-inputs-r5.json'),'--ordinary-inputs',str(w/'ordinary-inputs-r5.json'),'--output-dir',str(w/'window-r5'),'--recent-stream','--modes',*modes]
(w/'native-launch-r5.json').write_text(json.dumps(command,indent=2)+'\n',newline='\n')
subprocess.run(command,cwd=r,check=True)
a=json.loads((w/'window-r5/verification.json').read_text());assert a['passed']
for mode in modes:
 case=a['cases'][mode];assert case['normalExit'] and case['journey']['passed'] and not case['diagnostics']
 assert 'qt warning:' not in (w/'window-r5'/mode/'vlc.log').read_text(errors='replace')
 if mode.startswith('titlebar-') and mode!='titlebar-traditional':assert case['journey']['nativeTitlebarAccepted']
assert a['cases']['titlebar-close']['journey']['nativeCloseTriggered']
assert a['cases']['titlebar-traditional']['journey']['nativeTraditionalHeaderAccepted']
assert a['cases']['titlebar-media-title']['journey']['nativeTitleTransitionsAccepted']
print('All13 sequential native processes pass; all Qt warnings zero; title/window controls plus sidebar/history regression accepted.')
