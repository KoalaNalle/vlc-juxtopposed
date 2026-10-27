from pathlib import Path
from hashlib import sha256
import json,shutil
work=Path(__file__).resolve().parent;repo=work.parent.parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
old=read(work/'inputs-r1.json')['files'];assert all(digest(repo/p)==h for p,h in old.items())
first=read(work/'native-candidate-r1/frame-marker.mkv-wasapi-automatic-ids/probe/result.json')
assert not first['passed'] and first['automaticExternalSelectionObserved'] and not first['checks']['automaticExternalSelected']
dest=work/'candidate-r2';assert not dest.exists();shutil.copytree(work/'candidate-r1',dest)
p=dest/'es_out.c';s=p.read_text(encoding='utf-8-sig')
needle='        EsOutSelect( p_sys, best, true );\n        return EsIsSelected( best ) ? VLC_SUCCESS : VLC_EGENERIC;'
assert s.count(needle)==1
s=s.replace(needle,'''        /* Use the same exclusive path as a direct user track selection.
         * The main-ES hint can lag behind explicit string-ID selections. */
        int ret = EsOutControlLocked( p_sys, requested, ES_OUT_SET_ES, best );
        return ret == VLC_SUCCESS && EsIsSelected( best )
             ? VLC_SUCCESS : VLC_EGENERIC;''')
p.write_text(s,encoding='utf-8',newline='\n')
files=set(old);files.update(p.relative_to(repo).as_posix() for p in work.iterdir() if p.is_file() and p.suffix in ['.py','.sh','.mk'])
files.update(p.relative_to(repo).as_posix() for p in dest.iterdir() if p.is_file())
hashes={p:digest(repo/p) for p in sorted(files)};snapshot=work/'source-r2';assert not snapshot.exists();snapshot.mkdir()
for p,h in hashes.items():
    d=snapshot/p;d.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/p,d);assert digest(d)==h
(work/'inputs-r2.json').write_text(json.dumps(dict(files=hashes,sourceCount=len(hashes),sourceSnapshot=str(snapshot),productionApplied=False,fullBetaAccepted=False),indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(dict(sourceCount=len(hashes),productionApplied=False)))
