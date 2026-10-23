from pathlib import Path
from hashlib import sha256
import json, shutil, subprocess

work=Path(__file__).resolve().parent
repo=work.parent.parent
target=repo/'doc/juxtopposed-references/native/home-paused-subtitle-replacement-repair-20261023'
digest=lambda p:sha256(p.read_bytes()).hexdigest()
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
def copy(p,d):d.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,d)

assert not target.exists()
inputs=read(work/'inputs-ordinary-r2.json')
assert inputs['sourceCount']==2290
for name,h in inputs['files'].items():
    assert digest(repo/name)==h,name
    assert digest(Path(inputs['sourceSnapshot'])/name)==h,name
application=read(work/'application-r3.json')
assert all(digest(repo/p)==h for p,h in application['files'].items())
core=digest(work/'ordinary-core-r2/libvlccore.dll')
assert core=='ef3e47fd08fa342954494584bbecbb03fb6a6e6631481c05700b0c76876ff41f'
assert b'candidate subtitle frame refresh' not in (work/'ordinary-core-r2/libvlccore.dll').read_bytes()
assert (work/'build-ordinary-r2-exit.txt').read_text().strip()=='0'
assert (work/'build-preroll-r2-exit.txt').read_text().strip()=='0'
assert not subprocess.check_output(['git','diff','--cached','--name-only'],cwd=repo,text=True).strip()

jobs=[];snapshots=[];comparisons=[]
for phase,count in [('before-r1',6),('candidate-r1',5),('candidate-r2',6),('ordinary-r1',6)]:
    paths=sorted((work/('native-'+phase)).glob('*/process.json'))
    assert len(paths)==count,(phase,len(paths))
    for p in paths:
        d=read(p);r=read(p.parent/'probe/result.json')
        assert d['status']=='terminal' and d['normalExit'] and d['exitCode']==0 and not d['forcedCleanupUsed']
        assert d['runtimeUnchanged'] and d['sourceInputsUnchanged'] and d['fixtureUnchanged']
        assert r['status']=='terminal' and r['teardownComplete'] and r['stopReached']
        assert all(digest(p.parent/'runtime'/n)==h for n,h in d['runtimeSha256'].items())
        assert all(digest(work/'media-r1'/n)==h for n,h in d['fixtureSha256'].items())
        assert digest(repo/'test/libvlc/native_paused_subtitle_tracks.py')==d['producerSha256']
        for o in r['observations']:
            assert o['snapshotAvailable'] and o['state']==3
            assert all(s['state']==3 for s in o['pausedSamples'])
            assert len({s['timeUs'] for s in o['pausedSamples']})==1
            image=p.parent/'probe'/o['snapshot']
            assert digest(image)==o['snapshotSha256']
            snapshots.append(image)
        if phase=='before-r1':assert not r['passed'] and not r['checks']['captionsPresent']
        elif phase=='candidate-r1' and p.parent.name=='frame-marker.mp4-wasapi':
            assert not r['passed'] and not r['checks']['normalFramesConserved'] and not r['checks']['seekFramesConserved']
        else:assert r['passed'] and all(r['checks'].values())
        if phase=='ordinary-r1':assert d['runtimeSha256']['libvlccore.dll']==core
        jobs.append(p)
        comparisons.append(dict(phase=phase,case=p.parent.name,pid=d['pid'],normalExit=True,passed=r['passed'],checks=r['checks'],markers={o['step']:o['frameMarker'] for o in r['observations']},diagnostics=r['diagnostics'],automaticExternalSelectionObserved=r['automaticExternalSelectionObserved']))

regressions=read(work/'regressions-ordinary-r3/processes.json')
assert regressions['passed'] and len(regressions['records'])==17
for p in sorted((work/'regressions-ordinary-r3').glob('*/process.json')):
    d=read(p);r=read(p.parent/'probe/result.json')
    assert d['status']=='terminal' and d['normalExit'] and d['passed'] and d['exitCode']==0 and not d['forcedCleanupUsed']
    assert d['runtimeUnchanged'] and d['sourceInputsUnchanged'] and d['fixturesUnchanged'] and d['producerUnchanged']
    assert d['runtimeSha256']['libvlccore.dll']==core
    assert all(digest(p.parent/'runtime'/n)==h for n,h in d['runtimeSha256'].items())
    assert all(digest(Path(n))==h for n,h in d['fixtureSha256'].items())
    jobs.append(p)

