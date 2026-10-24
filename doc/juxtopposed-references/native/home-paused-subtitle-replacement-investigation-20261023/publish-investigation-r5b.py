from pathlib import Path
from hashlib import sha256
import json,re,shutil,subprocess
work=Path(__file__).resolve().parent;repo=work.parent.parent
target=repo/'doc/juxtopposed-references/native/home-paused-subtitle-replacement-investigation-20261023'
assert not target.exists()
digest=lambda p:sha256(p.read_bytes()).hexdigest()
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
assert digest(repo/'src/player/input.c')=='070effdd2dffbadc64c28668f0e49af99541ea8b4596f1bced769092c851a129'
assert digest(repo/'src/input/decoder.c')=='a1cc614c4df88bf1c72c9874fa1fd3f82c4208e71d92b16f969cb5583682fa01'
for rev in ['r1','r2','r3','r4']:
    manifest=read(work/('inputs-'+rev+'.json'))
    assert all(digest(Path(manifest['sourceSnapshot'])/p)==h for p,h in manifest['files'].items())
latest=read(work/'inputs-r4.json');assert all(digest(repo/p)==h for p,h in latest['files'].items())
groups={'native-r1':3,'native-r2':3,'native-before-r3':5,'native-before-r4':5,'native-candidate-r4':5}
records=[];pictures=[]
for group,count in groups.items():
    jobs=list((work/group).glob('*/process.json'));assert len(jobs)==count
    for path in jobs:
        p=read(path);d=read(path.parent/'investigation.json')
        assert p['status']=='terminal' and p['normalExit'] and p['exitCode']==0
        assert p['runtimeUnchanged'] and p['sourceInputsUnchanged'] and p['producerUnchanged'] and not p['forcedCleanupUsed']
        assert d['status']=='terminal' and d['teardownComplete'] and d['stopReached']
        assert all(digest(path.parent/'runtime'/name)==h for name,h in p['runtimeSha256'].items())
        for row in d['observations']:
            if row.get('snapshotAvailable'):
                image=path.parent/row['snapshot'];assert digest(image)==row['snapshotSha256'];pictures.append(image)
        records.append(dict(group=group,case=path.parent.name,pid=p['pid'],normalExit=True,coreSha256=p['runtimeSha256']['libvlccore.dll'],diagnostics=d['diagnostics']))
assert len(records)==21
assert not (work/'native-candidate-r3').exists()
for rev in ['r2','r3','r4']:
    assert (work/('build-'+('buffer' if rev=='r2' else 'refresh')+'-core-'+rev+'-exit.txt')).read_text().strip()=='0'
matrix=[]
names=['normal-pause-finnish','normal-pause-english','finnish-selected-paused','external-explicit-selected-paused']
for case in ['original-wasapi','preserve-wasapi','original-noaudio','colored-wasapi','colored-noaudio']:
    before=read(work/'native-before-r4'/case/'investigation.json');after=read(work/'native-candidate-r4'/case/'investigation.json')
    b={r['step']:r for r in before['observations']};a={r['step']:r for r in after['observations']}
    assert all(b[n]['brightPixels']==0 for n in names)
    assert all(a[n]['state']==3 and a[n]['snapshotAvailable'] and a[n]['brightPixels']>1000 for n in names)
    for name,language in [('normal-pause-finnish','fin'),('normal-pause-english','eng'),('finnish-selected-paused','fin')]:
        assert [t['language'] for t in a[name]['subtitleTracks'] if t['selected']]==[language]
    initialIds={t['id'] for t in a['seek-first-paused']['subtitleTracks']}
    selected=[t for t in a['external-explicit-selected-paused']['subtitleTracks'] if t['selected']]
    assert len(selected)==1 and selected[0]['id'] not in initialIds
    assert after['externalAddReturn']==0 and after['externalTrackAdded'] and after['externalExplicitSelected'] and after['externalResumeReached']
    log=(work/'native-candidate-r4'/case/'native.log').read_text(encoding='utf-8',errors='replace')
    refresh=[int(v) for v in re.findall(r'candidate paused subtitle refresh @(\d+)',log)]
    assert len(refresh)==4,(case,refresh)
    def last_point(row):
        points=[v['point']['ts'] for v in row['timerEvents'] if v['event']=='update']
        return points[-1] if points else None
    outputPoints=[last_point(a[n]) for n in ['played-paused','normal-pause-finnish','normal-pause-english','seek-first-paused','finnish-selected-paused','external-explicit-selected-paused']]
    if case.endswith('noaudio'):
        assert outputPoints[1]-outputPoints[0]==40_000 and outputPoints[2]-outputPoints[1]==40_000
        assert outputPoints[4]-outputPoints[3]==40_000 and outputPoints[5]-outputPoints[4]==40_000
        assert last_point(b['seek-first-paused'])==last_point(b['finnish-selected-paused'])==last_point(b['external-explicit-selected-paused'])
    if case.startswith('colored'):
        for name in names:assert a[name]['rgb'][0]>180 and max(a[name]['rgb'][1:])<80
        assert a['seek-middle-paused']['rgb'][2]>180 and max(a['seek-middle-paused']['rgb'][:2])<80
    matrix.append(dict(case=case,steps=names,beforeBrightPixels=[b[n]['brightPixels'] for n in names],candidateBrightPixels=[a[n]['brightPixels'] for n in names],pausedAtObservations=True,selectedTrackIdentitiesVerified=True,refreshTargetsUs=refresh,reportedOutputPointTimesUs=outputPoints,reportedPointDriftPerRefreshUs=40_000 if case.endswith('noaudio') else None,frameConservationNotAccepted=True,coldInitialBrightPixels=after['observations'][0]['brightPixels'],automaticExternalSelectionStillOpen=any(t['language']=='fin' and t['selected'] for t in a['external-added-selected-paused']['subtitleTracks']),resumeReached=after['externalResumeReached']))
