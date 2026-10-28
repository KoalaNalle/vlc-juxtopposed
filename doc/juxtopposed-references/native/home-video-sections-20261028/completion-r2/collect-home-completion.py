from pathlib import Path
from hashlib import file_digest
import json
import shutil
import time

root=Path(__file__).resolve().parent
repo=root.parents[1]
out=repo/'doc/juxtopposed-references/native/home-video-sections-20261028/completion-r2'
out.mkdir(exist_ok=False)
for run in ('native-boundaries-r1','native-boundaries-r2','native-boundaries-r3',
            'native-boundaries-before-r3','native-boundaries-r4','native-full-before-r6',
            'native-full-r5','native-full-r7','home_core12_playing_ass'):
    dest=out/run;dest.mkdir()
    for name in ('verification.json','result.txt','result.xml'):
        if (root/run/name).is_file(): shutil.copyfile(root/run/name,dest/name)
for build in ('native-r10','native-r11','native-r12','gui-r2'):
    dest=out/build;dest.mkdir()
    for name in ('inputs.sha256','binary.sha256','core.sha256','producer.sha256','source.patch','patch.sha256','exit.txt'):
        if (root/build/name).is_file(): shutil.copyfile(root/build/name,dest/name)
for name in ('build-home-r12.sh','build-home-gui-r2.sh','run-native-boundaries-r3.py',
             'run-native-boundaries-before-r3.py','run-native-boundaries-r4.py',
             'run-native-home-full-r5.py','run-native-home-full-r7.py',
             'run-native-full-before-r6.py','run-subtitle-home-core12-r2.py','collect-home-completion.py'):
    shutil.copyfile(root/name,out/name)
for name in ('src/input/input.c','src/input/input_internal.h','src/player/input.c',
             'src/player/player.c','src/player/player.h','modules/gui/qt/tests/test_home_activity.cpp',
             'modules/gui/qt/medialibrary/mlvideohistory.hpp','test/libvlc/native_home_activity.py'):
    dest=out/'source'/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/name,dest)
def digest(path):
    with Path(path).open('rb') as stream: return file_digest(stream,'sha256').hexdigest()
old=json.loads((root/'native-full-r5/verification.json').read_text())
supplement=dict(status='terminal-runner-failed-after-partial-XML',pid=26124,
    capturedAt=time.time(),exitCode=None,
    explanation='The bounded native helper stopped the stalled child at its 180-second limit. XML parsing failed before the launcher saved terminal verification. The child and diagnostic debugger were subsequently confirmed absent. Preserve the partial XML; no pass is claimed.',
    sourceUnchanged=all(digest(repo/n)==h for n,h in old['sourceSha256'].items()),
    runtimeUnchanged=all(digest(root/'native-full-r5/runtime'/n)==h for n,h in old['runtimeSha256'].items() if not n.endswith('plugins.dat')),
    fixtureUnchanged=all(digest(repo/'build-windows/home-native-fixtures-20261014'/n)==h for n,h in old['fixtureSha256'].items()),
    testUnchanged=digest(root/'native-r12/test_qt_home_activity.exe')==old['testSha256'],
    producerUnchanged=digest(root/'run-native-home-full-r5.py')==old['producerSha256'])
(out/'native-full-r5/termination-review.json').write_text(json.dumps(supplement,indent=2)+'\n')
identities={p.relative_to(out).as_posix():digest(p) for p in out.rglob('*') if p.is_file()}
(out/'artifact-sha256.json').write_text(json.dumps(identities,indent=2)+'\n')
print('Copied',len(identities),'evidence files')
