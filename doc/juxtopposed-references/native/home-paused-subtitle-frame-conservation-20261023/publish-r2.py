from pathlib import Path
from hashlib import sha256
import json,shutil,subprocess
work=Path(__file__).resolve().parent;repo=work.parent.parent
target=repo/'doc/juxtopposed-references/native/home-paused-subtitle-frame-conservation-20261023';assert not target.exists()
digest=lambda p:sha256(p.read_bytes()).hexdigest()
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
inputs=read(work/'inputs-r1.json');assert all(digest(repo/p)==h and digest(Path(inputs['sourceSnapshot'])/p)==h for p,h in inputs['files'].items())
assert digest(repo/'src/player/input.c')=='070effdd2dffbadc64c28668f0e49af99541ea8b4596f1bced769092c851a129'
assert digest(work/'candidate-r1/input.c')=='2181c37920bc7ac6c043b179662282f9120d49de1c4093ff03246cb03e55d4e6'
assert (work/'build-core-r1-exit.txt').read_text().strip()=='0'
assert digest(work/'candidate-r1-bin/libvlccore.dll')=='23d9b1b512ab297caadfe97b96437298f36bf343871d3baf7eeb840e0061dd2f'
frames=read(work/'media-r1/frames.json')['frames'];assert len(frames)==1500
assert all(abs(float(r['best_effort_timestamp_time'])-i/25)<.000001 for i,r in enumerate(frames))
phases={'before':2,'old':2,'candidate':7};records=[];snapshots=[];data={}
for phase,count in phases.items():
    root=work/('native-'+phase+'-r1');jobs=list(root.glob('*/process.json'));assert len(jobs)==count
    data[phase]={}
    for path in jobs:
        p=read(path);d=read(path.parent/'investigation.json');data[phase][path.parent.name]=d
        assert p['status']=='terminal' and p['normalExit'] and p['exitCode']==0 and not p['forcedCleanupUsed']
        assert p['runtimeUnchanged'] and p['sourceInputsUnchanged'] and p['producerUnchanged'] and p['markerMediaUnchanged']
        assert d['status']=='terminal' and d['teardownComplete'] and d['stopReached'] and d['externalResumeReached']
        assert all(digest(path.parent/'runtime'/name)==h for name,h in p['runtimeSha256'].items())
        for row in d['observations']:
            assert row['state']==3 and row['snapshotAvailable']
            assert all(s['state']==3 for s in row['pausedSamples']) and len({s['timeUs'] for s in row['pausedSamples']})==1
            image=path.parent/row['snapshot'];assert digest(image)==row['snapshotSha256'];snapshots.append(image)
        records.append(dict(phase=phase,case=path.parent.name,pid=p['pid'],normalExit=True,coreSha256=p['runtimeSha256']['libvlccore.dll'],diagnostics=d['diagnostics']))
normal=['normal-pause-finnish','normal-pause-english']
later=['finnish-selected-paused','repeat-0-eng','repeat-1-fin','repeat-2-eng','repeat-3-fin','external-explicit-selected-paused']
selected=normal+later;qualified=[]
for case,d in data['candidate'].items():
    rows={r['step']:r for r in d['observations']}
    assert all(rows[n]['brightPixels']>1000 for n in selected)
    for n in selected[:-1]:
        language='eng' if n in ['normal-pause-english','repeat-0-eng','repeat-2-eng'] else 'fin'
        assert [t['language'] for t in rows[n]['subtitleTracks'] if t['selected']]==[language]
    ids={t['id'] for t in rows['seek-first-paused']['subtitleTracks']}
    chosen=[t for t in rows['external-explicit-selected-paused']['subtitleTracks'] if t['selected']]
    assert len(chosen)==1 and chosen[0]['id'] not in ids
    if case.endswith('noaudio'):
        assert all(rows[n]['timeUs']==rows['played-paused']['timeUs'] for n in normal)
        assert all(rows[n]['timeUs']==rows['seek-first-paused']['timeUs'] for n in later)
    if case.startswith('marker'):
        assert rows['played-paused']['frameMarker']==25 and rows['seek-first-paused']['frameMarker']==125
        assert all(rows[n]['frameMarker']==25 for n in normal)
        assert all(rows[n]['frameMarker']==125 for n in later)
        assert rows['external-after-resume']['frameMarker']>125
    qualified.append(dict(case=case,selectedCaptionObservations=len(selected),frameMarkerConservationVerified=case.startswith('marker'),reportedPausedTimeConservationVerified=case.endswith('noaudio'),normalPauseFrames=[rows[n].get('frameMarker') for n in normal],seekFrames=[rows[n].get('frameMarker') for n in later],coldCaptionPixels=d['observations'][0]['brightPixels'],automaticExternalSelectionOpen=any(t['language']=='fin' and t['selected'] for t in rows['external-added-selected-paused']['subtitleTracks'])))
