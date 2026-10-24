from pathlib import Path
from hashlib import sha256
import json, shutil, subprocess
work=Path(__file__).resolve().parent;repo=work.parent.parent
prior=repo/'build-windows/beta-external-subtitle-acceptance-20261023'
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
old=read(prior/'inputs-ordinary-r2.json')['files'];last=read(prior/'final-verification-r6.json')
expected=set(last['onlyPostFreezeChanges'])|{'test/Makefile.am','test/libvlc/native_paused_subtitle_tracks.py','test/libvlc/native_paused_subtitle_tracks.md'}
assert {p for p,h in old.items() if digest(repo/p)!=h}==expected
for p,h in read(prior/'application-r2.json')['files'].items():assert digest(repo/p)==h
files=set(old)|{'test/libvlc/native_player_callback_layout.c'}
for root in [prior,work]:files.update(p.relative_to(repo).as_posix() for p in root.iterdir() if p.is_file() and p.suffix in ['.py','.sh','.mk'])
hashes={p:digest(repo/p) for p in sorted(files)};root=work/'source-before-r1';assert not root.exists();root.mkdir()
for p,h in hashes.items():
    d=root/p;d.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/p,d);assert digest(d)==h
(work/'inputs-before-r1.json').write_text(json.dumps(dict(files=hashes,sourceCount=len(hashes),sourceSnapshot=str(root),productionApplied=False,fullBetaAccepted=False),indent=2)+'\n',encoding='utf-8',newline='\n')
subprocess.run(['git','diff','--check'],cwd=repo,check=True)
print(json.dumps(dict(sourceCount=len(hashes),productionApplied=False)))
