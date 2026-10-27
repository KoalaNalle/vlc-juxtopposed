from pathlib import Path
from hashlib import sha256
import json,shutil
work=Path(__file__).resolve().parent;repo=work.parent.parent
parent=repo/'doc/juxtopposed-references/native/home-paused-subtitle-replacement-investigation-20261023'
target=parent.with_name(parent.name+'-r6');assert not target.exists()
digest=lambda p:sha256(p.read_bytes()).hexdigest()
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
checkpoint=read(parent/'checkpoint.json')
assert digest(parent/'checkpoint.json')=='befc73bbfbac6d12f81d8ea5d34b7b07bda6ed0f30d45553025e9d612eff6387'
assert all(digest(parent/p)==h for p,h in checkpoint['artifacts'].items())
review=read(parent/'review.json');comparison=read(parent/'comparison.json')
assert review['candidateRejectedForApplication'] and not review['selectedRepairAccepted']
assert comparison['candidateRejectedForApplication'] and comparison['frameTimeDriftObserved']
review['findings'][2]='Candidate reconstructs captions after a newly selected subtitle on paused seekable nonrecording input. WASAPI reconstruction requests use the pending seek-aware target, but its best-source time is stale and does not qualify video conservation. Both no-audio cases drift forward by40ms at every refresh; reject for application.'
review['parentEvidenceDirectory']=parent.name
review['parentCheckpointSha256']=digest(parent/'checkpoint.json')
target.mkdir();write(target/'review.json',review);write(target/'comparison.json',comparison)
write(target/'erratum.json',dict(parentReview='review.json/findings/2',correction='The first receipt review overstates target conservation across all cases. Its added frameTimeDrift field and comparison already identify the no-audio failure. This canonical review removes the contradictory summary; no candidate acceptance or source application occurred.',parentCheckpointSha256=digest(parent/'checkpoint.json'),originalReceiptPreserved=True))
shutil.copyfile(Path(__file__),target/Path(__file__).name)
(target/'README.md').write_text('''# Paused subtitle replacement investigation: canonical review

The accepted ordinary build loses captions when an embedded subtitle is changed
or an external subtitle is explicitly selected while paused. The new candidate
restores all20 tested caption observations across five cases, but is rejected
for application: both no-audio cases advance the paused video-source timer point
by40ms on every selection refresh. The25fps fixtures make that one frame.
Matching unchanged controls retain the video point. WASAPI's best-source time
remains stale and cannot qualify exact video-frame conservation.

The working-tree player input and decoder sources remain unchanged. The accepted
runtime stays `build-windows/beta-caption-layout-20261023/ui-after-r1/runtime`,
GUId0ff14c6 / Core000f1159 / MKVf46d8d15 / WASAPI4e4e. Candidate Core5837a5b7
is separate. The queued-SPU-only candidate made no difference, and the raw-timer
candidate was corrected before execution. No production repair is accepted.

The [original evidence receipt](../home-paused-subtitle-replacement-investigation-20261023/README.md)
preserves341 hashed artifacts,21 normal native exits and188 snapshot records,
including failed checks and native diagnostics. This canonical review corrects
one overbroad target-conservation sentence in its review; the original remains
immutable. The checked comparison already records the40ms drift.

Next: trace the current displayed-video/SMPTE point and seek normalization, or
refill only subtitles. Repeated selection must preserve the paused picture/time
before maintained native regression, ordinary Core build and desktop validation.
Cold initial captions and automatic external selection remain separate open
flows. Full caption layout, accessibility/DPI, physical outputs, Windows10,
independent review, packaging and the full beta remain open. No install,
commit/push, release/upload or shutdown occurred.

- [Canonical review and limitations](review.json)
- [Paired comparison and time drift](comparison.json)
- [Correction record](erratum.json)
- [Checkpoint and original evidence reference](checkpoint.json)
''',encoding='utf-8',newline='\n')
artifacts={p.relative_to(target).as_posix():digest(p) for p in sorted(target.iterdir()) if p.is_file()}
write(target/'checkpoint.json',dict(artifacts=artifacts,artifactCount=len(artifacts),parentDirectory=parent.name,parentCheckpointSha256=digest(parent/'checkpoint.json'),parentArtifactCount=341,nativeProcessCount=21,nativePictureCount=188,candidateRejectedForApplication=True,productionApplied=False,selectedRepairAccepted=False,fullBetaAccepted=False))
write(work/'publication-investigation-r6.json',dict(passed=True,path=target.relative_to(repo).as_posix(),checkpointSha256=digest(target/'checkpoint.json'),artifactCount=len(artifacts),parentCheckpointSha256=digest(parent/'checkpoint.json')))
print(json.dumps(read(work/'publication-investigation-r6.json')))
