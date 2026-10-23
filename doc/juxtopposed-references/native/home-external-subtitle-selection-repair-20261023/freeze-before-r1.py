from pathlib import Path
from hashlib import sha256
import json,shutil,subprocess
work=Path(__file__).resolve().parent;repo=work.parent.parent
prior=repo/'build-windows/beta-external-subtitle-selection-20261023'
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
old=read(prior/'inputs-r3.json')['files'];last=read(prior/'final-verification-r9.json')
expected=set(last['onlyPostFreezeChanges'])|{'test/libvlc/native_paused_subtitle_tracks.py','test/libvlc/native_paused_subtitle_tracks.md','test/libvlc/generate_paused_subtitle_tracks.sh'}
assert {p for p,h in old.items() if digest(repo/p)!=h}==expected
review=read(repo/'doc/juxtopposed-references/native/home-external-subtitle-selection-investigation-20261023/review.json')
assert all(digest(repo/p)==h for p,h in review['productionSources'].items())
files=set(old);files.update(p.relative_to(repo).as_posix() for p in prior.iterdir() if p.is_file() and p.suffix in ['.py','.sh','.mk'])
files.update(p.relative_to(repo).as_posix() for p in work.iterdir() if p.is_file() and p.suffix in ['.py','.sh','.mk'])
hashes={p:digest(repo/p) for p in sorted(files)};root=work/'source-before-r1';assert not root.exists();root.mkdir()
for p,h in hashes.items():
    d=root/p;d.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/p,d);assert digest(d)==h
(work/'inputs-before-r1.json').write_text(json.dumps(dict(files=hashes,sourceCount=len(hashes),sourceSnapshot=str(root),productionApplied=False,fullBetaAccepted=False),indent=2)+'\n',encoding='utf-8',newline='\n')
subprocess.run(['git','diff','--check'],cwd=repo,check=True)
print(json.dumps(dict(sourceCount=len(hashes),productionApplied=False)))
