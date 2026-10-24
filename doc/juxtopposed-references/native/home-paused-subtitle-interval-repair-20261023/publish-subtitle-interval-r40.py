from pathlib import Path
from hashlib import sha256
import json,re,shutil
from PySide6.QtGui import QImage
work=Path(__file__).resolve().parent;repo=work.parent.parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
protected={'test/libvlc/native_soxr_reference.py','test/modules/audio_filter/soxr_lifecycle_trace.c','test/modules/audio_filter/soxr_native_reference.c','test/modules/audio_filter/soxr_pcm_trace.c'}
freezes=[]
for manifest in ['mkv-interval-r23d-source/source-record.json','mkv-headers-r29-source/source-record.json','inputs-r35c.json']:
    value=read(work/manifest);assert not protected.intersection(value['files'])
    snapshot=Path(value['sourceSnapshot']).resolve();assert snapshot.is_relative_to(work)
    assert all(digest(snapshot/p)==h for p,h in value['files'].items())
    freezes.append(dict(manifest=manifest,sha256=digest(work/manifest),actualFrozenInputs=len(value['files'])))
current=read(work/'inputs-r35c.json');assert current['sourceCount']==2210
assert all(digest(repo/p)==h for p,h in current['files'].items())
desktop=read(work/'desktop-subtitle-observations-r39.json')
assert desktop['selectedPausedMatroskaSeekAccepted'] and not desktop['completeDesktopSubtitleWorkflowAccepted']
ordinary=read(work/'ordinary-ui-r37/verification.json')
assert ordinary['normalExit'] and ordinary['runtimeUnchanged'] and ordinary['sourceInputsUnchanged'] and ordinary['mediaUnchanged']
assert all(digest(work/'ordinary-ui-r37/runtime'/p)==h for p,h in ordinary['runtimeSha256'].items())
assert read(work/'owned-process-audit-r39.json')==[]
assert read(work/'ordinary-mkv-build-audit-r36c.json')['passed']
old_receipts={}
for name in ['home-paused-video-startup-repair-20261023','home-paused-subtitle-seek-investigation-20261023']:
    root=repo/'doc/juxtopposed-references/native'/name
    assert all(digest(root/p)==h for p,h in read(root/'checkpoint.json')['artifacts'].items())
    old_receipts[name]=digest(root/'checkpoint.json')
draft=work/'subtitle-interval-r40-draft';assert not draft.exists();draft.mkdir()
target=repo/'doc/juxtopposed-references/native/home-paused-subtitle-interval-repair-20261023';assert not target.exists()
def copy(relative,name=None):
    source=work/relative;dest=draft/(name if name else relative)
    if dest.exists():assert digest(dest)==digest(source);return
    dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,dest);assert digest(source)==digest(dest)
for p in sorted(work.iterdir()):
    if p.is_file() and re.search(r'(?:^|-)r(?:2[3-9]|3[0-9])[a-z]?(?:[-.]|$)',p.name) and p.suffix in {'.py','.sh','.mk','.log','.txt','.json'}:
        copy(p.name)
copy(Path(__file__).name)
for family in ['mkv-interval-r23-source','mkv-interval-r23b-source','mkv-interval-r23c-source','mkv-interval-r23d-source','mkv-headers-r29-source']:
    copy(family+'/source-record.json')
    for phase in ['before','after']:
        for name in ['matroska_segment.cpp','matroska_segment.hpp','matroska_segment_seeker.cpp','matroska_segment_seeker.hpp']:
            copy(family+'/'+phase+'/'+name)
for folder in ['mkv-interval-before-r23d-bin','mkv-interval-after-r23d-bin','mkv-headers-before-r29-bin','mkv-headers-after-r29-bin','ordinary-mkv-r35c-bin']:
    for p in sorted((work/folder).iterdir()):
        if p.is_file() and p.suffix in {'.d','.Plo','.txt'}:copy(p.relative_to(work).as_posix())
for folder in ['media-r20','media-r21','media-r26','media-r31','media-generator-smoke-r35']:
    for p in sorted((work/folder).iterdir()):
        if p.is_file() and p.suffix in {'.srt','.json','.txt'}:copy(p.relative_to(work).as_posix())
