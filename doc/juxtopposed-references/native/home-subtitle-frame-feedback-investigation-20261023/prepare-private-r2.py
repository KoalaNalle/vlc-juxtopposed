from pathlib import Path
from hashlib import sha256
import json, shutil
work=Path(__file__).resolve().parent;repo=work.parent.parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
source=(work/'run-native-r1.py').read_text(encoding='utf-8')
source=source.replace("choices=['before','ordinary']","choices=['before','ordinary','candidate']")
source=source.replace("'inputs-before-r1.json' if args.phase=='before' else 'inputs-ordinary-r2.json'", "'inputs-candidate-r2.json'")
source=source.replace("root=work/('native-'+args.phase+'-r1')", "root=work/'native-candidate-r2'")
source=source.replace("if args.phase=='ordinary':", "if args.phase=='candidate':")
source=source.replace("work/'ordinary-core-r2/libvlccore.dll'", "work/'candidate-r2-bin/libvlccore.dll'")
source=source.replace("privateCore=False", "privateCore=True")
source=source.replace("(args.phase=='ordinary')", "(args.phase=='candidate')")
target=work/'run-private-r2.py';assert not target.exists();target.write_text(source,encoding='utf-8',newline='\n')
before=read(work/'inputs-before-r1.json')['files'];assert all(digest(repo/p)==h for p,h in before.items())
files=set(before)|{'build-windows/beta-subtitle-frame-feedback-20261023/candidate-r2/decoder.c'}
files.update(p.relative_to(repo).as_posix() for p in work.iterdir() if p.is_file() and p.suffix in ['.py','.sh','.mk'])
hashes={p:digest(repo/p) for p in sorted(files)};root=work/'source-candidate-r2';assert not root.exists();root.mkdir()
for p,h in hashes.items():
    d=root/p;d.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/p,d);assert digest(d)==h
(work/'inputs-candidate-r2.json').write_text(json.dumps(dict(files=hashes,sourceCount=len(hashes),sourceSnapshot=str(root),productionApplied=False,fullBetaAccepted=False),indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(dict(sourceCount=len(hashes),productionApplied=False)))