gui=read(work/'ui-observations-r4.json')
assert len(gui['cases'])==3
for c in gui['cases']:
    p=work/('ui-'+c['media']+'-r3/verification.json');d=read(p)
    assert d['status']=='terminal' and d['normalExit'] and d['exitCode']==0 and d['pid']==c['pid']
    assert not d['diagnostics'] and not d['forcedCleanupUsed']
    assert d['sourceInputsUnchanged'] and d['runtimeUnchanged'] and d['mediaUnchanged']
    assert c['languageSwitchVisible'] and c['sameTopMarkerPattern'] and c['pauseAndDisplayedPositionHeld']
    assert d['runtimeSha256']['libvlccore.dll']==core
    assert all(digest(p.parent/'runtime'/n)==h for n,h in d['runtimeSha256'].items())
assert not gui['cases'][0]['externalAutomaticallySelected'] and gui['cases'][0]['externalExplicitSelectionVisible']
assert len(jobs)==40 and len(snapshots)==345

# Every public result is copied only after preflight; staged runtimes stay ignored.
target.mkdir()
for p in work.iterdir():
    if p.is_file() and p.suffix in ['.py','.sh','.mk','.json','.txt','.log']:copy(p,target/p.name)
for folder in ['ordinary-object-audit-r4','ordinary-core-r2','candidate-preroll-r2','media-r1']:
    for p in (work/folder).rglob('*'):
        if p.is_file() and p.suffix in ['.c','.h','.Plo','.json','.txt','.png','.srt']:copy(p,target/p.relative_to(work))
for folder in ['native-before-r1','native-candidate-r1','native-candidate-r2','native-ordinary-r1','regressions-ordinary-r3']:
    root=work/folder
    if (root/'processes.json').exists():copy(root/'processes.json',target/folder/'processes.json')
    for out in root.iterdir():
        if not out.is_dir():continue
        for p in out.iterdir():
            if p.is_file() and p.suffix in ['.json','.txt','.log']:copy(p,target/p.relative_to(work))
        for p in (out/'probe').rglob('*'):
            if p.is_file() and p.suffix in ['.json','.txt','.log','.png']:copy(p,target/p.relative_to(work))
for c in gui['cases']:
    for p in (work/('ui-'+c['media']+'-r3')).iterdir():
        if p.is_file():copy(p,target/p.relative_to(work))
sources=list(application['files'])+['test/Makefile.am','test/libvlc/native_paused_subtitle_tracks.py','test/libvlc/generate_paused_subtitle_tracks.sh','test/libvlc/native_paused_subtitle_tracks.md']
for name in sources:copy(repo/name,target/'sources/after'/name)
for name in application['files']:copy(work/'source-tests-r1'/name,target/'sources/before'/name)
write(target/'comparison.json',dict(cases=comparisons,nativeProcessCount=40,replacementSnapshots=345,ordinaryReplacementPasses=6,ordinaryRegressionPasses=17,guiNormalExits=3))
review=dict(productionApplied=True,selectedRepairAccepted=True,ordinaryBuildAccepted=True,fullBetaAccepted=False,
    runtime='build-windows/beta-subtitle-replacement-repair-20261023/ui-frame-marker.mkv-r3/runtime',
    coreSha256=core,sourceFiles=application['files'],sourceInputCount=2290,nativeReplacementPasses=6,nativeRegressionPasses=17,guiSessions=3,
    change='Refill a newly selected subtitle at the frozen displayed-video timestamp with the proper media-time origin. Retain an exact paused decoded-picture boundary after packet preroll, so MP4 B-frame reordering cannot expose an earlier picture. Audio remains paused; ordinary sources and internal header are rebuilt together.',
    rejectedCandidate='Player-only candidate23d restored captions on Matroska but moved MP4 frames backwards. The maintained test caught this without changing thresholds. Its five normal results are preserved, including the failure. Combined candidate578d then passed all six cases before ordinary Coreef3e was built.',
    limits=[
        'Selected-subtitle replacement is the accepted scope, not the whole subtitle workflow or beta.',
        'Cold initial captions remain a separate acceptance task; automatic external selection fails in native and actual desktop checks. Explicit selection succeeds.',
        'Frame conservation compares the actual held picture with the picture after selection. It does not establish exact absolute seek PTS, including MP4 timestamp offsets.',
        'WASAPI public get_time can be stale or change after seek reconstruction; actual marker pixels qualify video conservation. No-audio reported paused time is also conserved.',
        'No recording/nonseekable/pending-seek stress, unknown frame rate, previous-frame stress, complete caption layout, Windows 10, DPI, physical audibility or full-beta acceptance.',
        'Existing native dummy-window teardown/buffering diagnostics and ordinary compiler warnings are retained; no error-free-native-log claim.',
        'Desktop keyboard/first Play activation was not qualified; see UI observation limits.'
    ],next='Resolve automatic external selection, cold initial cue delivery and language/default/synchronization workflows; broaden policy and seek/step stress coverage, then continue all13 categories and32 surfaces through design/accessibility/DPI/platform/review/package gates.')
