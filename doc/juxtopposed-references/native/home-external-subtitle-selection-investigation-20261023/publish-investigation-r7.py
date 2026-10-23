from pathlib import Path
from hashlib import sha256
import json,shutil,subprocess
work=Path(__file__).resolve().parent;repo=work.parent.parent
read=lambda p:json.loads(p.read_text(encoding='utf-8-sig'))
digest=lambda p:sha256(p.read_bytes()).hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8',newline='\n')
def copy(p,d):d.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,d)
target=repo/'doc/juxtopposed-references/native/home-external-subtitle-selection-investigation-20261023'
assert not target.exists()
inputs=read(work/'inputs-r3.json');assert inputs['sourceCount']==2327
assert all(digest(repo/p)==h and digest(Path(inputs['sourceSnapshot'])/p)==h for p,h in inputs['files'].items())
base=repo/'build-windows/beta-subtitle-replacement-repair-20261023/ui-frame-marker.mkv-r3/runtime'
baseIds=read(base.parent/'verification.json')['runtimeSha256'];assert all(digest(base/p)==h for p,h in baseIds.items())
assert baseIds['libvlccore.dll']=='ef3e47fd08fa342954494584bbecbb03fb6a6e6631481c05700b0c76876ff41f'
cores={n:digest(work/('candidate-'+n+'-bin/libvlccore.dll')) for n in ['r1','r2','r3']}
assert cores==dict(r1='ee0ad6d1299583dd786d3ba3acb300257bb1ad1bb7fceb665236fa112c5b053d',r2='9a8e12c1c705500ef79d9e1e0a5245d067b49d3bb6d0f87909b82569373eb117',r3='67541fd99158afbda221761c293ef150ba19ad919f824fc77b3ada7ab549f4cf')
folders={'native-before-r1':10,'native-candidate-r1':1,'native-candidate-r2':10,'native-extra-r4':4,'native-extra-r5':2,'native-candidate-r3':10,'native-policy-r6':5}
records=[];snapshots=[];newest=[]
for folder,count in folders.items():
    paths=sorted((work/folder).glob('*/process.json'));assert len(paths)==count,(folder,len(paths))
    for p in paths:
        d=read(p);r=read(p.parent/'probe/result.json')
        assert d['status']=='terminal' and d['exitCode']==0 and d['normalExit'] and not d['forcedCleanupUsed']
        assert d['sourceInputsUnchanged'] and d['runtimeUnchanged'] and d['fixturesUnchanged']
        assert r['status']=='terminal' and r['teardownComplete'] and r['stopReached']
        assert all(digest(p.parent/'runtime'/n)==h for n,h in d['runtimeSha256'].items())
        worker=repo/'test/libvlc/native_paused_subtitle_tracks.py' if folder.startswith('native-candidate') or folder=='native-before-r1' else work/'native-tracks-extra-r3.py'
        assert digest(worker)==d['producerSha256']
        for row in r['observations']:
            assert row['state']==3 and row['snapshotAvailable']
            assert all(s['state']==3 for s in row['pausedSamples']) and len({s['timeUs'] for s in row['pausedSamples']})==1
            image=p.parent/'probe'/row['snapshot'];assert digest(image)==row['snapshotSha256'];snapshots.append(image)
        if folder in ['native-extra-r5','native-candidate-r3','native-policy-r6']:
            assert r['passed'] and all(r['checks'].values()) and d['runtimeSha256']['libvlccore.dll']==cores['r3']
            newest.append(dict(case=p.parent.name,folder=folder,checks=r['checks'],diagnostics=r['diagnostics']))
        records.append(dict(folder=folder,case=p.parent.name,pid=d['pid'],normalExit=True,passed=r['passed'],coreSha256=d['runtimeSha256']['libvlccore.dll'],checks=r['checks'],diagnostics=r['diagnostics'],markers={o['step']:o['frameMarker'] for o in r['observations']}))
