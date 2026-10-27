from pathlib import Path
from hashlib import sha256
import json,shutil,subprocess
work=Path(__file__).resolve().parent;repo=work.parent.parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
before=read(work/'inputs-candidate-r2.json')['files'];assert all(digest(repo/p)==h for p,h in before.items())
cases=read(work/'native-candidate-r2/processes.json')['records'];assert len(cases)==7
assert all(r['normalExit'] and r['passed'] and all(r['result']['checks'].values()) and r['sourceInputsUnchanged'] and r['runtimeUnchanged'] and r['fixturesUnchanged'] for r in cases)
assert digest(work/'candidate-r2-bin/libvlccore.dll')=='7e3981d1cfc77ba3a93e14e5df0c98da3a99831cef4cab3d77c8666bc8a2a364'
preimage={'src/input/decoder.c':'ce774a56e630a0603aa82387975a184b88c3b2fa3696f8125d3738ffdc2b04bc','src/input/es_out.c':'00fc7a6c326809655462427e0980893afb1502206f6c49f5f5819191bc083ae0'}
assert all(digest(repo/p)==h for p,h in preimage.items())
for name in preimage:
    p=repo/name;d=work/'production-preimage'/name;d.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,d)
    shutil.copyfile(work/'candidate-r2'/p.name,p)
changed={p:digest(repo/p) for p,h in before.items() if digest(repo/p)!=h};assert set(changed)==set(preimage)
(work/'application-r3.json').write_text(json.dumps(dict(productionApplied=True,files=changed,preimage=preimage,privateCandidateCoreSha256=digest(work/'candidate-r2-bin/libvlccore.dll'),ordinaryBuildAccepted=False,fullBetaAccepted=False),indent=2)+'\n',encoding='utf-8',newline='\n')
files=set(before)
files.update(p.relative_to(repo).as_posix() for p in work.iterdir() if p.is_file() and p.suffix in ['.py','.sh','.mk'])
hashes={p:digest(repo/p) for p in sorted(files)};snapshot=work/'source-ordinary-r3';assert not snapshot.exists();snapshot.mkdir()
for p,h in hashes.items():
    d=snapshot/p;d.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/p,d);assert digest(d)==h
(work/'inputs-ordinary-r3.json').write_text(json.dumps(dict(files=hashes,sourceCount=len(hashes),sourceSnapshot=str(snapshot),productionApplied=True,fullBetaAccepted=False),indent=2)+'\n',encoding='utf-8',newline='\n')
subprocess.run(['git','diff','--check'],cwd=repo,check=True)
print(json.dumps(dict(sourceCount=len(hashes),productionApplied=True,files=changed)))
