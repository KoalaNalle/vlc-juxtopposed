from pathlib import Path
from hashlib import sha256
import json,shutil,subprocess
work=Path(__file__).resolve().parent;repo=work.parent.parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
def copy(p,d):d.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,d)
target=repo/'doc/juxtopposed-references/native/home-subtitle-frame-feedback-investigation-20261023';assert not target.exists()
inputs=read(work/'inputs-candidate-r2.json');assert inputs['sourceCount']==2355
assert all(digest(repo/p)==h and digest(Path(inputs['sourceSnapshot'])/p)==h for p,h in inputs['files'].items())
assert digest(repo/'src/input/decoder.c')=='ce774a56e630a0603aa82387975a184b88c3b2fa3696f8125d3738ffdc2b04bc'
base=repo/'build-windows/beta-external-subtitle-acceptance-20261023/ui-external.srt-r3/runtime'
baseIds=read(base.parent/'verification.json')['runtimeSha256'];assert all(digest(base/p)==h for p,h in baseIds.items())
assert baseIds['libvlccore.dll']=='ea14300c8c8366a99d17eab124216f543287822c18a85ed305eea0cc557b23d4'
candidateCore=digest(work/'candidate-r2-bin/libvlccore.dll');assert candidateCore=='77670ec8df7ff796269f68e5b8acace167cdd5f6956a813190b5df94e5a13f62'
assert (work/'build-candidate-r2-exit.txt').read_text().strip()=='0'
assert (work/'build-layout-r1-exit.txt').read_text().strip()=='0'
folders={'native-before-r1':1,'native-controls-r1b':6,'native-candidate-r2':1}
records=[];snapshots=[]
for folder,count in folders.items():
    paths=sorted((work/folder).glob('*/process.json'));assert len(paths)==count
    for p in paths:
        d=read(p);r=read(p.parent/'probe/result.json');candidate=folder=='native-candidate-r2'
        assert d['status']=='terminal' and d['normalExit'] and d['exitCode']==0 and not d['forcedCleanupUsed']
        assert d['sourceInputsUnchanged'] and d['runtimeUnchanged'] and d['fixturesUnchanged']
        assert r['status']=='terminal' and r['stopReached'] and r['teardownComplete'] and not r['passed']
        assert all(digest(p.parent/'runtime'/n)==h for n,h in d['runtimeSha256'].items())
        assert digest(repo/'test/libvlc/native_paused_subtitle_tracks.py')==d['producerSha256']
        failed={n for n,v in r['checks'].items() if not v}
        assert failed==({'externalDistinct'} if candidate else {'externalDistinct','noUnrequestedFrameFeedback'}),failed
        assert len(r['frameFeedbackBeforeRequests'])==(0 if candidate else 10)
        assert len(r['frameFeedbackEvents'])==(2 if candidate else 12)
        assert d['runtimeSha256']['libvlccore.dll']==(candidateCore if candidate else baseIds['libvlccore.dll'])
        rows={o['step']:o for o in r['observations']}
        baseMarker=rows['external-explicit-selected-paused']['frameMarker']
        assert rows['requested-next-1']['frameMarker']==baseMarker+1 and rows['requested-next-2']['frameMarker']==baseMarker+2
        assert rows['external-explicit-selected-paused']['brightPixels']>1000 and rows['external-after-resume']['brightPixels']==0
        for o in r['observations']:
            assert o['state']==3 and o['snapshotAvailable'] and all(s['state']==3 for s in o['pausedSamples'])
            assert len({s['timeUs'] for s in o['pausedSamples']})==1
            picture=p.parent/'probe'/o['snapshot'];assert digest(picture)==o['snapshotSha256'];snapshots.append(picture)
        records.append(dict(folder=folder,case=p.parent.name,pid=d['pid'],coreSha256=d['runtimeSha256']['libvlccore.dll'],normalExit=True,passed=False,failedChecks=sorted(failed),checks=r['checks'],unsolicitedEvents=len(r['frameFeedbackBeforeRequests']),totalFrameEvents=len(r['frameFeedbackEvents']),markers={n:o['frameMarker'] for n,o in rows.items()},diagnostics=r['diagnostics']))
