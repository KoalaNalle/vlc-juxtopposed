from pathlib import Path
from hashlib import sha256
import json, shutil, subprocess
repo=Path(__file__).resolve().parents[2]
work=Path(__file__).resolve().parent
prior=repo/'build-windows/beta-subtitle-fps-20261023'
digest=lambda p:sha256(p.read_bytes()).hexdigest()
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
write=lambda p,v:p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
old=read(work/'inputs-r5.json')
qt=repo/'modules/gui/qt'
names=set(old['files'])
# The inherited manifest omitted test sources. Capture the entire maintained
# Qt test tree and public/core headers as well as all previously frozen inputs.
for prefix in ('modules/gui/qt','src','include'):
    tracked=subprocess.check_output(['git','ls-files','-z','--',prefix],cwd=repo).split(b'\0')
    names.update(p.decode('utf-8') for p in tracked if p)
names.update(p.relative_to(repo).as_posix() for p in work.iterdir() if p.suffix in ('.py','.sh','.mk'))
forbidden={'test/libvlc/native_soxr_reference.py','test/modules/audio_filter/soxr_lifecycle_trace.c','test/modules/audio_filter/soxr_native_reference.c','test/modules/audio_filter/soxr_pcm_trace.c'}
assert not names.intersection(forbidden)
snapshot=work/'source-r6'
assert not snapshot.exists()
snapshot.mkdir()
files={name:digest(repo/name) for name in sorted(names)}
copies={}
new=0
for name,h in files.items():
    if old['files'].get(name)==h:
        copy=Path(old['sourceCopies'][name])
    else:
        copy=snapshot/name
        copy.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(repo/name,copy)
        new+=1
    assert digest(copy)==h
    copies[name]=str(copy)
for name in ('modules/gui/qt/tests/test_player_subtitle_delay.cpp','modules/gui/qt/tests/vlc_stub_modules.cpp'):
    assert name in files
write(work/'inputs-r6.json',dict(files=files,sourceCopies=copies,sourceCount=len(files),newCopies=new,fullBetaAccepted=False))
print('Frozen r6',len(files),'inputs;',new,'new copies',flush=True)
