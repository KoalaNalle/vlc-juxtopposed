from pathlib import Path
from hashlib import sha256
import json,shutil,subprocess
work=Path(__file__).resolve().parent;repo=work.parent.parent
prior=repo/'build-windows/beta-external-subtitle-selection-20261023'
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
inputs=read(work/'inputs-before-r1.json')['files'];assert all(digest(repo/p)==h for p,h in inputs.items())
controls=read(work/'native-before-r1/processes.json');assert len(controls['records'])==17 and controls['allProcessesNormal'] and controls['expectedBehaviorObserved']
assert sum(r['passed'] for r in controls['records'])==2
assert all(r['normalExit'] and r['sourceInputsUnchanged'] and r['runtimeUnchanged'] and r['fixturesUnchanged'] for r in controls['records'])
published=repo/'doc/juxtopposed-references/native/home-external-subtitle-selection-investigation-20261023'
checkpoint=read(published/'checkpoint.json');assert all(digest(published/p)==h for p,h in checkpoint['artifacts'].items())
review=read(published/'review.json');assert not review['productionApplied']
assert all(digest(repo/p)==h for p,h in review['productionSources'].items())
assert all(digest(prior/'candidate-r3'/Path(p).name)==h for p,h in review['candidateSources'].items())
backup=work/'production-preimage';assert not backup.exists();backup.mkdir()
for p,h in review['productionSources'].items():
    d=backup/p;d.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/p,d);assert digest(d)==h
for p,h in review['candidateSources'].items():
    shutil.copyfile(prior/'candidate-r3'/Path(p).name,repo/p);assert digest(repo/p)==h
write(work/'application-r2.json',dict(productionApplied=True,files=review['candidateSources'],preimage=review['productionSources'],candidateCoreSha256=review['candidateCoreSha256'],ordinaryBuildAccepted=False,fullBetaAccepted=False))
files=set(inputs);files.update(p.relative_to(repo).as_posix() for p in work.iterdir() if p.is_file() and p.suffix in ['.py','.sh','.mk'])
hashes={p:digest(repo/p) for p in sorted(files)};root=work/'source-ordinary-r2';assert not root.exists();root.mkdir()
for p,h in hashes.items():
    d=root/p;d.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/p,d);assert digest(d)==h
write(work/'inputs-ordinary-r2.json',dict(files=hashes,sourceCount=len(hashes),sourceSnapshot=str(root),productionApplied=True,ordinaryBuildAccepted=False,fullBetaAccepted=False))
subprocess.run(['git','diff','--check'],cwd=repo,check=True)
print(json.dumps(dict(productionApplied=True,sourceCount=len(hashes),files=list(review['candidateSources']),ordinaryBuildAccepted=False)))
