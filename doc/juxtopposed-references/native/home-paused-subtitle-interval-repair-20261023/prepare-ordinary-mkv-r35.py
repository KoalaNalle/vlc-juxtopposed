from pathlib import Path
from hashlib import sha256
import ast,json,shutil
work=Path(__file__).resolve().parent;repo=work.parent.parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
prior=read(work/'mkv-headers-r29-source/source-record.json')['files']
changed=[p for p,h in prior.items() if digest(repo/p)!=h]
expected={'modules/demux/mkv/'+p for p in ['matroska_segment.cpp','matroska_segment.hpp','matroska_segment_seeker.cpp','matroska_segment_seeker.hpp']}
expected.add('test/Makefile.am')
assert set(changed)==expected,changed
applied=read(work/'source-application-r34.json')
assert all(digest(repo/'modules/demux/mkv'/p)==h for p,h in applied['productionFiles'].items())
ast.parse((repo/'test/libvlc/native_paused_subtitle_seek.py').read_text(encoding='utf-8'))
files=set(prior)
files.update('test/libvlc/'+p for p in ['native_paused_subtitle_seek.py','generate_paused_subtitle_seek.sh','native_paused_subtitle_seek.md'])
files.update('build-windows/beta-tracks-subtitles-20261023/'+p for p in ['promote-subtitle-interval-r34.py','prepare-ordinary-mkv-r35.py','build-ordinary-mkv-r35.sh'])
assert not any('soxr_' in p and p.endswith(('.py','.c')) for p in files if p in {
    'test/libvlc/native_soxr_reference.py','test/modules/audio_filter/soxr_lifecycle_trace.c',
    'test/modules/audio_filter/soxr_native_reference.c','test/modules/audio_filter/soxr_pcm_trace.c'})
hashes={p:digest(repo/p) for p in sorted(files)}
snapshot=work/'source-r35';assert not snapshot.exists();snapshot.mkdir()
for p,h in hashes.items():
    target=snapshot/p;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/p,target);assert digest(target)==h
record=dict(sourceCount=len(hashes),files=hashes,sourceSnapshot=str(snapshot),changedPriorFiles=changed,
    localSourceApplied=True,ordinaryBuildAccepted=False,fullBeta=False,
    previousSourceRecordSha256=digest(work/'mkv-headers-r29-source/source-record.json'))
(work/'inputs-r35.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(dict(sourceCount=len(hashes),changedPriorFiles=changed)))
