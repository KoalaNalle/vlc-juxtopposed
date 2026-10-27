from pathlib import Path
from hashlib import sha256
import json,shutil

base=Path('/root/vlc-juxtopposed-build/contrib/contrib-win64')
work=Path('/mnt/c/Modding/vlc-juxtopposed/build-windows/beta-subtitle-accessibility-20261023')
out=work/'qt-source-r4'
assert not out.exists()
out.mkdir()
names=[
 'qt/src/plugins/platforms/windows/uiautomation/qwindowsuiarangevalueprovider.cpp',
 'qt/src/plugins/platforms/windows/uiautomation/qwindowsuiavalueprovider.cpp',
 'qtdeclarative/src/quick/accessible/qaccessiblequickitem.cpp',
 'qtdeclarative/src/quicktemplates/qquickspinbox.cpp',
]
files={}
for name in names:
    source=base/name
    target=out/name
    target.parent.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(source,target)
    files[name]=sha256(source.read_bytes()).hexdigest()
(work/'qt-source-r4.json').write_text(json.dumps(dict(files=files,
    scope='Read-only copies of the pinned Qt sources explaining the value range route; not a private VLC hook or a Windows UIA run.'),indent=2)+'\n',encoding='utf-8')
print('Preserved four pinned Qt source files.',flush=True)
