from pathlib import Path
from hashlib import sha256
import json, shutil, subprocess

work = Path(__file__).resolve().parent
repo = work.parent.parent
target = repo / 'doc/juxtopposed-references/native/home-external-subtitle-selection-repair-20261023'
read = lambda p: json.loads(p.read_text(encoding='utf-8-sig'))
digest = lambda p: sha256(p.read_bytes()).hexdigest()

def write(p, value):
    p.write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8', newline='\n')

def copy(p, destination):
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(p, destination)

assert not target.exists()
inputs = read(work / 'inputs-ordinary-r2.json')
assert inputs['sourceCount'] == 2340
for name, expected in inputs['files'].items():
    assert digest(repo / name) == expected, name
    assert digest(Path(inputs['sourceSnapshot']) / name) == expected, name
application = read(work / 'application-r2.json')
assert application['productionApplied']
for name, expected in application['files'].items():
    assert digest(repo / name) == expected
    assert digest(work / 'production-preimage' / name) == application['preimage'][name]
core = digest(work / 'ordinary-core-r2/libvlccore.dll')
assert core == 'ea14300c8c8366a99d17eab124216f543287822c18a85ed305eea0cc557b23d4'
assert (work / 'build-ordinary-r2-exit.txt').read_text().strip() == '0'
assert (work / 'generate-fixtures-r1-exit.txt').read_text().strip() == '0'
assert not subprocess.check_output(['git', 'diff', '--cached', '--name-only'], cwd=repo, text=True).strip()
subprocess.run(['git', 'diff', '--check'], cwd=repo, check=True)

comparisons = []
snapshots = []
for phase in ['before', 'ordinary']:
    root = work / ('native-' + phase + '-r1')
    parent = read(root / 'processes.json')
    assert len(parent['records']) == 17 and parent['allProcessesNormal'] and parent['expectedBehaviorObserved']
    paths = sorted(root.glob('*/process.json'))
    assert len(paths) == 17
    for p in paths:
        record = read(p)
        result = read(p.parent / 'probe/result.json')
        assert record['status'] == 'terminal' and record['normalExit'] and record['exitCode'] == 0
        assert not record['forcedCleanupUsed'] and not record['privateCore']
        assert record['sourceInputsUnchanged'] and record['runtimeUnchanged'] and record['fixturesUnchanged']
        assert result['status'] == 'terminal' and result['teardownComplete'] and result['stopReached']
        assert all(digest(p.parent / 'runtime' / name) == value for name, value in record['runtimeSha256'].items())
        assert all(digest(work / 'media-r1' / name) == value for name, value in record['fixtureSha256'].items())
        assert digest(repo / 'test/libvlc/native_paused_subtitle_tracks.py') == record['producerSha256']
        assert result['passed'] == (phase == 'ordinary' or record['config']['beforePass'])
        if phase == 'ordinary':
            assert all(result['checks'].values()) and record['runtimeSha256']['libvlccore.dll'] == core
        for observation in result['observations']:
            assert observation['state'] == 3 and observation['snapshotAvailable']
            assert all(sample['state'] == 3 for sample in observation['pausedSamples'])
            assert len({sample['timeUs'] for sample in observation['pausedSamples']}) == 1
            image = p.parent / 'probe' / observation['snapshot']
            assert digest(image) == observation['snapshotSha256']
            snapshots.append(image)
        comparisons.append(dict(phase=phase, case=p.parent.name, pid=record['pid'], passed=result['passed'],
            checks=result['checks'], coreSha256=record['runtimeSha256']['libvlccore.dll'],
            markers={o['step']: o['frameMarker'] for o in result['observations']}, diagnostics=result['diagnostics']))
assert sum(row['passed'] for row in comparisons if row['phase'] == 'before') == 2
assert sum(row['passed'] for row in comparisons if row['phase'] == 'ordinary') == 17

regression = read(work / 'regressions-ordinary-r3/processes.json')
assert regression['passed'] and len(regression['records']) == 17
for p in (work / 'regressions-ordinary-r3').glob('*/process.json'):
    record = read(p)
    assert record['status'] == 'terminal' and record['normalExit'] and record['exitCode'] == 0 and record['passed']
    assert not record['forcedCleanupUsed'] and not record['privateCore']
    assert record['sourceInputsUnchanged'] and record['runtimeUnchanged'] and record['fixturesUnchanged'] and record['producerUnchanged']
    assert record['runtimeSha256']['libvlccore.dll'] == core
    assert all(digest(p.parent / 'runtime' / name) == value for name, value in record['runtimeSha256'].items())
    assert all(digest(Path(name)) == value for name, value in record['fixtureSha256'].items())