for case in ['marker-wasapi','marker-noaudio']:
    rows={r['step']:r for r in data['before'][case]['observations']}
    assert all(rows[n]['brightPixels']==0 for n in selected)
    assert all(rows[n]['frameMarker']==25 for n in normal) and all(rows[n]['frameMarker']==125 for n in later)
old={r['step']:r for r in data['old']['marker-noaudio']['observations']}
assert [old[n]['frameMarker'] for n in normal]==[26,27]
assert [old[n]['frameMarker'] for n in later]==[126,127,128,129,130,131]
assert len(records)==11 and len(snapshots)==154
assert not subprocess.check_output(['git','diff','--cached','--name-only'],cwd=repo,text=True).strip()
target.mkdir()
for path in work.iterdir():
    if path.is_file() and path.suffix in ['.py','.mk','.sh','.json','.txt','.log']:shutil.copyfile(path,target/path.name)
sources=target/'sources';sources.mkdir();shutil.copyfile(repo/'src/player/input.c',sources/'player-input-before.c');shutil.copyfile(work/'candidate-r1/input.c',sources/'player-input-candidate.c')
media=target/'fixture-reference';media.mkdir()
for name in ['probe.json','frames.json','frame-0.png','frame-125.png','frame-126.png','frame-625.png']:shutil.copyfile(work/'media-r1'/name,media/name)
for phase in phases:
    root=work/('native-'+phase+'-r1')
    for folder in root.iterdir():
        if not folder.is_dir():continue
        dest=target/root.name/folder.name;dest.mkdir(parents=True)
        for path in folder.iterdir():
            if path.is_file() and path.suffix in ['.json','.txt','.log','.png']:shutil.copyfile(path,dest/path.name)
    shutil.copyfile(root/'processes.json',target/root.name/'processes.json')
write(target/'comparison.json',dict(candidateSelectedCaptionObservations=56,candidateFrameConservationObservations=16,cases=qualified,unfixedCaptionAbsenceReproduced=True,oldCandidateActualFrameDriftReproduced=True,candidateFrameConservationSupported=True,productionApplied=False,selectedRepairAccepted=False,fullBetaAccepted=False))
review=dict(productionApplied=False,selectedRepairAccepted=False,fullBetaAccepted=False,ordinaryRuntime='build-windows/beta-caption-layout-20261023/ui-after-r1/runtime',acceptedCoreSha256='000f1159b7d190d83edaba83739b97b869669ca2eec2ba97bc35d6c51f9bb9cb',candidateCoreSha256='23d9b1b512ab297caadfe97b96437298f36bf343871d3baf7eeb840e0061dd2f',
    finding='Timer video points include the VLC_TICK_0 origin. The demux precise seek adds that origin to its relative date. Use the current displayed-video/SMPTE point, remove its origin for the demux target and retain the original point for timer publication. Pending user targets keep their existing domain.',
    evidence='Independently generated 11-bit red/black frame marker; 1500 frames at25fps and four direct FFmpeg reference extractions. Actual native snapshots show old candidate advancing125 to131 without audio, while the corrected candidate retains125 with captions through six changed selections and25 through two changes, in both audio modes.',
    limitations=['Private candidate with diagnostic logging; not the ordinary acceptance build.','The legacy child passed=false/privateCore=false labels are inherited probe metadata, not build-role proof. Parent runtime hashes identify all variants.','WASAPI public paused get_time remains stale or changes after reconstruction; only actual marker frames qualify its picture conservation.','No exact-caption-layout, ASS/MP4/unknown-frame-rate, pending-seek stress, disabled/recording policy, desktop, physical-audibility, DPI or full-beta acceptance.','Cold initial captions and b_select=true automatic external selection remain open; explicit selected-ID replacement is the qualified action.','Native buffer-deadlock prevention and vdummy window-closed errors remain recorded; no error-free-log claim.'],
    next='Promote a portable maintained frame-marker/subtitle replacement regression; remove candidate-only logging, build ordinary Core and repeat native and actual desktop checks. Broaden to ASS/other containers and policy controls before completing subtitle workflows.')