assert not subprocess.check_output(['git','diff','--cached','--name-only'],cwd=repo,text=True).strip()
target.mkdir()
for path in work.iterdir():
    if path.is_file() and (path.suffix in ['.py','.mk','.sh'] or path.name.startswith(('inputs-','buffer-candidate-','refresh-candidate-','build-buffer-','build-refresh-'))):
        shutil.copyfile(path,target/path.name)
sources=target/'sources';sources.mkdir()
for source,name in [(repo/'src/player/input.c','player-input-before.c'),(repo/'src/input/decoder.c','decoder-before.c'),(work/'buffer-candidate-r2/decoder.c','decoder-queued-candidate.c'),(work/'refresh-candidate-r3/input.c','player-input-raw-timer-candidate.c'),(work/'refresh-candidate-r4/input.c','player-input-seek-aware-candidate.c')]:shutil.copyfile(source,sources/name)
for group in groups:
    for folder in (work/group).iterdir():
        if not folder.is_dir():continue
        destination=target/group/folder.name;destination.mkdir(parents=True)
        for path in folder.iterdir():
            if path.is_file() and path.suffix in ['.json','.txt','.log','.png']:shutil.copyfile(path,destination/path.name)
    shutil.copyfile(work/group/'processes.json',target/group/'processes.json')
write(target/'comparison.json',dict(candidateCaptionObservationsSupported=True,candidateRejectedForApplication=True,frameTimeDriftObserved=True,selectedRepairAccepted=False,fullBetaAccepted=False,cases=matrix,qualifiedReplacementObservations=20,baselineMissingCaptionObservations=20))
review=dict(productionApplied=False,selectedRepairAccepted=False,ordinaryAcceptedRuntime='build-windows/beta-caption-layout-20261023/ui-after-r1/runtime',acceptedCoreSha256='000f1159b7d190d83edaba83739b97b869669ca2eec2ba97bc35d6c51f9bb9cb',candidateCoreSha256='5837a5b7555099fd251385adb1caa109b1d6a8a56f93b41fc39515143840c4c5',
    findings=['Previous precise Matroska repair also restores backward-seek captions in all three replay cases.','New subtitle selection creates a decoder while the paused master does not demux new packets; merely allowing queued SPU consumption changes none of the observed cases.','Candidate refreshes newly selected subtitles on paused, seekable, nonrecording current input, using the existing seek-aware time query. Four reconstruction targets per case match ordinary pause time twice and the explicit 5-second seek twice.'],
    rejected={'r2':'Queued-SPU-only candidate: no behavior improvement in three actual native runs; never applied.','r3':'Raw timer candidate compiled but not run. Baseline time-watch data shows that the last raw source point can be 21ms while the paused clock is around 1s. R4 uses existing seek-aware interpolation.'},
    provenance=['Original child investigation labels passed=false and privateCore=false are inherited producer metadata; they do not identify the candidate build role. Parent runtime hashes and copied candidate sources are authoritative.','Child coreBuildScopeSha256 describes the earlier startup repair, not the newly compiled candidate player input object.','The candidate contains a diagnostic log. It is not the ordinary acceptance DLL.','Native diagnostics, including original buffer-deadlock prevention and vdummy window-closed teardown errors, remain preserved. No error-free-log claim.'],
    limitations=['Single paused observations and coarse colored regions do not prove identical video frame PTS or a frozen clock across each reconstruction.','Absolute public paused time remains stale in WASAPI cases after seeking; do not treat it as target PTS.','Cold initial captions can still be absent. b_select=true external addition can retain the prior selected ID; explicit selection is the successful action.','No new desktop, physical audibility, DPI, Windows10, independent review, packaging or full-beta acceptance.'],
    next='Promote a maintained native regression with frozen-pause and selection assertions; remove candidate-only logging; build ordinary Core and repeat native/desktop checks before applying acceptance.',
    noInstallCommitPushReleaseShutdown=True,fullBetaAccepted=False)
