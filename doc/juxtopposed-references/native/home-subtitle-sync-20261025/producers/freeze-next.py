from pathlib import Path
from hashlib import sha256
import json,shutil,sys
repo=Path(__file__).resolve().parents[2]
work=Path(__file__).resolve().parent
prior,revision=sys.argv[1:]
read=lambda p:json.loads(p.read_text(encoding='utf-8'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
old=read(work/f'inputs-{prior}.json')
names={p for p in old['files'] if (repo/p).is_file()}
names.update(p.relative_to(repo).as_posix() for p in work.iterdir() if p.suffix in ('.py','.sh','.mk','.cpp','.h'))
forbidden={'test/libvlc/native_soxr_reference.py','test/modules/audio_filter/soxr_lifecycle_trace.c','test/modules/audio_filter/soxr_native_reference.c','test/modules/audio_filter/soxr_pcm_trace.c'}
assert not names.intersection(forbidden)
snapshot=work/f'source-{revision}'
assert not snapshot.exists()
snapshot.mkdir()
files={p:digest(repo/p) for p in sorted(names)}
copies={}
new=0
for name,h in files.items():
    if old['files'].get(name)==h: target=Path(old['sourceCopies'][name])
    else:
        target=snapshot/name
        target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(repo/name,target)
        new+=1
    assert digest(target)==h
    copies[name]=str(target)
(work/f'inputs-{revision}.json').write_text(json.dumps(dict(files=files,sourceCopies=copies,
    sourceCount=len(files),newCopies=new,fullBetaAccepted=False),indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(dict(inputs=len(files),newCopies=new,revision=revision)),flush=True)
