from pathlib import Path
from hashlib import sha256
import json,shutil
work=Path(__file__).resolve().parent;repo=work.parent.parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
before=read(work/'inputs-before-r1b.json')['files']
assert all(digest(repo/p)==h for p,h in before.items())
baseline=read(work/'native-before-r1b/process.json')
assert baseline['normalExit'] and not baseline['passed'] and baseline['sourceInputsUnchanged'] and baseline['runtimeUnchanged']
assert baseline['journey']['error']=='accessibility or pointer action reaches native seek'
assert all(r['passed'] for r in baseline['journey']['checks'][:-1])
candidate=read(work/'slider-candidate-r2.json')
source=repo/candidate['sourceRelativePath'];prepared=work/'candidate-r2/SliderBar.qml'
assert digest(source)==candidate['beforeSha256'] and digest(prepared)==candidate['afterSha256']
shutil.copyfile(prepared,source);assert digest(source)==candidate['afterSha256']
changed=[p for p,h in before.items() if digest(repo/p)!=h]
assert changed==[candidate['sourceRelativePath']]
files=set(before)
files.update('build-windows/beta-caption-accessibility-20261023/'+p for p in ['prepare-slider-candidate-r2.py','slider-candidate-r2.json','candidate-r2/SliderBar.qml',Path(__file__).name])
hashes={p:digest(repo/p) for p in sorted(files)}
root=work/'source-after-r2b';assert not root.exists();root.mkdir()
for p,h in hashes.items():
    target=root/p;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/p,target);assert digest(target)==h
record=dict(sourceCount=len(hashes),files=hashes,sourceSnapshot=str(root),phase='after',fullBetaAccepted=False,
    productionSourceChanged=changed,baselineResultSha256=digest(work/'native-before-r1b/process.json'),
    productionApplied=True,ordinaryRuntimeAccepted=False,
    scope='Single production SliderBar QML change after verified old-build accessibility failure. Preserve existing native seek-speed, pointer/chapter FSM and callbacks; validate private then ordinary GUI builds.')
(work/'inputs-after-r2b.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(dict(sourceCount=len(hashes),productionSourceChanged=changed)))