for relative in ['modules/demux/mkv/matroska_segment.cpp','modules/demux/mkv/matroska_segment.hpp','modules/demux/mkv/matroska_segment_seeker.cpp','modules/demux/mkv/matroska_segment_seeker.hpp','test/Makefile.am','test/libvlc/native_paused_subtitle_seek.py','test/libvlc/generate_paused_subtitle_seek.sh','test/libvlc/native_paused_subtitle_seek.md']:
    copy('source-r35c/'+relative,'production-sources/'+relative)
groups=['native-mkv-interval-r25','native-mkv-cost-r28','native-overlap-mask-reference-r24','native-ass-overlap-reference-r27','native-mkv-headers-r30','native-subtitle-cancel-r31','native-subtitle-cancel-r31b','native-seek-policy-r33','native-subtitle-ordinary-r36c','ordinary-seek-policy-r38','ordinary-subtitle-cancel-r38']
matrix=[];pictures=[];processes=[];cost=[]
for group in groups:
    parent=read(work/group/'comparison.json');assert parent['allProcessesNormal']
    copy(group+'/comparison.json')
    for process in parent['records']:
        assert process['normalExit'] and process['exitCode']==0 and process['runtimeUnchanged'] and process['sourceInputsUnchanged'] and process['producerUnchanged']
        assert process.get('fixturesUnchanged',True) and not process.get('forcedCleanupUsed',False)
        relative=group+'/'+process['phase']+'/'+process['case'];result=read(work/relative/'result.json')
        assert result['teardownComplete'] and result['stopReached']
        for name in ['process.json','result.json','native.log','stdout.txt','stderr.txt','scenario.json']:
            if (work/relative/name).exists():copy(relative+'/'+name)
        observations=[]
        for row in result.get('observations',[]):
            observations.append({p:row[p] for p in ['step','rgb','brightPixels','passed','captionMaskJaccard','referenceComparisonPerformed','reportedReadBytesDelta','processCpuSecondsIncludingProbe','processIoDeltaIncludingProbe'] if p in row})
            if row.get('snapshotAvailableWhilePaused') or row.get('snapshotAvailable'):
                path=relative+'/'+row['snapshot'];assert digest(work/path)==row['snapshotSha256']
                im=QImage(str(work/path));assert not im.isNull() and im.width()==960 and im.height()==540
                copy(path);pictures.append(path)
            if row.get('referenceComparisonPerformed'):
                path=row['referencePicture'];assert digest(work/path)==row['referencePictureSha256'];copy(path)
            if group in ['native-mkv-headers-r30','native-subtitle-ordinary-r36c'] and row['step'].startswith('seek-'):
                cost.append(dict(group=group,phase=process['phase'],case=process['case'],step=row['step'],processReadBytes=row['processIoDeltaIncludingProbe']['readBytes'],processCpuSeconds=row['processCpuSecondsIncludingProbe'],settleAndSnapshotSeconds=row['seekToSnapshotSecondsIncludingFixedSettle']))
        matrix.append(dict(group=group,phase=process['phase'],case=process['case'],normalExit=True,behaviorPassed=result['passed'],observations=observations,indexCancellationIsolated=result.get('indexCancellationIsolated')))
        processes.append(dict(group=group,phase=process['phase'],case=process['case'],pid=process['pid'],normalExit=True,behaviorPassed=result['passed']))
assert len(processes)==64
for group in ['native-mkv-headers-r30','native-subtitle-cancel-r31b','native-seek-policy-r33','native-subtitle-ordinary-r36c','ordinary-seek-policy-r38','ordinary-subtitle-cancel-r38']:
    assert read(work/group/'comparison.json')['passed']
for name in ['verification.json','vlc.log','stdout.txt','stderr.txt','config-before.txt','config-after.txt']:
    copy('ordinary-ui-r37/'+name)
