from pathlib import Path
from hashlib import sha256
import json,shutil,re
from PySide6.QtGui import QImage
repo=Path('C:/Modding/vlc-juxtopposed');work=Path(__file__).resolve().parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
source=read(work/'mkv-spu-seek-r18b-source/source-record.json');inputs=source['files']
assert source['sourceCount']==2189 and all(digest(repo/p)==h and digest(work/'source-mkv-r18b'/p)==h for p,h in inputs.items())
assert digest(repo/'modules/demux/mkv/matroska_segment.cpp')==source['beforeSha256']
assert digest(work/'mkv-spu-seek-r18b-source/matroska_segment-after.cpp')==source['afterSha256']
ordinary=read(work/'ordinary-ui-r14/verification.json')['runtimeSha256']
assert all(digest(work/'ordinary-ui-r14/runtime'/p)==h for p,h in ordinary.items())
mkv=digest(work/'ordinary-ui-r14/runtime/plugins/demux/libmkv_plugin.dll');assert mkv=='469751a13e8547291bbfc3bcc28e59cf0bebd7e8f6354c5bb00dfeb7c7cd359b'
groups=['native-mkv-seek-r19','native-mkv-edges-r20','native-mkv-overlap-r21','native-overlap-reference-r22']
records=[]
for group in groups:
    parent=read(work/group/'comparison.json');assert parent['allProcessesNormal'] and not parent['repairAccepted']
    for process in parent['records']:
        assert process['normalExit'] and process['exitCode']==0 and process['runtimeUnchanged'] and process['sourceInputsUnchanged'] and process['producerUnchanged']
        if 'fixturesUnchanged' in process:assert process['fixturesUnchanged']
        records.append(dict(group=group,phase=process['phase'],case=process['case'],pid=process['pid']))
assert len(records)==17
for case in ['cold-long-english','cold-long-finnish','cold-delayed-first-cue','short-cue-gaps']:
    assert read(work/'native-mkv-edges-r20/after'/case/'result.json')['passed']
    assert not read(work/'native-mkv-edges-r20/before'/case/'result.json')['passed']
overlap=read(work/'native-mkv-overlap-r21/after/overlapping-cues/result.json');assert not overlap['passed']
observations={r['step']:r for r in overlap['observations']}
assert observations['seek-0-overlap-first']['brightPixels']>5000
assert observations['seek-1-long-alone']['brightPixels']==0 and observations['seek-3-long-alone-again']['brightPixels']==0
assert observations['seek-2-overlap-second']['passed'] and observations['seek-2-overlap-second']['brightPixels']<4000
reference=read(work/'native-overlap-reference-r22/before/normal-playback-reference/result.json');assert reference['passed']
assert reference['observations'][-1]['brightPixels']>2500 and reference['resumeAdvancedFromRequestedTarget']
for name in ['home-paused-video-startup-repair-20261023','home-paused-tracks-investigation-20261023']:
    root=repo/'doc/juxtopposed-references/native'/name
    assert all(digest(root/p)==h for p,h in read(root/'checkpoint.json')['artifacts'].items())
draft=work/'subtitle-investigation-r22-draft';assert not draft.exists();draft.mkdir()
target=repo/'doc/juxtopposed-references/native/home-paused-subtitle-seek-investigation-20261023';assert not target.exists()
def copy(relative,name=None):
    source=work/relative;dest=draft/(name if name else relative);dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,dest);assert digest(source)==digest(dest)
for name in ['probe-subtitle-packets-r17.sh','analyze-subtitle-seek-r17.py','subtitle-seek-analysis-r17/analysis.json','prepare-mkv-spu-seek-r18.py','prepare-mkv-spu-seek-r18b-producer.py','prepare-mkv-spu-seek-r18b.py','mkv-spu-seek-r18b.mk','build-mkv-spu-seek-r18b.sh','build-mkv-spu-seek-r18b.log','build-mkv-spu-seek-r18b-exit.txt','prepare-native-mkv-r19.py','native-mkv-seek-r19.py','run-native-mkv-r19.py','generate-subtitle-seek-edges-r20.sh','prepare-native-mkv-edges-r20.py','native-mkv-edges-r20-tail.py.txt','native-mkv-edges-r20.py','run-native-mkv-edges-r20.py','generate-subtitle-overlap-r21.sh','prepare-native-mkv-overlap-r21.py','run-native-mkv-overlap-r21.py','prepare-native-overlap-reference-r22.py','native-overlap-reference-r22.py','run-native-overlap-reference-r22.py','publish-subtitle-investigation-r22.py']:
    copy(name)
