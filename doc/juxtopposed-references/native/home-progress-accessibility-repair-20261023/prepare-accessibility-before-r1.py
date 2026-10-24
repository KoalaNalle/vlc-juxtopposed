from pathlib import Path
from hashlib import sha256
import json,shutil
work=Path(__file__).resolve().parent;repo=work.parent.parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
old=repo/'build-windows/beta-tracks-subtitles-20261023'
prior=read(old/'inputs-r35c.json')['files']
protected={'test/libvlc/native_soxr_reference.py','test/modules/audio_filter/soxr_lifecycle_trace.c','test/modules/audio_filter/soxr_native_reference.c','test/modules/audio_filter/soxr_pcm_trace.c'}
assert not protected.intersection(prior)
assert all(digest(repo/p)==h for p,h in prior.items())
files=set(prior)
files.update(['PROJECT_HANDOFF.md','doc/juxtopposed-beta-roadmap.md','doc/juxtopposed-settings-consumer-audit.md'])
files.update('build-windows/beta-caption-accessibility-20261023/'+p for p in ['seek_accessibility_journey.cpp','accessibility-journey-r1.mk','build-accessibility-r1.sh',Path(__file__).name])
hashes={p:digest(repo/p) for p in sorted(files)}
root=work/'source-before-r1';assert not root.exists();root.mkdir()
for p,h in hashes.items():
    target=root/p;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(repo/p,target);assert digest(target)==h
record=dict(sourceCount=len(hashes),files=hashes,sourceSnapshot=str(root),phase='before',fullBetaAccepted=False,
    purpose='Actual QAccessibleValueInterface on production SliderBar, real native player seek count/pictures, pointer compatibility and feedback-free playback updates. No source edit before baseline run.')
(work/'inputs-before-r1.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(dict(sourceCount=len(hashes),baselineSliderSha256=hashes['modules/gui/qt/player/qml/SliderBar.qml'])))