assert len(records)==8 and len(snapshots)==136
assert not subprocess.check_output(['git','diff','--cached','--name-only'],cwd=repo,text=True).strip()
subprocess.run(['git','diff','--check'],cwd=repo,check=True)
target.mkdir()
for p in work.iterdir():
    if p.is_file() and p.suffix in ['.py','.sh','.mk','.patch','.json','.txt','.log']:copy(p,target/p.name)
copy(work/'candidate-r2/decoder.c',target/'candidate-r2/decoder.c')
copy(work/'candidate-r2-bin/decoder.d',target/'candidate-r2-bin/decoder.d')
for folder in folders:
    root=work/folder
    if (root/'processes.json').exists():copy(root/'processes.json',target/folder/'processes.json')
    for out in root.iterdir():
        if not out.is_dir():continue
        for p in out.iterdir():
            if p.is_file() and p.suffix in ['.json','.txt','.log']:copy(p,target/p.relative_to(work))
        for p in (out/'probe').iterdir():
            if p.is_file() and p.suffix in ['.json','.txt','.log','.png']:copy(p,target/p.relative_to(work))
for name in ['src/input/decoder.c','test/Makefile.am','test/libvlc/native_paused_subtitle_tracks.py','test/libvlc/native_paused_subtitle_tracks.md','test/libvlc/native_player_callback_layout.c','include/vlc/libvlc_media_player.h']:
    copy(repo/name,target/'sources'/name)
write(target/'comparison.json',dict(records=records,nativeProcessCount=8,snapshotCount=136,ordinaryControls=7,privateCandidateCases=1,privateCandidateFullyPassed=False,productionApplied=False,fullBetaAccepted=False))
write(target/'review.json',dict(productionApplied=False,selectedRepairAccepted=False,ordinaryBuildAccepted=False,fullBetaAccepted=False,
    runtime=base.relative_to(repo).as_posix(),ordinaryCoreSha256=baseIds['libvlccore.dll'],candidateCoreSha256=candidateCore,
    rootCause='A paused decoder flush inserts one frames_countdown picture for internal refresh. The video completion paths report that synthetic request as frame_next_status, which reaches player feedback and the Next frame OSD.',
    proposedChange='Keep the existing refresh request and picture path. Mark only the flush-owned first countdown picture, suppress its completion event, and clear that marker on completion/pause/reset/previous-frame transitions. Explicit user steps retain their existing success/error paths.',
    evidence='Seven unchanged controls each emit ten unsolicited next-frame successes, then two correct requested successes and actual held-marker-plus1/plus2 steps. MP4 has its own held marker; no absolute seek-PTS equality is assumed. The one private decoder candidate emits zero unsolicited events and the same two requested successes/marker advances. Every case still loses the external caption after resume, so no full case passes and no production source is applied.',
    newFailure='The external caption is visible before and during the two steps, but absent in the subsequent resumed/paused snapshot. This reproduces across SubRip/ASS/MP4 with/without WASAPI and a two-track MKS source. The unchanged and private candidate both fail externalDistinct. Causation by stepping versus added paused wall time is not established.',
    setup='The first parent started before source copying finished and exited before staging or launching a child. It was retried only after original copy session79757 completed0 and absence of output was confirmed. The first actual control parent and private parent each stop on the retained externalDistinct failure; all8 native children exit0 normally.',
    limits=['Private one-case feedback evidence only; no ordinary/desktop acceptance for this candidate.','The maintained optional callback mode preserves the post-step caption assertion; thresholds and original failures are not removed.','Frame callback layout232/next176/previous184 comes from the compiled matching C header companion, with executable/source/header/compiler evidence. Callbacks only copy scalar data.','Prior ordinary paused-selection acceptance retains its earlier no-step scope and binaries. The new extended route is not accepted.','Burst/EOF/cancellation/previous-frame policies, exact absolute PTS, complete subtitle/layout/DPI/physical-output/Windows10 and full beta remain open.'],
    next='Compare equal-duration paused hold without frame steps, then trace subtitle channel clock conversions/stop dates and queue lifetime across step/resume. Keep all assertions. Resolve caption loss and qualify the private feedback proposal before any ordinary application, then continue the full13-category/32-surface roadmap.'))
