from pathlib import Path
from hashlib import sha256
import json,shutil,subprocess
work=Path(__file__).resolve().parent;repo=work.parent.parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
result=read(work/'native-after-r1b/process.json');assert result['normalExit'] and result['passed'] and result['runtimeUnchanged'] and result['sourceInputsUnchanged']
assert len(result['journey']['checks'])==39 and not result['diagnostics']
make=repo/'modules/gui/qt/Makefile.am';raw=make.read_bytes()
assert raw.endswith(b'EXTRA_DIST += tests/native_ui/seek_accessibility_journey.cpp tests/native_ui/seek_accessibility_journey.md\r\n')
make.write_bytes(raw[:-2]+b'\n')
subprocess.run(['git','diff','--check','--','modules/gui/qt/Makefile.am','modules/gui/qt/player/qml/SliderBar.qml'],cwd=repo,check=True)
prior=read(work/'inputs-after-r2b.json')['files']
changed=[p for p,h in prior.items() if digest(repo/p)!=h]
assert changed==['modules/gui/qt/Makefile.am']
assert digest(repo/'modules/gui/qt/tests/native_ui/seek_accessibility_journey.cpp')==digest(work/'seek_accessibility_journey.cpp')
files=set(prior)
files.update(['modules/gui/qt/tests/native_ui/seek_accessibility_journey.cpp','modules/gui/qt/tests/native_ui/seek_accessibility_journey.md'])
files.update('build-windows/beta-caption-accessibility-20261023/'+p for p in ['ordinary-accessibility-r3.mk','build-ordinary-accessibility-r3.sh',Path(__file__).name])
hashes={p:digest(repo/p) for p in sorted(files)}
root=work/'source-ordinary-r3';assert not root.exists();root.mkdir()
for p,h in hashes.items():
    target=root/p;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/p,target);assert digest(target)==h
record=dict(sourceCount=len(hashes),files=hashes,sourceSnapshot=str(root),fullBetaAccepted=False,ordinaryRuntimeAccepted=False,
    priorCandidateChanges=changed,selectedRepair='Guard accessible progress value writes and preserve normal position updates without feedback seeks.',
    evidence=dict(baselineResultSha256=digest(work/'native-before-r1b/process.json'),candidateResultSha256=digest(work/'native-after-r1b/process.json')))
(work/'inputs-ordinary-r3.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(dict(sourceCount=len(hashes),privateNativeChecks=39,productionChangedSincePrivate=changed)))