assert len(records)==42 and len(snapshots)==653 and len(newest)==17
before=read(work/'native-before-r1/processes.json')['records'];assert len(before)==10
assert sum(r['passed'] for r in before)==2 and all(r['passed']==(r['config']['selection']=='preserve') for r in before)
first=read(work/'native-candidate-r1/frame-marker.mkv-wasapi-automatic-ids/probe/result.json')
assert not first['passed'] and first['automaticExternalSelectionObserved'] and not first['checks']['automaticExternalSelected']
second=read(work/'native-extra-r4/candidate-multi-language.mks/probe/result.json')
assert not second['passed'] and second['checks']['automaticExternalSelected'] and not second['checks']['externalAdditionFrameConserved']
broken={o['step']:o for o in second['observations']};assert broken['repeat-3-fin']['frameMarker']==125 and broken['external-added-selected-paused']['frameMarker']==2047
assert all(r['passed'] for r in read(work/'native-candidate-r2/processes.json')['records'])
assert not subprocess.check_output(['git','diff','--cached','--name-only'],cwd=repo,text=True).strip()
subprocess.run(['git','diff','--check'],cwd=repo,check=True)

target.mkdir()
for p in work.iterdir():
    if p.is_file() and p.suffix in ['.py','.sh','.mk','.json','.txt','.log']:copy(p,target/p.name)
for folder in ['candidate-r1','candidate-r2','candidate-r3','candidate-r1-bin','candidate-r2-bin','candidate-r3-bin','media-extra-r2b']:
    for p in (work/folder).iterdir():
        if p.is_file() and p.suffix in ['.c','.h','.d','.json','.srt','.ass','.mks']:copy(p,target/p.relative_to(work))
for folder in folders:
    root=work/folder
    if (root/'processes.json').exists():copy(root/'processes.json',target/folder/'processes.json')
    for out in root.iterdir():
        if not out.is_dir():continue
        for p in out.iterdir():
            if p.is_file() and p.suffix in ['.json','.txt','.log']:copy(p,target/p.relative_to(work))
        for p in (out/'probe').iterdir():
            if p.is_file() and p.suffix in ['.json','.txt','.log','.png']:copy(p,target/p.relative_to(work))
for name in ['input.c','es_out.c','es_out.h','es_out_source.c']:copy(repo/'src/input'/name,target/'production-before'/name)
for name in ['native_paused_subtitle_tracks.py','native_paused_subtitle_tracks.md']:copy(repo/'test/libvlc'/name,target/'maintained-tests'/name)
write(target/'comparison.json',dict(records=records,latestCandidateCases=newest,nativeProcessCount=42,snapshotCount=653,latestCandidatePasses=17,productionApplied=False,ordinaryBuildAccepted=False,fullBetaAccepted=False))
review=dict(productionApplied=False,selectedRepairAccepted=False,ordinaryBuildAccepted=False,fullBetaAccepted=False,ordinaryRuntime=base.relative_to(repo).as_posix(),ordinaryCoreSha256=baseIds['libvlccore.dll'],candidateCoreSha256=cores['r3'],candidateSources={('src/input/'+p.name):digest(p) for p in (work/'candidate-r3').iterdir()},productionSources={('src/input/'+n):digest(repo/'src/input'/n) for n in ['input.c','es_out.c','es_out.h','es_out_source.c']},
    findings=[
        'The runtime associated-media path marks a source for automatic selection. Existing string IDs or language preferences can veto the explicitly requested external track.',
        'The first candidate forced the new ES directly, but the main-ES hint may be stale after string-ID selection. It left Finnish and the external track selected together. The revised path reuses the existing exclusive user-selection control.',
        'Subtitle-only Matroska exposes an independent source-timing problem. Its demuxer publishes PCR, reset and next-display controls through the source wrapper, including during opening. These can replace the main video timing boundary and clock.',
        'The combined candidate identifies subtitle sources before demux opening and keeps their PCR/reset/display-target controls from driving the main media clock. Generic external sources retain their existing pass-through behavior.'
    ],
    evidence='Eight automatic-selection failures and two load-only passes on the accepted Core. Candidate1 failed exclusive selection; Candidate2 passed ten base cases but failed the two-track MKS clock case. Candidate3 passes ten base cases, external ASS/MKS plus invalid-path checks, and five MKS audio/load-only/language/disabled cases. Native snapshots show the selected text and actual frame markers; all42 children exit normally. Parent1 and extra4 intentionally stop on behavior failures.',
    limitations=['Private candidate only; no production Core source applied and no ordinary DLL/desktop acceptance for this repair.','The extra ASS/MKS/missing-path probes are private extensions; promote their options/checks into the maintained regression before ordinary acceptance.','Paused runtime requests are tested. Playing/pending/nonseekable/recording/repeated-add/removal/restart and multi-selection policies require broader checks.','Candidate2 MKS failure also exists after explicit selection on the unchanged build; restoring selection alone is insufficient.','Cold initial caption delivery, absolute seek PTS, WASAPI public paused time, exact layout, physical audibility, Windows10, DPI and the full beta remain open.','Native dummy-window teardown and intentionally missing-file errors are retained; no error-free-log claim.','The source-control filter is scoped to subtitle sources; generic associated audio/video behavior is unchanged but not newly qualified.'],
    next='Promote extra fixture options and missing-file checks into the maintained probe. Apply only the four combined candidate3 source files after preimage checks, build ordinary Core with header dependencies, repeat paired native and broad regressions, then verify actual desktop automatic selection before accepting the repair.')