write(target/'review.json',review)
(target/'README.md').write_text('''# Paused subtitle frame conservation, 23 October 2026

The corrected Core candidate restores paused subtitle replacement and preserves
the actual video frame in the generated marker test. Timer video points include
the VLC_TICK_0 origin; demux seeks add it to a relative media date. The candidate
uses the current displayed-video/SMPTE point, removes its origin for the demux
request, and keeps the original point for timer publication. Pending user seeks
retain their own target domain.

An independent11-bit frame marker identifies1500 frames at25fps. The unchanged
build holds frames25/125 but loses selected captions. The previous candidate
restores captions but advances125 to131 without audio. The corrected candidate
holds25 through two language changes and125 through six more changed selections,
with and without WASAPI. Captions remain visible and resume advances the marker.
Five other original/remuxed/colored cases also retain the replacement behavior;
all three no-audio cases conserve their reported paused times.

There are56 successful selected-caption observations,16 actual frame-marker
conservation observations,154 verified snapshots and11 normal native exits.
Source/runtime/fixture hashes remain unchanged. These are candidate results:
no production source has been applied, and the diagnostic DLL is not accepted
as the ordinary build. The working runtime remains
`build-windows/beta-caption-layout-20261023/ui-after-r1/runtime`, Core000f1159.

Next: maintained regression, ordinary Core build and desktop checks, followed
by ASS/other-container and policy coverage. Cold initial captions, automatic
external selection, exact caption layout and stale WASAPI public paused time
remain open. The full beta stays active. No install, commit/push, release/upload
or shutdown occurred. Native diagnostics are retained.

- [Paired frame and caption comparison](comparison.json)
- [Scope, source reasoning and remaining checks](review.json)
- [Source trace](normalization-investigation-r1.json)
- [Checkpoint and artifact hashes](checkpoint.json)
''',encoding='utf-8',newline='\n')
artifacts={p.relative_to(target).as_posix():digest(p) for p in sorted(target.rglob('*')) if p.is_file()}
write(target/'checkpoint.json',dict(artifacts=artifacts,artifactCount=len(artifacts),sourceCount=inputs['sourceCount'],sourceManifestSha256=digest(work/'inputs-r1.json'),nativeProcessCount=11,nativeSnapshotCount=154,candidateCaptionObservations=56,candidateFrameConservationObservations=16,productionApplied=False,selectedRepairAccepted=False,fullBetaAccepted=False))
assert all(digest(target/p)==h for p,h in artifacts.items())
write(work/'publication-r2.json',dict(passed=True,path=target.relative_to(repo).as_posix(),checkpointSha256=digest(target/'checkpoint.json'),artifactCount=len(artifacts),nativeProcessCount=11,nativeSnapshotCount=154))
print(json.dumps(read(work/'publication-r2.json')))