review['candidateRejectedForApplication']=True
review['frameTimeDrift']='Both no-audio cases advance the paused video-source timer point by 40,000us per selection refresh; the matching unchanged controls hold the point. WASAPI best-source points remain stale, so they cannot qualify exact video conservation.'
review['next']='Trace the current displayed-video/SMPTE source and seek target normalization, or use subtitle-only refill, before another candidate. Require repeated selection to conserve the paused picture/time, then maintained native and ordinary/desktop validation.'
shutil.copyfile(work/'publication-r5-failure.json',target/'publication-r5-failure.json')
write(target/'review.json',review)
(target/'README.md').write_text('''# Paused subtitle replacement investigation, 23 October 2026

Switching an embedded subtitle or explicitly selecting an external subtitle
while paused clears the current caption in the accepted ordinary build. Resume
restores it. The earlier Matroska repair fixes the backward-seek cue gap, but
does not itself supply data for a newly selected subtitle decoder.

A separate Core candidate requests a precise refill at the existing seek-aware
paused time after a newly selected subtitle is reported. Five unchanged controls
have twenty missing-caption observations. Five matching candidate runs show all
twenty captions, with verified English/Finnish/external selected identities.
Generated colored video retains the expected coarse color regions, and native
images show the Finnish/external text correctly. However, both no-audio cases
advance the paused video-source timer point by40ms on each selection refresh.
At25fps this is one frame. The matching unchanged controls retain the point.
This violates frame conservation, so the candidate is rejected for application.
The publication preflight caught this defect before writing the receipt; its
failed assertion is preserved rather than waived as an acceptance check.

This is candidate evidence, not an accepted production repair. The working-tree
player input and decoder sources remain unchanged. The accepted runtime is still
`build-windows/beta-caption-layout-20261023/ui-after-r1/runtime`, with Core000f.
The diagnostic candidate Core5837a5b7 remains separate and must not be promoted. A queued-SPU candidate
was rejected after three native runs; a raw-timer candidate was corrected before
execution because it could choose an old point after an ordinary play/Pause.

All21 native processes close normally, with188 snapshot records checked where
available. Native window-closed teardown errors and initial buffer warnings are
retained. A normal exit is not a whole-workflow pass. Cold initial captions,
automatic external selection, exact output time/frame conservation, ordinary
build verification and desktop checks remain open. No install, commit/push,
release/upload or shutdown occurred. The full beta goal stays active.

- [Paired candidate comparison](comparison.json)
- [Source reasoning, limitations and next validation](review.json)
- [Checkpoint and artifact hashes](checkpoint.json)
''',encoding='utf-8',newline='\n')
# Confirm the image count used in the prose against actual qualified records.
assert len(pictures)==188,len(pictures)
artifacts={p.relative_to(target).as_posix():digest(p) for p in sorted(target.rglob('*')) if p.is_file()}
write(target/'checkpoint.json',dict(artifacts=artifacts,artifactCount=len(artifacts),nativeProcessCount=len(records),nativePictureCount=len(pictures),records=records,sourceCount=latest['sourceCount'],sourceManifestSha256=digest(work/'inputs-r4.json'),productionApplied=False,selectedRepairAccepted=False,candidateReplacementObservations=20,candidateRejectedForApplication=True,frameTimeDriftObserved=True,fullBetaAccepted=False))
assert all(digest(target/p)==h for p,h in artifacts.items())
write(work/'publication-investigation-r5b.json',dict(passed=True,path=target.relative_to(repo).as_posix(),checkpointSha256=digest(target/'checkpoint.json'),artifactCount=len(artifacts),nativeProcessCount=len(records),nativePictureCount=len(pictures)))
print(json.dumps(read(work/'publication-investigation-r5b.json')))