gui = read(work / 'ui-observations-r4.json')
assert gui['ordinaryGui'] and gui['ordinaryCore'] and len(gui['cases']) == 3
assert {case['externalFile'] for case in gui['cases']} == {'external.srt', 'external.ass', 'multi-language.mks'}
for case in gui['cases']:
    p = work / ('ui-' + case['externalFile'] + '-r3/verification.json')
    record = read(p)
    assert record['status'] == 'terminal' and record['normalExit'] and record['exitCode'] == 0 and record['pid'] == case['pid']
    assert not record['diagnostics'] and not record['forcedCleanupUsed'] and not record['privateCore'] and not record['privateGui']
    assert record['sourceInputsUnchanged'] and record['runtimeUnchanged'] and record['fixturesUnchanged']
    assert record['runtimeSha256']['libvlccore.dll'] == core
    assert all(digest(p.parent / 'runtime' / name) == value for name, value in record['runtimeSha256'].items())
    for flag in ['externalFileOpenedViaSubtitleMenu', 'externalAutomaticallySelected', 'onlyOneSubtitleSelected',
                 'sameTopMarkerPattern', 'pausedElapsedHeld', 'pausedObserved', 'resumeAndPauseObserved', 'normalQuitRequested']:
        assert case[flag], (case['externalFile'], flag)

target.mkdir()
for p in work.iterdir():
    if p.is_file() and p.suffix in ['.py', '.sh', '.json', '.txt', '.log']:
        copy(p, target / p.name)
for folder in ['ordinary-core-r2', 'media-r1', 'production-preimage']:
    for p in (work / folder).rglob('*'):
        if p.is_file() and p.suffix in ['.c', '.h', '.Plo', '.json', '.srt', '.ass', '.mks', '.png', '.txt']:
            copy(p, target / p.relative_to(work))
for folder in ['native-before-r1', 'native-ordinary-r1', 'regressions-ordinary-r3']:
    root = work / folder
    copy(root / 'processes.json', target / folder / 'processes.json')
    for out in root.iterdir():
        if not out.is_dir():
            continue
        for p in out.iterdir():
            if p.is_file() and p.suffix in ['.json', '.txt', '.log']:
                copy(p, target / p.relative_to(work))
        for p in (out / 'probe').rglob('*'):
            if p.is_file() and p.suffix in ['.json', '.txt', '.log', '.png']:
                copy(p, target / p.relative_to(work))
for case in gui['cases']:
    for p in (work / ('ui-' + case['externalFile'] + '-r3')).iterdir():
        if p.is_file():
            copy(p, target / p.relative_to(work))
for name in list(application['files']) + ['test/Makefile.am', 'test/libvlc/native_paused_subtitle_tracks.py',
        'test/libvlc/native_paused_subtitle_tracks.md', 'test/libvlc/generate_paused_subtitle_tracks.sh']:
    copy(repo / name, target / 'sources/after' / name)

write(target / 'comparison.json', dict(records=comparisons, beforePasses=2, beforeFailures=15,
    ordinarySelectionPasses=17, ordinaryRegressionPasses=17, nativeProcessCount=51,
    snapshotCount=len(snapshots), guiNormalExits=3, fullBetaAccepted=False))
write(target / 'review.json', dict(productionApplied=True, selectedRepairAccepted=True, ordinaryBuildAccepted=True,
    fullBetaAccepted=False, runtime='build-windows/beta-external-subtitle-acceptance-20261023/ui-external.srt-r3/runtime',
    coreSha256=core, sourceFiles=application['files'], sourceInputCount=2340,
    change='Explicit runtime subtitle load-and-select reuses exclusive user selection, with preferred language/priority inside a multi-track source. Subtitle source identity is set before demux opening; its PCR/reset/display-target controls cannot replace the main-media clock or paused picture target. Generic associated sources retain their prior pass-through behavior.',
    evidence='Identical maintained tests and fixtures: 15 old behavior failures plus 2 load-only passes; 17 new ordinary passes; 17 ordinary startup/interval regressions. All51 native children close normally. Three ordinary desktop file-dialog sessions check automatic selected captions, sole selection, held frame/elapsed text and resume/Pause.',
    earlierInvestigation='../home-external-subtitle-selection-investigation-20261023/README.md',
    limits=['Paused runtime subtitle addition is the accepted scope; this is not whole-subtitle or full-beta acceptance.',
        'Playing/pending/nonseekable/recording, repeated additions/removal/restart, multi-selection and generic associated audio/video still need targeted qualification.',
        'Cold initial captions, absolute seek PTS, stale WASAPI public paused time, exact caption layout, physical audibility, Windows10 and DPI remain separate.',
        'Native missing-file and dummy-window diagnostics and build warnings remain recorded. No error-free-native-log claim.',
        'Desktop screenshots were reviewed through Computer Use; native marker pixels provide the quantitative frame-conservation proof.',
        'The first accessible SubRip seek retains a requested slider value until track refresh reconciles it slightly. Elapsed text and actual held picture remain; exact GUI normalized-value conservation is not accepted.',
        'A transient Next frame OSD appears on some ASS/MKS subtitle refreshes despite a held picture and no step key. This reproduced feedback defect remains open.'],
    next='Correct the reproduced Next frame feedback during subtitle refresh; broaden subtitle workflow policies and cold initial/default/language/sync behavior, then continue all13 categories/32 surfaces through remaining consumers, design/accessibility/DPI, consolidated real-media, review and clean-package gates.'))