write(draft/'matrix.json',matrix);write(draft/'cost-observations.json',cost)
review=dict(selectedPausedMatroskaSeekAccepted=True,completeDesktopSubtitleWorkflowAccepted=False,fullBetaAccepted=False,
    ordinaryIdentities=ordinary['runtimeSha256'],sourceFreezes=freezes,previousImmutableReceipts=old_receipts,
    explanation='Keep a contiguous index of subtitle block spans. A precise seek can replay from the earliest still-active cue before the AV seek bound, including older overlapping cues. Preserve AV time/skip anchors and fast-seek policy. Header-only indexing avoids loading compressed video/audio payloads; normal playback still reads full payloads.',
    validation=['Eight ordinary caption cases pass; their eight unfixed ordinary controls fail. English/Finnish, cold/delayed cues, gaps, overlapping SubRip and ASS, backward/repeated seeks, frozen paused clocks and resume are included.','Configured ordinary rules rebuild all sixteen C++ units with matching class headers. Object hashes, real dependencies and the actual link command are retained.','Private full-payload/header controls pass all sixteen cases. First seek on the 30.66MB gap file drops from 27.40MiB to 2.56MiB of process reads. A long cue starting at zero still needs replay, measured at 29.43MiB including the probe.','Pending-seek Stop is isolated using a non-blocking public LibVLC log callback and normal stop_async. Private two and ordinary one cancellation runs pass. The earlier two file-log attempts stop normally but miss the scan and remain inconclusive.','Six private and three ordinary policy controls preserve disabled-subtitle and fast-seek behavior. The existing missing long cue under fast seek is recorded, not accepted as a complete caption workflow.','Visible ordinary fullscreen shows both overlapping captions, keeps the older cue after short cues end and after backward seeking, holds the exact paused position through subtitle-menu open/Escape, resumes/Pause and quits normally.'],
    limits=['The small 960x540 window covers the lower caption line with the playback bar; fullscreen bottom gradient also darkens that line. This blocks complete desktop subtitle acceptance.','Direct accessibility progress set_value returns without committing a seek in this observation. Physical progress-bar input works. Keep accessibility work open.','Initial subtitles, new selected tracks, external associated subtitles, language/default policy and synchronization edits remain separate.','Process I/O/CPU includes probe/PNG/font reads and filesystem caches. Settle/snapshot timings are not exact seek latency or physical disk benchmarks. No universal large-movie or network performance claim.','Native raw time fields are output-clock observations; colors prove only ten-second segments.'],
    provenanceCorrections=['R23/b/c are uncompiled static refinements; R23d and R29 rebuild every C++ unit in each phase. R23d private object names derive per-unit labels; R29 uses ordinary-style names, and the final ordinary build uses standard object paths.','R25 preparation initially expected one pass assignment rather than two, then its producer metadata referenced the generated parent variable; no native run preceded correction. Preflight producers and the finalizer remain.','R26 build auditor expected a libtool output prefix, but the configured wrapper is doltlibtool. The corrected audit checks the recorded wrapper command and object hashes.','R35 retained stale objects because canonical assume-new names did not match configured source spellings and old .Plo files were dummy. R35b rebuilt all sixteen and linked successfully; a subsequent provenance-copy filename was wrong because the configured later -MF emitted standard .Plo files. R35c collects those real dependencies after make -q succeeds, without recompiling.','R35b preflight expected exit1 for the original make failure; the saved shell code was2. The original producer is retained and corrected before its freeze/build.','The ASS reference parent retains an inherited original overlap-generator provenance label; the actual ASS fixture was generated by generate-subtitle-cost-r26.sh, and its bytes are identified in result.json. That generator and packet probes are included.','The ordinary R38 comparison/worker text retains inherited private-candidate labels. Process runtime identities, source manifest and ordinary build audit establish their actual ordinary DLL scope; raw records are unchanged.','The GUI wrapper intentionally leaves passed:false until observations are reviewed. desktop-subtitle-observations-r39.json records selected success and rejects complete desktop acceptance; the raw wrapper is unchanged.'],
    nextActions=['Repair caption clearance above visible controls in small windows and remove the gradient obscuring readable caption text; use existing video/subtitle geometry policy and revalidate windowed/fullscreen.','Repair progress accessibility seek commitment, then recheck keyboard/range behavior.','Continue remaining subtitle/track workflows and the full13-category/32-surface beta roadmap.'],questionsForMorning=[],
    sideEffects=dict(localProductionSourceEdited=True,ordinaryTestRuntimeCreated=True,installs=False,commit=False,push=False,release=False,upload=False,shutdown=False))
