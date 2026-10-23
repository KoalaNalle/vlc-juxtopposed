from pathlib import Path
from hashlib import sha256
import json, shutil

repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
def new_text(name,text):
    target=work/name;assert not target.exists();target.write_text(text,encoding='utf-8',newline='\n')

ordinary=(work/'run-ordinary-r4.py').read_text(encoding='utf-8')
ordinary=ordinary.replace('inputs-r4.json','inputs-r14.json').replace('ordinary-ui-r4','ordinary-ui-r14')
before="qt=runtime/'plugins/gui/libqt_plugin.dll';qt.unlink();shutil.copyfile(work/'ordinary-r4/libqt_plugin.dll',qt)"
after=before+"\ncore=runtime/'libvlccore.dll';core.unlink();shutil.copyfile(work/'core-r14/libvlccore.dll',core)"
assert ordinary.count(before)==1;ordinary=ordinary.replace(before,after)
ordinary=ordinary.replace('No hotkey/controller/model/Core changes', 'No hotkey/controller/model changes')
new_text('run-ordinary-r14.py',ordinary)

native=(work/'run-native-startup-edges-r12.py').read_text(encoding='utf-8')
native=native.replace("inputs-r4.json", "inputs-r14.json").replace('len(inputs)==2139','len(inputs)>=2139')
start=native.index("source_record = read(");end=native.index("candidate_hash =", start)
native=native[:start]+"candidate = work/'core-r14/libvlccore.dll'\n"+native[end:]
native=native.replace("worker = work/'native-startup-edges-r12.py'", "worker = repo/'test/libvlc/native_paused_video_startup.py'")
native=native.replace("root = work/'native-startup-edges-r12'", "root = work/'native-startup-ordinary-r14'")
native=native.replace("['original','candidate']", "['ordinary']").replace("variant=='candidate'", "variant=='ordinary'")
native=native.replace("candidateSourceRecordSha256=digest(work/'startup-candidate-r11b-source/source-record.json')", "sourceManifestSha256=digest(work/'inputs-r14.json'),privateCore=False")
native=native.replace('candidateOnly=True','candidateOnly=False')
new_text('run-native-startup-ordinary-r14.py',native)

prior=read(work/'inputs-r4.json');files=set(prior)
for p in ['test/libvlc/native_paused_video_startup.py','test/libvlc/generate_paused_video_startup.sh','test/libvlc/native_paused_video_startup.md']:
    files.add(p)
for name in ['prepare-r14.py','build-core-r14.sh','run-ordinary-r14.py','run-native-startup-ordinary-r14.py','prepare-startup-fixture-generator-r14.py']:
    files.add('build-windows/beta-tracks-subtitles-20261023/'+name)
protected={'test/libvlc/native_soxr_reference.py','test/modules/audio_filter/soxr_lifecycle_trace.c','test/modules/audio_filter/soxr_native_reference.c','test/modules/audio_filter/soxr_pcm_trace.c'}
assert not files&protected
inputs={p:digest(repo/p) for p in sorted(files)}
changed=[p for p,h in prior.items() if inputs[p]!=h]
assert changed==['src/input/decoder.c','test/Makefile.am'],changed
assert digest(work/'source-r4/src/input/decoder.c')==prior['src/input/decoder.c']
assert not (work/'inputs-r14.json').exists();write(work/'inputs-r14.json',inputs)
snapshot=work/'source-r14';assert not snapshot.exists();snapshot.mkdir()
for p,h in inputs.items():
    target=snapshot/p;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/p,target);assert digest(target)==h
write(snapshot/'snapshot.json',dict(fileCount=len(inputs),files=inputs))
write(work/'scope-r14.json',dict(changedPriorInputs=changed,addedInputs=sorted(files-set(prior)),ordinaryGuiReusedSha256=digest(work/'ordinary-r4/libqt_plugin.dll'),productionScope='A paused video decoder awaiting its first picture can consume/request input independently of explicit frame stepping. Subtitle demux/selection and audio clocks stay unchanged.',privateCandidate='startup-candidate-r11b-source',sourceSnapshot=str(snapshot)))
print('Frozen',len(inputs),'inputs',changed)
