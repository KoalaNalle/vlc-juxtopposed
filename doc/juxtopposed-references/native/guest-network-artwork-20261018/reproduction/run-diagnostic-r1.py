from pathlib import Path
from hashlib import file_digest
import json,subprocess,sys
r=Path('E:/Modding/vlc-juxtopposed');w=r/'build-windows/beta-network-artwork-20261018';old=r/'build-windows/beta-sidebar-pins-20261018'
assert (w/'build-diagnostic-r2-exit.txt').read_text().strip()=='0'
def sha(p):
 with p.open('rb') as f:return file_digest(f,'sha256').hexdigest()
names=list(json.loads((old/'private-inputs-r7.json').read_text()))+[(w/name).relative_to(r).as_posix() for name in ['diagnostic.mk','color_svg_image_provider_diagnostic.cpp','build-diagnostic-r2.sh']]
inputs={name:sha(r/name) for name in names};(w/'diagnostic-inputs.json').write_text(json.dumps(inputs,indent=2)+'\n')
command=[sys.executable,str(r/'modules/gui/qt/tests/native_ui/run_sidebar_pins_journey.py'),'--package',str(old/'ordinary-runtime-r6'),'--qa-gui',str(w/'native-diagnostic-r1/.libs/libqt_plugin.dll'),'--fixtures',str(old/'window-r4/Owned Pins Ää 日本語'),'--inputs',str(w/'diagnostic-inputs.json'),'--ordinary-inputs',str(old/'ordinary-inputs-r6.json'),'--output-dir',str(w/'diagnostic-window-r1'),'--modes','populate']
(w/'diagnostic-launch.json').write_text(json.dumps(command,indent=2)+'\n',encoding='utf-8')
result=subprocess.run(command,cwd=r)
log=(w/'diagnostic-window-r1/populate/vlc.log').read_text(encoding='utf-8',errors='replace')
print('Diagnostic only; not acceptance. Caller lines:')
for line in log.splitlines():
 if 'OWNED_EMPTY' in line or 'SVGColorizer:' in line or 'No file name' in line: print(line)
raise SystemExit(result.returncode)