from pathlib import Path
import hashlib,json,subprocess,sys
r=Path('E:/Modding/vlc-juxtopposed');w=r/'build-windows/beta-home-edges-20261018'
assert (w/'build-private-r4-exit.txt').read_text().strip()=='0'
for n,v in json.loads((w/'private-inputs-r4.json').read_text()).items():assert hashlib.sha256((r/n).read_bytes()).hexdigest()==v,n
command=[sys.executable,r/'modules/gui/qt/tests/native_ui/run_home_content_journey.py','--package',w/'ordinary-runtime-r1','--qa-gui',w/'native-ui-r4/.libs/libqt_plugin.dll','--fixtures',r/'build-windows/beta-home-content-20261017/fixtures','--keep-window-size','--modes','populate','reopen','edges','--output-dir',w/'window-r4']
(w/'launch-r4.json').write_text(json.dumps([str(x) for x in command],indent=2)+'\n',encoding='utf-8')
raise SystemExit(subprocess.call([str(x) for x in command],cwd=r))
