from pathlib import Path
from hashlib import sha256
import ast,json
work=Path(__file__).resolve().parent;repo=work.parent.parent
old=repo/'build-windows/beta-tracks-subtitles-20261023'
source=repo/'src/input/decoder.c';text=source.read_text(encoding='utf-8')
before='''        if( p_owner->paused && p_owner->frames_countdown == 0
         && !DecoderNeedsPausedPicture(p_owner) )'''
after='''        /* Subtitle cues already queued can change the paused picture without
         * advancing AV playback. Only video requests more input when empty. */
        if( p_owner->paused && p_owner->cat != SPU_ES
         && p_owner->frames_countdown == 0
         && !DecoderNeedsPausedPicture(p_owner) )'''
assert text.count(before)==1
folder=work/'buffer-candidate-r2';assert not folder.exists();folder.mkdir()
(folder/'decoder.c').write_text(text.replace(before,after),encoding='utf-8',newline='\n')
make=(old/'core-trace-r9c.mk').read_text(encoding='utf-8')
for a,b in [('TRACE_CORE','CANDIDATE_CORE'),('TRACE_DECODER','CANDIDATE_DECODER'),('owned-decoder-trace-core-r9c','owned-subtitle-buffer-core-r2')]:make=make.replace(a,b)
(work/'buffer-candidate-r2.mk').write_text(make,encoding='utf-8',newline='\n')
build=(old/'build-core-trace-r9c.sh').read_text(encoding='utf-8')
for a,b in [('beta-tracks-subtitles-20261023','beta-subtitle-workflows-20261023'),('owned-decoder-trace-core-r9c','owned-subtitle-buffer-core-r2'),('build-core-trace-r9c','build-buffer-core-r2'),('core-trace-r9c.mk','buffer-candidate-r2.mk'),('core-trace-r9c-source','buffer-candidate-r2'),('core-trace-r9c-bin','buffer-candidate-r2-bin'),('TRACE_CORE','CANDIDATE_CORE'),('TRACE_DECODER','CANDIDATE_DECODER'),('Private trace Core','Candidate buffered-subtitle Core')]:build=build.replace(a,b)
(work/'build-buffer-core-r2.sh').write_text(build,encoding='utf-8',newline='\n')
worker=(work/'native-tracks-r1.py').read_text(encoding='utf-8').replace("runtimeRole='Ordinary Core000f / MKVf46 / GUId0ff; replay of previous track investigation after interval repair.'", "runtimeRole='Candidate Core with queued SPU consumption while paused; no input advancement or full-workflow acceptance.'")
worker=worker.replace("sourceManifest='build-windows/beta-caption-layout-20261023/inputs-after-r1.json'", "sourceManifest='build-windows/beta-subtitle-workflows-20261023/inputs-r2.json'")
ast.parse(worker);(work/'native-tracks-r2.py').write_text(worker,encoding='utf-8',newline='\n')
parent=(work/'run-r1.py').read_text(encoding='utf-8')
for a,b in [("snapshot=work/'source-r1'","snapshot=work/'source-r2'"),("work/'inputs-r1.json'","work/'inputs-r2.json'"),("work/'native-r1'","work/'native-r2'"),("work/'native-tracks-r1.py'","work/'native-tracks-r2.py'")]:parent=parent.replace(a,b)
parent=parent.replace("files=set(prior);files.update(p.relative_to(repo).as_posix() for p in work.iterdir() if p.suffix=='.py')", "files=set(prior);files.update(p.relative_to(repo).as_posix() for p in work.iterdir() if p.suffix in ['.py','.mk','.sh']);files.add((work/'buffer-candidate-r2/decoder.c').relative_to(repo).as_posix())")
parent=parent.replace('identities={p:digest(runtime/p) for p in protected};env=environment(runtime,out)', "core=runtime/'libvlccore.dll';core.unlink();shutil.copyfile(work/'buffer-candidate-r2-bin/libvlccore.dll',core)\n+        identities={p:digest(runtime/p) for p in protected};env=environment(runtime,out)".replace('\n+','\n'))
parent=parent.replace('privateCore=False','privateCore=True')
parent=parent.replace('digest(runtime/p)==h for p,h in protected.items()', 'digest(runtime/p)==identities[p] for p,h in protected.items()')
ast.parse(parent);(work/'run-r2.py').write_text(parent,encoding='utf-8',newline='\n')
record=dict(productionApplied=False,baselineDecoderSha256=sha256(source.read_bytes()).hexdigest(),candidateDecoderSha256=sha256((folder/'decoder.c').read_bytes()).hexdigest(),scope='Allow buffered SPU decoding while paused, without changing audio/video pause or video-only input demand. Selection-source refill is a separate unresolved path.')
(work/'buffer-candidate-r2.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(record))
