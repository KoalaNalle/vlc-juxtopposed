from pathlib import Path
from hashlib import sha256
import json,shutil
work=Path(__file__).resolve().parent;repo=work.parent.parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
prior=read(work/'inputs-r35b.json')['files'];assert all(digest(repo/p)==h for p,h in prior.items())
log=(work/'build-ordinary-mkv-r35b.log').read_text(encoding='utf-8',errors='replace')
assert sum('--mode=compile' in l for l in log.splitlines())==16
assert 'undefined reference' not in log and 'error:' not in log
assert 'cp: cannot stat' in log and "make: Leaving directory" in log
files=set(prior)
files.update('build-windows/beta-tracks-subtitles-20261023/'+p for p in ['prepare-ordinary-build-r35c.py','finalize-ordinary-build-r35c.sh'])
hashes={p:digest(repo/p) for p in sorted(files)}
snapshot=work/'source-r35c';assert not snapshot.exists();snapshot.mkdir()
for p,h in hashes.items():
    dest=snapshot/p;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/p,dest);assert digest(dest)==h
record=dict(sourceCount=len(hashes),files=hashes,sourceSnapshot=str(snapshot),ordinaryBuildAccepted=False,fullBeta=False,
    previousManifestSha256=digest(work/'inputs-r35b.json'),
    correction='R35b standard compile/link succeeded; packaging requested a dependency filename overridden by the configured later -MF option. R35c does not recompile: it requires make -q success and collects the real standard .Plo files and object hashes. Production sources unchanged.')
(work/'inputs-r35c.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(dict(sourceCount=len(hashes),productionSourceChanged=False)))
