from pathlib import Path
from hashlib import sha256
import json, shutil

work=Path(__file__).resolve().parent
repo=work.parent.parent
previous=repo/'build-windows/beta-subtitle-delay-repair-20261023'
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
old=read(previous/'inputs-ordinary-r1.json')
final=read(previous/'final-verification-r6.json')
preparation=read(work/'preparation-r3.json')
names=set(old['files'])
names.update(p.relative_to(repo).as_posix() for p in work.iterdir() if p.suffix in ['.py','.sh','.mk'])
forbidden={'test/libvlc/native_soxr_reference.py','test/modules/audio_filter/soxr_lifecycle_trace.c',
           'test/modules/audio_filter/soxr_native_reference.c','test/modules/audio_filter/soxr_pcm_trace.c'}
assert not names.intersection(forbidden)
delta=work/'source-delta-r3'
assert not delta.exists()
delta.mkdir()
files={name:digest(repo/name) for name in sorted(names)}
copies={}
changed={}
for name,h in files.items():
    if name in old['files'] and h==old['files'][name]:
        copy=Path(old['sourceSnapshot'])/name
    else:
        assert name in preparation['preimages'] or name in final['onlyPostFreezeChanges'] or name not in old['files'],name
        copy=delta/name
        copy.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(repo/name,copy)
        changed[name]=h
    assert digest(copy)==h,name
    copies[name]=str(copy)
record=dict(files=files,sourceCopies=copies,sourceCount=len(files),changedCopies=changed,
            reusedImmutableCopies=len(files)-len(changed),
            baseManifest=str(previous/'inputs-ordinary-r1.json'),
            baseManifestSha256=digest(previous/'inputs-ordinary-r1.json'),
            fullBetaAccepted=False)
(work/'inputs-r3.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8',newline='\n')
print('Frozen',len(files),'inputs with',len(changed),'new copies; unchanged snapshot files reused.',flush=True)
