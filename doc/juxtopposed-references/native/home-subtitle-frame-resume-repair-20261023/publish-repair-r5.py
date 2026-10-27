from pathlib import Path
from hashlib import sha256
import json, shutil, subprocess

work = Path(__file__).resolve().parent
repo = work.parent.parent
target = repo / 'doc/juxtopposed-references/native/home-subtitle-frame-resume-repair-20261023'
read = lambda p: json.loads(p.read_text(encoding='utf-8-sig'))
digest = lambda p: sha256(p.read_bytes()).hexdigest()

def write(p, value):
    p.write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8', newline='\n')

def copy(p, destination):
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(p, destination)

assert not target.exists()
inputs = read(work / 'inputs-ordinary-r3.json')
assert inputs['sourceCount'] == len(inputs['files']) == 2372
for name, expected in inputs['files'].items():
    assert digest(repo / name) == expected, name
    assert digest(Path(inputs['sourceSnapshot']) / name) == expected, name
application = read(work / 'application-r3.json')
assert application['productionApplied']
for name, expected in application['files'].items():
    assert digest(repo / name) == expected
    assert digest(work / 'production-preimage' / name) == application['preimage'][name]
core = digest(work / 'ordinary-core-r3/libvlccore.dll')
assert core == '0ef9dde2f0a221135cab0a548ac292dd89d7cc9b7b79427eaaba121995b7773f'
assert (work / 'build-ordinary-r3-exit.txt').read_text().strip() == '0'
assert (work / 'build-candidate-r2-exit.txt').read_text().strip() == '0'
assert not subprocess.check_output(['git', 'diff', '--cached', '--name-only'], cwd=repo, text=True).strip()
subprocess.run(['git', 'diff', '--check'], cwd=repo, check=True)

rows, snapshots = [], []
media = repo / 'build-windows/beta-external-subtitle-acceptance-20261023/media-r1'
for folder, count in [('native-controls-r1', 4), ('native-candidate-r2', 7), ('native-ordinary-r3', 7)]:
    parent = read(work / folder / 'processes.json')
    assert len(parent['records']) == count and parent['allProcessesNormal']
    paths = sorted((work / folder).glob('*/process.json'))
    assert len(paths) == count
    for p in paths:
        record, result = read(p), read(p.parent / 'probe/result.json')
        assert record['status'] == result['status'] == 'terminal'
        assert record['normalExit'] and record['exitCode'] == 0 and not record['forcedCleanupUsed']
        assert result['teardownComplete'] and result['stopReached']
        assert record['sourceInputsUnchanged'] and record['runtimeUnchanged'] and record['fixturesUnchanged']
        assert all(digest(p.parent / 'runtime' / n) == h for n, h in record['runtimeSha256'].items())
        assert all(digest(media / n) == h for n, h in record['fixtureSha256'].items())
        if folder == 'native-controls-r1':
            assert not result['checks']['noUnrequestedFrameFeedback']
            assert result['checks']['externalDistinct'] == p.parent.name.startswith('hold-')
        else:
            assert result['passed'] and all(result['checks'].values())
            assert record['privateCore'] == (folder == 'native-candidate-r2')
            if folder == 'native-ordinary-r3':
                assert record['runtimeSha256']['libvlccore.dll'] == core
            held = next(o['frameMarker'] for o in result['observations'] if o['step'] == 'external-explicit-selected-paused')
            for offset in [1, 2]:
                assert next(o['frameMarker'] for o in result['observations'] if o['step'] == f'requested-next-{offset}') == held + offset
        for observation in result['observations']:
            assert observation['snapshotAvailable']
            picture = p.parent / 'probe' / observation['snapshot']
            assert digest(picture) == observation['snapshotSha256']
            snapshots.append(picture)
        rows.append(dict(phase=folder, case=p.parent.name, pid=record['pid'], checks=result['checks'],
                         coreSha256=record['runtimeSha256']['libvlccore.dll'],
                         markers={o['step']: o['frameMarker'] for o in result['observations']},
                         diagnostics=result['diagnostics']))
assert len(snapshots) == 306

regressions = read(work / 'regressions-ordinary-r4/processes.json')
assert regressions['passed'] and len(regressions['records']) == 17
for p in (work / 'regressions-ordinary-r4').glob('*/process.json'):
    record = read(p)
    assert record['status'] == 'terminal' and record['passed'] and record['normalExit'] and record['exitCode'] == 0
    assert not record['privateCore'] and not record['forcedCleanupUsed']
    assert all(record[k] for k in ['sourceInputsUnchanged', 'runtimeUnchanged', 'fixturesUnchanged', 'producerUnchanged'])
    assert record['runtimeSha256']['libvlccore.dll'] == core
    assert all(digest(p.parent / 'runtime' / n) == h for n, h in record['runtimeSha256'].items())
    assert all(digest(Path(n)) == h for n, h in record['fixtureSha256'].items())