write(draft/'review.json',review)
(draft/'README.md').write_text('''# Paused subtitle interval repair, 23 October 2026

The selected **precise Matroska subtitle-seeking repair is accepted locally**.
The complete desktop subtitle workflow and full beta remain open.

The ordinary build keeps an index of subtitle block spans so a seek can replay
every still-active cue, including older long cues overlapped by shorter ones.
It scans block headers instead of loading video/audio payloads during indexing.
Audio/video anchors, fast-seek policy and normal full-payload playback remain.

Eight ordinary caption scenarios pass and eight unfixed controls fail. Coverage
includes English/Finnish, cold/delayed cues, blank gaps, overlapping SubRip/ASS,
backward/repeated seeks, stable paused clocks, resume and normal teardown. Full
caption masks are compared with normal-playback references where configured.
All sixteen C++ units were rebuilt under the ordinary rules with matching
headers. Core000f1159, GUI716862a1 and WASAPI4e4e remain; Matroska is nowf46d8d15.
The new runtime is `build-windows/beta-tracks-subtitles-20261023/ordinary-ui-r37/runtime`.

On the30.66MB gap fixture, header indexing reduces first-seek process reads from
27.40MiB to2.56MiB. Long active captions still require replay from their start.
These counters include probe/cache activity and are not storage/latency claims.
Normal Stop during a pending index seek passes with private and ordinary DLLs.
Disabled-subtitle and existing fast-seek behavior are preserved.

The visible ordinary player shows both captions in fullscreen, keeps the long
cue after the short one ends and on backward seek, holds pause through menu
open/Escape, resumes/Pause and quits0. **The small window still covers the bottom
caption with its controls**, and the fullscreen gradient darkens that line.
Direct accessibility progress value-setting also did not commit a seek. These
are recorded next tasks, not a full desktop acceptance. Initial/external/newly
selected subtitle workflows and all remaining beta categories stay open.

This receipt retains64 normal native jobs plus one ordinary GUI exit, failed
unfixed controls, two inconclusive initial cancellation attempts, exact producers,
build failures/corrections and frozen input manifests. Actual source snapshots
remain in the ignored build area; the final one contains2,210 inputs. That is
not a compile count. No commit/push, install, release/upload or shutdown occurred.

- [Checkpoint and artifact hashes](checkpoint.json)
- [Review, limits and next actions](review.json)
- [Native observations](matrix.json)
- [Process cost observations](cost-observations.json)
- [Ordinary build audit](ordinary-mkv-build-audit-r36c.json)
- [Ordinary native controls](native-subtitle-ordinary-r36c/comparison.json)
- [Desktop observations](desktop-subtitle-observations-r39.json)
- [Reusable test documentation](production-sources/test/libvlc/native_paused_subtitle_seek.md)
''',encoding='utf-8',newline='\n')
artifacts={p.relative_to(draft).as_posix():digest(p) for p in sorted(draft.rglob('*')) if p.is_file()}
checkpoint=dict(selectedPausedMatroskaSeekAccepted=True,completeDesktopSubtitleWorkflowAccepted=False,fullBetaAccepted=False,
    date='2026-10-23',sourceInputs=2210,ordinaryIdentities=ordinary['runtimeSha256'],nativeProcessesNormal=64,ordinaryGuiProcessesNormal=1,
    qualifiedPictures=len(pictures),qualifiedPicturePaths=pictures,processes=processes,previousImmutableReceipts=old_receipts,artifacts=artifacts)
write(draft/'checkpoint.json',checkpoint)
assert all(digest(draft/p)==h for p,h in artifacts.items())
shutil.copytree(draft,target)
assert digest(target/'checkpoint.json')==digest(draft/'checkpoint.json') and all(digest(target/p)==h for p,h in artifacts.items())
result=dict(publicationVerified=True,target=str(target),checkpointSha256=digest(target/'checkpoint.json'),artifacts=len(artifacts),qualifiedPictures=len(pictures),nativeProcessesNormal=64,ordinaryGuiProcessesNormal=1,selectedPausedMatroskaSeekAccepted=True,completeDesktopSubtitleWorkflowAccepted=False,fullBetaAccepted=False)
write(work/'subtitle-interval-publication-r40.json',result);print(json.dumps(result,indent=2))
