from pathlib import Path
from hashlib import file_digest
import json,subprocess,sys
r=Path('E:/Modding/vlc-juxtopposed');w=r/'build-windows/beta-network-artwork-20261018';old=r/'build-windows/beta-sidebar-pins-20261018'
p=r/'modules/gui/qt/tests/native_ui/run_sidebar_pins_journey.py';text=p.read_text()
needle="'SyntaxError','QSignalSpy:')"
assert text.count(needle)==1
text=text.replace(needle,"'SyntaxError','QSignalSpy:','SVGColorizer:','QFSFileEngine::open: No file name specified')")
p.write_text(text,encoding='utf-8',newline='\n')
def sha(p):
 with p.open('rb') as f:return file_digest(f,'sha256').hexdigest()
for source,target in [('ordinary-inputs-r6.json','negative-ordinary-inputs.json'),('private-inputs-r7.json','negative-private-inputs.json')]:
 data=json.loads((old/source).read_text());changed=[name for name,value in data.items() if sha(r/name)!=value]
 assert all(name==p.relative_to(r).as_posix() for name in changed),changed
 data={name:sha(r/name) for name in data};(w/target).write_text(json.dumps(data,indent=2)+'\n')
command=[sys.executable,str(p),'--package',str(old/'ordinary-runtime-r6'),'--qa-gui',str(old/'native-ui-r7/.libs/libqt_plugin.dll'),'--fixtures',str(old/'window-r4/Owned Pins Ää 日本語'),'--inputs',str(w/'negative-private-inputs.json'),'--ordinary-inputs',str(w/'negative-ordinary-inputs.json'),'--output-dir',str(w/'negative-window-r1'),'--modes','populate']
(w/'negative-launch.json').write_text(json.dumps(command,indent=2)+'\n',encoding='utf-8')
result=subprocess.run(command,cwd=r)
report=json.loads((w/'negative-window-r1/verification.json').read_text())
case=report['cases']['populate']
assert result.returncode!=0 and case['normalExit'] and case['journey']['passed']
assert any('SVGColorizer:' in line for line in case['diagnostics'])
print('Expected negative reproduced: normal exit, 203 functional checks, strict artwork diagnostics fail:',len(case['diagnostics']))
(w/'negative-control.json').write_text(json.dumps(dict(expectedFailure=True,process=case,sourceGuiSha256=report['runtimeSha256']['plugins/gui/libqt_plugin.dll']),indent=2)+'\n')