gui = read(work / 'ui-observations-r5.json')
assert gui['ordinaryCore'] and gui['ordinaryGui'] and len(gui['cases']) == 2
assert {c['externalFile'] for c in gui['cases']} == {'external.srt', 'multi-language.mks'}
for case in gui['cases']:
    p = work / ('ui-' + case['externalFile'] + '-r4/verification.json')
    record = read(p)
    assert record['status'] == 'terminal' and record['normalExit'] and record['exitCode'] == 0 and record['pid'] == case['pid']
    assert not record['privateCore'] and not record['privateGui'] and not record['forcedCleanupUsed'] and not record['diagnostics']
    assert all(record[k] for k in ['sourceInputsUnchanged', 'runtimeUnchanged', 'fixturesUnchanged'])
    assert record['runtimeSha256']['libvlccore.dll'] == core
    assert all(digest(p.parent / 'runtime' / n) == h for n, h in record['runtimeSha256'].items())
    for flag in ['externalFileOpenedViaSubtitleMenu', 'externalAutomaticallySelected', 'onlyOneSubtitleSelected',
                 'genuineNextFrameFeedbackVisible', 'visibleMarkerPatternAdvanced',
                 'noMisleadingStepFeedbackObservedOnSubtitleLoad', 'captionsSurviveStepAndResume',
                 'realSeekAfterStepsVerified', 'normalQuitRequested']:
        assert case[flag], (case['externalFile'], flag)

target.mkdir()
for p in work.iterdir():
    if p.is_file() and p.suffix in ['.py', '.sh', '.json', '.txt', '.log', '.patch', '.mk']:
        copy(p, target / p.name)
for folder in ['ordinary-core-r3', 'candidate-r2', 'candidate-r2-bin', 'production-preimage']:
    for p in (work / folder).rglob('*'):
        if p.is_file() and p.suffix in ['.c', '.h', '.Plo', '.d', '.json', '.txt']:
            copy(p, target / p.relative_to(work))
for folder in ['native-controls-r1', 'native-candidate-r2', 'native-ordinary-r3', 'regressions-ordinary-r4']:
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
    for p in (work / ('ui-' + case['externalFile'] + '-r4')).iterdir():
        if p.is_file():
            copy(p, target / p.relative_to(work))
for name in list(application['files']) + ['test/Makefile.am', 'test/libvlc/native_paused_subtitle_tracks.py',
        'test/libvlc/native_paused_subtitle_tracks.md', 'test/libvlc/native_player_callback_layout.c']:
    copy(repo / name, target / 'sources/after' / name)
abi = repo / 'build-windows/beta-subtitle-frame-feedback-20261023'
for name in ['layout-r1.json', 'layout-provenance-r1.json']:
    copy(abi / name, target / 'callback-layout' / name)

write(target / 'comparison.json', dict(records=rows, holdControls=2, stepControls=2,
    privatePasses=7, ordinaryPasses=7, regressionPasses=17, nativeNormalExits=35,
    selectionAndStepSnapshots=len(snapshots), guiNormalExits=2, fullBetaAccepted=False))
limits = [
    'The accepted scope is paused subtitle replacement/addition, two genuine next-frame requests and caption-preserving resume in the paired generated fixtures; this is not whole-subtitle or full-beta acceptance.',
    'Long stepping across cue boundaries, previous-frame transitions, pending/nonseekable/recording, repeated source removal/restart and broader multi-selection policies need separate coverage.',
    'Absolute seek PTS, stale WASAPI public paused time, exact GUI normalized-value conservation, cold initial cues and complete caption layout remain separate.',
    'Native missing-file/dummy-window diagnostics and compiler warnings remain preserved. No error-free-native-log claim.',
    'Actual desktop screenshots were reviewed through Computer Use, without saving or decoding their pixels. Native snapshot pixels provide quantitative frame checks.',
    'Windows10, physical audibility/outputs, actual OS DPI, independent review and a clean distributable remain separate beta gates.'
]
write(target / 'review.json', dict(productionApplied=True, selectedRepairAccepted=True, ordinaryBuildAccepted=True,
    fullBetaAccepted=False, sourceFiles=application['files'], coreSha256=core,
    runtime='build-windows/beta-subtitle-frame-resume-20261023/ui-external.srt-r4/runtime',
    cause='Matched paused holds keep the caption, whereas two genuine next-frame requests lose it on the old Core. Frame stepping discarded subtitle packets, then resume flushed already-demuxed subtitle cues while retaining video. Separately, internal paused flush pictures incorrectly completed user-step status.',
    change='Keep subtitle packets during stepping and preserve their decoder on retained-video frame-step resume only. Actual seeks still flush subtitle decoders. Track synthetic paused-refresh completion separately from genuine requested-frame feedback.',
    publicAbiChanged=False, callbacks='The maintained optional callback regression uses the matching compiled C layout and copies scalar data only.',
    ordinaryNativePasses=24, nativeNormalExits=35, guiNormalExits=2, sourceInputCount=2372, limits=limits,
    next='Recheck preview hover on the current tuple; broaden subtitle boundaries and continue the complete thirteen-category/32-surface consumer, design/accessibility/DPI, real-media, review and clean-package roadmap.'))
