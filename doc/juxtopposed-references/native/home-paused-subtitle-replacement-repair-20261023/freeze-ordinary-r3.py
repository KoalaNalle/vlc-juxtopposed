from pathlib import Path
from hashlib import sha256
import json,shutil,subprocess
work=Path(__file__).resolve().parent;repo=work.parent.parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
prior=read(work/'inputs-tests-r2.json')['files']
changed=[p for p,h in prior.items() if digest(repo/p)!=h]
expected=['src/player/input.c','src/input/decoder.c','src/input/decoder.h','src/input/es_out.c']
assert sorted(changed)==sorted(expected),changed
assert all(digest(repo/p)==h for p,h in read(work/'application-r3.json')['files'].items())
files=set(prior);files.update(p.relative_to(repo).as_posix() for p in work.iterdir() if p.suffix in ['.py','.sh','.mk'])
hashes={p:digest(repo/p) for p in sorted(files)}
root=work/'source-ordinary-r2';assert not root.exists();root.mkdir()
for p,h in hashes.items():
    target=root/p;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/p,target);assert digest(target)==h
record=dict(sourceCount=len(hashes),files=hashes,sourceSnapshot=str(root),productionApplied=True,ordinaryBuildAccepted=False,fullBetaAccepted=False,changedSincePrivateCandidate=changed)
(work/'inputs-ordinary-r2.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8',newline='\n')
subprocess.run(['git','diff','--check'],cwd=repo,check=True)
print(json.dumps(dict(sourceCount=len(hashes),changed=changed)))