write(target/'review.json',review)
(target/'README.md').write_text('''# External subtitle selection investigation, 23 October 2026

The combined private candidate selects a newly opened subtitle file and keeps
the paused picture in17 native cases, including SubRip/Matroska, ASS/Matroska,
mov_text/MP4, external ASS and a two-track subtitle-only Matroska file. Tests
cover WASAPI/no audio, previous ID/language/disabled choices, load-only requests,
missing files, explicit selection and resume. All42 native children exit
normally;653 snapshot hashes and2,327 actual frozen input copies verify.

There were three distinct problems. Existing selection preferences could veto
the load-and-select request. Directly forcing the new ES could leave the old
one selected because the main-ES hint was stale. Finally, a subtitle container
could reset the main video's PCR/preroll through its source wrapper. The final
candidate uses the existing exclusive user-selection path and keeps subtitle
source timing controls from changing the main media clock, starting at demux
opening. Generic associated sources keep their existing behavior.

The earlier candidates and failures are preserved. Candidate1 selected both
Finnish and the new file. Candidate2 passed the ten base cases but the MKS case
lost its held picture/time; the same MKS issue appears after explicit selection
on the unchanged build. Caption presence or a selected-ID flag alone therefore
does not qualify the workflow. The final candidate restores the actual marker
and text; reviewed native images show EXTERNAL FIRST CUE.

This is candidate evidence. No production Core source is applied, and no
ordinary build or actual desktop automatic-selection result is accepted here.
Current runtime stays
`build-windows/beta-subtitle-replacement-repair-20261023/ui-frame-marker.mkv-r3/runtime`,
Coreef3e47fd / GUId0ff14c6 / MKVf46d8d15 / WASAPI4e4e. Candidate67541fd9 must
still pass ordinary compilation, maintained regression and desktop gates.

Promote the extra ASS/MKS/missing-file checks into the maintained worker, apply
the four combined sources after preimage checks, then repeat native and visible
app checks. Playing/pending/nonseekable/recording/multi-select/repeated/restart
coverage, cold captions, layout/accessibility/DPI, physical outputs, Windows10,
review and clean packaging remain open under the full13-category/32-surface
beta goal. No install, commit/push, release/upload or shutdown occurred.
Native expected missing-file and dummy-window diagnostics remain visible.

- [Per-case checks, marker values and retained failures](comparison.json)
- [Source reasoning and next acceptance gates](review.json)
- [Combined proposed sources](candidate-r3)
- [Artifact hashes](checkpoint.json)
''',encoding='utf-8',newline='\n')
artifacts={p.relative_to(target).as_posix():digest(p) for p in sorted(target.rglob('*')) if p.is_file()}
write(target/'checkpoint.json',dict(artifacts=artifacts,artifactCount=len(artifacts),sourceInputCount=2327,sourceManifestSha256=digest(work/'inputs-r3.json'),nativeProcessCount=42,nativeSnapshotCount=653,candidatePasses=17,productionApplied=False,selectedRepairAccepted=False,fullBetaAccepted=False))
assert all(digest(target/p)==h for p,h in artifacts.items())
write(work/'publication-r7.json',dict(passed=True,path=target.relative_to(repo).as_posix(),checkpointSha256=digest(target/'checkpoint.json'),artifactCount=len(artifacts),nativeProcessCount=42,nativeSnapshotCount=653,candidatePasses=17,productionApplied=False))
print(json.dumps(read(work/'publication-r7.json')))