for version in ['r18','r18b']:
    for name in ['source-record.json','matroska_segment-before.cpp','matroska_segment-after.cpp']:copy('mkv-spu-seek-'+version+'-source/'+name)
for phase in ['before','after']:copy('mkv-spu-seek-'+phase+'-r18b-bin/matroska_segment.d')
for path in sorted((work/'subtitle-packet-probes-r17').glob('*.json')):copy(str(path.relative_to(work)))
for folder in ['media-r20','media-r21']:
    for path in sorted((work/folder).iterdir()):
        if path.is_file() and path.suffix in ['.srt','.json']:copy(str(path.relative_to(work)))
pictures=[];matrix=[]
for group in groups:
    copy(group+'/comparison.json')
    for process in read(work/group/'comparison.json')['records']:
        root=group+'/'+process['phase']+'/'+process['case']
        report='investigation.json' if group=='native-mkv-seek-r19' else 'result.json'
        for name in ['process.json',report,'native.log','stdout.txt','stderr.txt']:copy(root+'/'+name)
        if (work/root/'scenario.json').exists():copy(root+'/scenario.json')
        result=read(work/root/report);assert result['teardownComplete'] and result['stopReached']
        matrix.append(dict(group=group,phase=process['phase'],case=process['case'],normalExit=True,behaviorPassed=result.get('passed',False),observations=[dict(step=r['step'],rgb=r.get('rgb'),brightPixels=r.get('brightPixels'),passed=r.get('passed'),maskComparisonPerformed=r.get('maskComparisonPerformed',False),seekToSnapshotSecondsIncludingFixedSettle=r.get('seekToSnapshotSecondsIncludingFixedSettle'),reportedReadBytesDelta=r.get('reportedReadBytesDelta')) for r in result['observations']]))
        for item in result['observations']:
            if item.get('snapshotAvailable') or item.get('snapshotAvailableWhilePaused'):
                relative=root+'/'+item['snapshot'];assert digest(work/relative)==item['snapshotSha256']
                image=QImage(str(work/relative));assert not image.isNull() and image.width()==960 and image.height()==540
                copy(relative);pictures.append(relative)
for p in ['modules/demux/mkv/matroska_segment.cpp','modules/demux/mkv/matroska_segment_seeker.cpp','modules/demux/mkv/matroska_segment_seeker.hpp','modules/demux/mkv/matroska_segment.hpp']:
    copy('source-mkv-r18b/'+p,'production-sources/'+p)