(target / 'README.md').write_text('''# Subtitle frame-step resume repair, 23 October 2026

The ordinary build now retains external captions after two genuine frame steps
and playback resume in all seven maintained cases. Internal paused refreshes no
longer emit unsolicited Next frame status, while real requests still return two
successes and advance the actual held picture by one each. This repair is
accepted locally within the tested scope; the full beta remains active.

Four matched controls distinguish the failure: with the unchanged Core, both
paused holds retain captions and both stepped runs lose them after resume,
with and without WASAPI. Stepping discarded subtitle packets, and resume then
flushed already-demuxed cues. The fix retains those packets and skips subtitle
flush only for the existing retained-video frame-step-resume path. Actual seeks
continue to clear subtitle decoders, and audio resynchronization remains.
The decoder separately tracks the internal flush picture so its completion is
not reported as a user request. This changes no public structure or callback ABI.

All seven combined private cases and the same seven ordinary cases pass with
the maintained assertions intact: SubRip/Matroska, ASS/Matroska and mov_text/MP4,
with and without audio, plus a two-track external MKS source. All17 earlier
startup/interval regressions also pass. All35 native children exit normally;
306 paired control/selection/step snapshots verify. Native diagnostics and
compiler warnings remain recorded.

Two actual desktop sessions open external SRT/MKS through the subtitle file
dialog, verify sole automatic selection, make two genuine keyboard steps,
resume with the cue visible, and seek forward/backward to the expected cues.
MKS also checks Finnish selection and English restoration. An additional
deliberate step after the MKS seek checks visibly shows Next frame; a 250-ms
follow-up capture catches that transient feedback. Both sessions quit normally
without checked GUI errors. Desktop observations are separate from native
pixel-based frame checks and do not establish absolute seek PTS.

Current runtime:
`build-windows/beta-subtitle-frame-resume-20261023/ui-external.srt-r4/runtime`.
Tuple: Core0ef9dde2 / GUId0ff14c6 / MKVf46d8d15 / WASAPI4e4e.
Ordinary Core recompiles matching decoder/ES-output production sources with no
private objects or test hooks. The2,372 actual frozen input copies verify.
The earlier decoder-only candidate remains rejected in its immutable receipt;
this combined repair supersedes that private-only direction.

Long stepping across cue boundaries, previous-frame transitions, broader
subtitle policies, full caption layout and original preview-hover attribution
remain open. Continue all13 categories/32 surfaces, design/accessibility/DPI,
consolidated real-media checks, physical/platform limits, independent review
and clean packaging. No install, commit/push, release/upload or shutdown.

- [Paired results](comparison.json)
- [Actual desktop observations](ui-observations-r5.json)
- [Scope and limits](review.json)
- [Ordinary broader regressions](regressions-ordinary-r4/processes.json)
- [Artifact hashes](checkpoint.json)
''', encoding='utf-8', newline='\n')
artifacts = {p.relative_to(target).as_posix(): digest(p) for p in sorted(target.rglob('*')) if p.is_file()}
write(target / 'checkpoint.json', dict(artifacts=artifacts, artifactCount=len(artifacts), sourceInputCount=2372,
    sourceManifestSha256=digest(work / 'inputs-ordinary-r3.json'), nativeProcessCount=35, guiProcessCount=2,
    selectionAndStepSnapshotCount=len(snapshots), productionApplied=True, selectedRepairAccepted=True,
    ordinaryBuildAccepted=True, fullBetaAccepted=False))
assert all(digest(target / name) == expected for name, expected in artifacts.items())
write(work / 'publication-r5.json', dict(passed=True, path=target.relative_to(repo).as_posix(),
    checkpointSha256=digest(target / 'checkpoint.json'), artifactCount=len(artifacts),
    nativeProcessCount=35, guiProcessCount=2, selectionAndStepSnapshotCount=len(snapshots), selectedRepairAccepted=True))
print(json.dumps(read(work / 'publication-r5.json')))