write(target/'review.json',review)
(target/'README.md').write_text('''# Paused subtitle replacement repair, 23 October 2026

Changing a selected subtitle while paused now displays the new cue and preserves
the held video picture in the tested SubRip/Matroska, ASS/Matroska and mov_text/MP4
fixtures, with WASAPI and with audio disabled. This selected repair is accepted
locally on ordinary Core `ef3e47fd`; the full beta remains active.

The player refills the new subtitle at the displayed-video timestamp, converting
its time origin for the demux seek. The decoder retains the exact paused picture
boundary after packet preroll. The second part is necessary for reordered MP4
video: the maintained regression rejected the earlier player-only candidate
despite its restored captions. That failure is retained here.

The same portable regression reproduces missing captions in six unchanged
controls. The combined private candidate then passes six cases; the ordinary
build independently passes all six, with 90 paused snapshots. All17 earlier
startup and subtitle-interval regressions pass. There are40 normal native exits
across controls/candidates/regressions and345 replacement snapshots. Three
ordinary desktop sessions show English-to-Finnish changes with a held marker
pattern and timeline, and close normally with no checked GUI errors. SubRip also
shows English restoration, explicit external loading and resume/Pause.

External files load, but automatic selection still retains the previous track;
explicitly choosing the new track works. Cold initial cues, absolute seek PTS,
stale WASAPI reported time, policy/step/concurrent-seek stress, complete caption
layout, Windows10, DPI and physical audibility retain separate gates. Desktop
observations are not pixel metrics. Native diagnostics and compiler warnings
are preserved. No install, commit/push, release/upload or shutdown occurred.

Current runtime:
`build-windows/beta-subtitle-replacement-repair-20261023/ui-frame-marker.mkv-r3/runtime`.
Tuple: GUId0ff14c6 / Coreef3e47fd / MKVf46d8d15 / WASAPI4e4e.
The earlier fade, accessible seeking and Matroska interval repairs retain their
tested scopes. All2,290 frozen source inputs were copied and verified.

- [Per-case checks and actual frame markers](comparison.json)
- [Scope and remaining beta work](review.json)
- [Reviewed visible app observations](ui-observations-r4.json)
- [Ordinary regression results](regressions-ordinary-r3/processes.json)
- [Artifact hashes](checkpoint.json)
''',encoding='utf-8',newline='\n')
artifacts={p.relative_to(target).as_posix():digest(p) for p in sorted(target.rglob('*')) if p.is_file()}
write(target/'checkpoint.json',dict(artifacts=artifacts,artifactCount=len(artifacts),sourceInputCount=2290,sourceManifestSha256=digest(work/'inputs-ordinary-r2.json'),productionApplied=True,selectedRepairAccepted=True,ordinaryBuildAccepted=True,fullBetaAccepted=False,nativeProcessCount=40,guiProcessCount=3,replacementSnapshotCount=345))
assert all(digest(target/p)==h for p,h in artifacts.items())
write(work/'publication-r4.json',dict(passed=True,path=target.relative_to(repo).as_posix(),checkpointSha256=digest(target/'checkpoint.json'),artifactCount=len(artifacts),nativeProcessCount=40,guiProcessCount=3))
print(json.dumps(read(work/'publication-r4.json')))