review=dict(passed=False,investigationComplete=True,repairAccepted=False,fullBetaAccepted=False,privateCandidateKeptSeparate=True,ordinaryCoreSha256=ordinary['libvlccore.dll'],ordinaryGuiSha256=ordinary['plugins/gui/libqt_plugin.dll'],ordinaryMkvSha256=mkv,privateMkvSha256=read(work/'native-mkv-seek-r19/comparison.json')['privateMkvHashes'],evidence=['Eight packet/seek-position observations match whether an active cue begins before the video-led file-position bound. The ordinary follow-up matches the earlier private Core trace.','The private selected-SPU replay candidate fixes original/remux backward seeks in three controls and passes cold English/Finnish, delayed first-cue and empty-gap cases on colored video.','The overlap case still loses a long cue at15s and35s after a shorter cue has been indexed. Normal playback from the beginning preserves that long cue after15s on the preimage plugin.','The first-cluster QUESTIONABLE seekpoint is seeded for every track by Preload. This existing fallback explains why the delayed first-cue case works even before its first real subtitle packet has been encountered.'],manualPictureInspection=['Finnish middle cue reads SUOMI KESKIMMÄINEN on the expected blue segment.','The25s short-cue gap is blank on blue.','At11s the candidate shows SHORT OVERLAY CUE plus LONG OVERLAPPING CUE on green.','At31s it shows only SECOND SHORT OVERLAY on red: the longer cue is missing, despite a presence-only raw row saying passed.','Normal playback after15s shows LONG OVERLAPPING CUE on green.'],testScopeCorrections=['Presence-only success at31s does not qualify the complete overlapping-caption requirement. That row remains unchanged and is explicitly rejected as whole-caption acceptance.','White masks are recorded, but scenario configs do not supply independent expected masks. maskComparisonPerformed is false; no exact mask-comparison pass is claimed.','Center colors prove the coarse10s target segment; paused output-clock fields are not exact video PTS.','Seek timings include a fixed0.9s settle and snapshot overhead. Input counters can be reported asynchronously. Neither qualifies a latency or scanning-cost budget.','R18 was not compiled. Static inspection moved replay after the existing no-seekpoint return before building R18b, preserving that failure path.'],nextActions=['Replace the latest-point assumption with a model that retains every cue still active at the target, including overlapping spans. Inspect finalized BlockDuration and seekpoint insertion/coverage before choosing the implementation.','Preserve AV time anchors, per-track skip positions, preroll, fast-seek semantics and cancellation; measure first/repeated seeks on larger inputs.','If index/class layouts change, rebuild all affected Matroska C++ translation units together for both controls; do not mix old-layout objects with new headers.','Require the full overlap case, cold/played/gap/language controls and ordinary GUI validation before promoting any Matroska change.','Paused track replacement, external associated subtitle delivery, default/language policy, initial/desktop subtitle display and the full13-category/32-surface roadmap remain open.'],questionsForMorning=[],sideEffects=dict(productionMkvEdited=False,ordinaryRuntimeReplaced=False,installs=False,commit=False,push=False,release=False,upload=False,shutdown=False))
write(draft/'matrix.json',matrix);write(draft/'review.json',review)
(draft/'README.md').write_text('''# Paused subtitle seek investigation, 23 October 2026

This investigation is complete; its candidate is **not accepted or promoted**.
The ordinary Core000f1159/GUI716862a1/MKV469751a1 tuple remains unchanged.

Active long cues can precede the video-led seek position and are then not read.
Packet locations, decoder trace and ordinary follow-up agree in eight controls.
A private Matroska candidate re-reads the selected subtitle's last known block
while retaining the audio/video time and skip anchors. Original/remux backward
seeks improve, and cold English/Finnish, delayed-first-cue and blank-gap tests
pass with the expected coarse video colors and frozen paused clock.

The overlap test prevents promotion. A long0–59.9s cue overlaps shorter10–12s
and30–32s cues. The candidate loses the long cue at15s and35s after using a newer
short cue as its replay point. Normal playback from the beginning preserves
the long cue after15s. At31s only the short cue appears: a raw presence-only
row says passed, but the review rejects that as complete-caption acceptance.

All17 native jobs close normally, with120 qualified pictures and retained
errors/warnings. This includes six initial comparisons, eight colored controls,
two overlap controls and one normal-playback reference. No exact mask, video
PTS, interaction latency, scanning-cost or whole-workflow acceptance is claimed.
The2,189-input freeze contains actual source copies, headers and producers;
it is not a translation-unit compile count. Only the Matroska segment unit is
replaced in each private plugin; other objects and build flags are shared.

Next, cue selection must account for all still-active spans, with appropriate
index coverage and measured scanning cost. Any layout change requires rebuilding
all affected C++ objects together. Track replacement, associated subtitles,
desktop/initial display and the full beta roadmap remain open. No new question
requires the sleeping user's input. No production Matroska edit, install,
commit/push, release/upload or shutdown occurred.

- [Checkpoint and artifact hashes](checkpoint.json)
- [Review, rejected conclusions and next actions](review.json)
- [Collected observations](matrix.json)
- [Packet/seek position analysis](subtitle-seek-analysis-r17/analysis.json)
- [Matched private plugin controls](native-mkv-seek-r19/comparison.json)
- [Cold/language/gap controls](native-mkv-edges-r20/comparison.json)
- [Overlap failures](native-mkv-overlap-r21/comparison.json)
- [Normal-playback reference](native-overlap-reference-r22/comparison.json)
''',encoding='utf-8',newline='\n')
artifacts={p.relative_to(draft).as_posix():digest(p) for p in sorted(draft.rglob('*')) if p.is_file()}
checkpoint=dict(passed=False,investigationComplete=True,repairAccepted=False,fullBetaAccepted=False,date='2026-10-23',sourceInputs=2189,ordinaryIdentities=ordinary,ordinaryMkvSha256=mkv,nativeProcessesNormal=17,qualifiedPictures=len(pictures),qualifiedPicturePaths=pictures,artifacts=artifacts)
assert len(pictures)==120
write(draft/'checkpoint.json',checkpoint);assert all(digest(draft/p)==h for p,h in artifacts.items())
shutil.copytree(draft,target);assert digest(draft/'checkpoint.json')==digest(target/'checkpoint.json') and all(digest(target/p)==h for p,h in artifacts.items())
result=dict(publicationVerified=True,investigationComplete=True,repairAccepted=False,fullBetaAccepted=False,target=str(target),checkpointSha256=digest(target/'checkpoint.json'),artifacts=len(artifacts),nativeProcessesNormal=17,qualifiedPictures=len(pictures),sourcesCurrentAndFrozen=2189)
write(work/'subtitle-publication-r22.json',result);print(json.dumps(result,indent=2))
