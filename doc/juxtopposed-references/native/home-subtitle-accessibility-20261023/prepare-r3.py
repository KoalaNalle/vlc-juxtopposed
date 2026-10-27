from pathlib import Path
from hashlib import sha256
import json, shutil, subprocess

work = Path(__file__).resolve().parent
repo = work.parent.parent
previous = repo/'build-windows/beta-subtitle-delay-repair-20261023'
read = lambda p: json.loads(p.read_text(encoding='utf-8-sig'))
digest = lambda p: sha256(p.read_bytes()).hexdigest()
final = read(previous/'final-verification-r6.json')
inputs = read(previous/'inputs-ordinary-r1.json')
assert final['passed'] and final['ordinaryDelayRepairAccepted']
assert not (work/'preimages-r3').exists()
for name, h in inputs['files'].items():
    assert digest(Path(inputs['sourceSnapshot'])/name) == h, name
    assert digest(repo/name) == final['onlyPostFreezeChanges'].get(name, h), name
base = previous/'ui-frame-marker-ass.mkv-r4/runtime'
assert all(digest(base/p) == h for p, h in final['runtimeSha256'].items())
assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip() == final['gitHead']
assert not subprocess.check_output(['git','diff','--cached','--name-only'],cwd=repo,text=True).strip()
names = [
    'modules/gui/qt/widgets/qml/SpinBoxExt.qml',
    'modules/gui/qt/player/qml/TracksPageSubtitle.qml',
    'modules/gui/qt/dialogs/dialogs/qml/JuxtopposedSubtitlesSettings.qml',
    'modules/gui/qt/tests/test_juxtopposed_subtitles_settings.py',
]
backup = work/'preimages-r3'
backup.mkdir()
for name in names:
    target = backup/name
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(repo/name, target)
record = dict(preimages={name:digest(repo/name) for name in names},
              baseInputsSha256=digest(previous/'inputs-ordinary-r1.json'),
              previousFinalSha256=digest(previous/'final-verification-r6.json'),
              runtimeSha256=final['runtimeSha256'],gitHead=final['gitHead'],
              scope='Qt bounds compatibility, individual synchronization control names and actual next-media help. No player/decoder changes.',
              fullBetaAccepted=False)
(work/'preparation-r3.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8',newline='\n')
print('Verified previous source/runtime checkpoint and preserved four preimages.',flush=True)