(target/'README.md').write_text('''# Subtitle frame-feedback investigation, 23 October 2026

The misleading Next frame OSD comes from a paused decoder flush borrowing the
user frame-step counter for one internal refresh picture. Its completion emits
a user-facing frame status. The maintained optional callback probe reproduces
ten unsolicited successes in each of seven unchanged controls.

A private decoder candidate distinguishes that flush-owned picture without
changing the rendering request. In one paired case it removes all unsolicited
events while two real next-frame requests still each return success and advance
the actual marker125 to126 to127. This is limited private evidence, not acceptance.

The extended test also exposes a separate failure: the external caption is
visible before/during the steps but disappears after resume. All seven old
controls and the private candidate fail the unchanged externalDistinct check.
SubRip/ASS/MP4, WASAPI/no audio and two-track MKS reproduce it. Whether stepping
or the extra paused wall time causes the loss is not established. No failing
caption assertion or threshold was removed, and no complete case passes.

All8 native children exit normally;136 snapshot hashes verify. The first actual
control parent and private parent stop on the retained behavior failure. A
separate early launcher preflight failure occurred before staging/child launch
while source copying was still active; its recovery is recorded. The matching
C header companion supplies callback layout rather than guessed offsets.
There are2,355 actual frozen candidate-input copies and2,347 prior input copies.

No production Core source is applied. Accepted runtime remains
`build-windows/beta-external-subtitle-acceptance-20261023/ui-external.srt-r3/runtime`,
Coreea14300c / GUId0ff14c6 / MKVf46d8d15 / WASAPI4e4e. Candidate77670ec8 is not
qualified for ordinary/desktop use. Earlier paused external-selection results
retain their earlier no-step scope. No install, commit/push, release/upload or
shutdown occurred. Native expected diagnostics and compiler warnings remain.

Next compare an equal-duration pause without steps and trace subtitle clock/
queue lifetime across resume. Resolve the caption failure, then qualify feedback
and ordinary/desktop behavior before application. The complete13-category/
32-surface beta, original latest-runtime preview-hover check, remaining workflows,
design/accessibility/DPI, platform/review and clean-package gates remain active.

- [Per-case failures, callbacks and markers](comparison.json)
- [Source reasoning and remaining work](review.json)
- [Proposed decoder source](candidate-r2/decoder.c)
- [Artifact hashes](checkpoint.json)
''',encoding='utf-8',newline='\n')
artifacts={p.relative_to(target).as_posix():digest(p) for p in sorted(target.rglob('*')) if p.is_file()}
write(target/'checkpoint.json',dict(artifacts=artifacts,artifactCount=len(artifacts),sourceInputCount=2355,sourceManifestSha256=digest(work/'inputs-candidate-r2.json'),nativeProcessCount=8,snapshotCount=136,productionApplied=False,selectedRepairAccepted=False,fullBetaAccepted=False))
assert all(digest(target/p)==h for p,h in artifacts.items())
write(work/'publication-r3.json',dict(passed=True,path=target.relative_to(repo).as_posix(),checkpointSha256=digest(target/'checkpoint.json'),artifactCount=len(artifacts),nativeProcessCount=8,snapshotCount=136,productionApplied=False,selectedRepairAccepted=False))
print(json.dumps(read(work/'publication-r3.json')))