(target / 'README.md').write_text('''# External subtitle selection repair, 23 October 2026

Opening an external subtitle while paused now selects its preferred track as
the sole subtitle, displays its cue and retains the held picture in the tested
ordinary build. This repair is accepted locally; the full beta remains active.

The explicit request uses VLC's existing exclusive user-selection route.
Subtitle source identity is established before opening its demuxer, and that
source's PCR/reset/display-target controls no longer drive the main media clock.
This also fixes the held-picture/time loss exposed by a two-track MKS file.
Generic associated sources keep their prior behavior.

The maintained regression uses the same generated fixtures, worker and options
before and after: 15 behavior failures and two load-only passes on the earlier
ordinary Core, then all17 passes on ordinary Core `ea14300c`. The earlier17
startup/interval regressions also pass. All51 native children exit normally.
Three actual desktop sessions open SubRip, ASS and two-track MKS through the
subtitle file dialog, verify automatic/sole selection and retained picture/
elapsed text, resume, pause and close normally without checked GUI errors.
The first SubRip accessible seek's normalized value reconciles slightly on
track refresh; exact GUI slider-value conservation is not claimed.
A transient Next frame OSD on some ASS/MKS subtitle refreshes is a reproduced
feedback defect and remains open; no step key was sent and the picture stayed.

Runtime: `build-windows/beta-external-subtitle-acceptance-20261023/ui-external.srt-r3/runtime`.
Tuple: Coreea14300c / GUId0ff14c6 / MKVf46d8d15 / WASAPI4e4e.
Ordinary Core recompiles matching production sources and header dependents;
no private replacement objects or GUI hooks are used. The earlier candidate
failures remain in their immutable investigation receipt. Frozen source input
copies and all exported artifact hashes are verified before publication.

This qualifies paused runtime addition, not all subtitle paths. Playing,
pending/nonseekable/recording, repeated additions/removal/restart, multi-select,
cold captions, defaults/language/sync, exact layout, physical audibility,
Windows10, DPI, independent review and clean packaging retain separate gates.
Native missing-file/dummy-window diagnostics and compiler warnings are retained.
No install, commit/push, release/upload or shutdown occurred.

- [Paired results, checks and markers](comparison.json)
- [Reviewed desktop observations](ui-observations-r4.json)
- [Source scope and remaining work](review.json)
- [Ordinary broader regressions](regressions-ordinary-r3/processes.json)
- [Artifact hashes](checkpoint.json)
''', encoding='utf-8', newline='\n')
artifacts = {p.relative_to(target).as_posix(): digest(p) for p in sorted(target.rglob('*')) if p.is_file()}
write(target / 'checkpoint.json', dict(artifacts=artifacts, artifactCount=len(artifacts), sourceInputCount=2340,
    sourceManifestSha256=digest(work / 'inputs-ordinary-r2.json'), nativeProcessCount=51, guiProcessCount=3,
    snapshotCount=len(snapshots), productionApplied=True, selectedRepairAccepted=True,
    ordinaryBuildAccepted=True, fullBetaAccepted=False))
assert all(digest(target / name) == expected for name, expected in artifacts.items())
write(work / 'publication-r4.json', dict(passed=True, path=target.relative_to(repo).as_posix(),
    checkpointSha256=digest(target / 'checkpoint.json'), artifactCount=len(artifacts),
    nativeProcessCount=51, guiProcessCount=3, snapshotCount=len(snapshots), selectedRepairAccepted=True))
print(json.dumps(read(work / 'publication-r4.json')))
