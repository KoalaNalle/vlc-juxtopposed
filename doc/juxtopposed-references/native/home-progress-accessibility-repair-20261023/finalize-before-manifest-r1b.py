from pathlib import Path
from hashlib import sha256
import json,shutil
work=Path(__file__).resolve().parent;repo=work.parent.parent
digest=lambda p:sha256(p.read_bytes()).hexdigest()
manifest=work/'inputs-before-r1b.json'
record=json.loads(manifest.read_text(encoding='utf-8-sig'))
assert all(digest(repo/p)==h for p,h in record['files'].items())
assert not (work/'build-before-r1b.log').exists() and not (work/'native-before-r1b').exists()
preserved=work/'inputs-before-r1b-preflight.json';assert not preserved.exists();shutil.copyfile(manifest,preserved)
assert digest(work/'accessibility-journey-r1b.mk')==digest(work/'accessibility-journey-r1.mk')
for name in ['accessibility-journey-r1b.mk',Path(__file__).name]:
    relative='build-windows/beta-caption-accessibility-20261023/'+name
    source=work/name;target=Path(record['sourceSnapshot'])/relative
    target.parent.mkdir(parents=True,exist_ok=True);assert not target.exists();shutil.copyfile(source,target)
    record['files'][relative]=digest(source);assert digest(target)==digest(source)
record.update(sourceCount=len(record['files']),preflightManifestSha256=digest(preserved),
    preflightCompletion='Added the byte-identical phase-named makefile required by the generated build script before any R1b build/native run; original preflight manifest retained.')
manifest.write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(dict(sourceCount=record['sourceCount'],productionSourceChanged=False)))
