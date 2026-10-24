from pathlib import Path
from hashlib import sha256
import json,shutil
work=Path(__file__).resolve().parent;repo=work.parent.parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
prior=read(work/'inputs-r35.json')['files']
assert all(digest(repo/p)==h for p,h in prior.items())
assert (work/'build-ordinary-mkv-r35-exit.txt').read_text().strip()=='1'
assert "undefined reference to `mkv::matroska_segment_c::BlockGet" in (work/'build-ordinary-mkv-r35.log').read_text(encoding='utf-8',errors='replace')
files=set(prior)
files.update('build-windows/beta-tracks-subtitles-20261023/'+p for p in ['prepare-ordinary-mkv-r35b.py','build-ordinary-mkv-r35b.sh','ordinary-mkv-dependencies-r35b.mk'])
hashes={p:digest(repo/p) for p in sorted(files)}
snapshot=work/'source-r35b';assert not snapshot.exists();snapshot.mkdir()
for p,h in hashes.items():
    target=snapshot/p;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/p,target);assert digest(target)==h
record=dict(sourceCount=len(hashes),files=hashes,sourceSnapshot=str(snapshot),ordinaryBuildAccepted=False,fullBeta=False,
    previousManifestSha256=digest(work/'inputs-r35.json'),
    correction='First ordinary build retained stale objects because canonical assume-new paths did not match configured noncanonical source spellings; .Plo files were dummy. R35b forces all sixteen exact configured source names and records real dependencies with additive -MD/-MF flags. No production source changes after R35.')
(work/'inputs-r35b.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(dict(sourceCount=len(hashes),productionSourceChanged=False)))
