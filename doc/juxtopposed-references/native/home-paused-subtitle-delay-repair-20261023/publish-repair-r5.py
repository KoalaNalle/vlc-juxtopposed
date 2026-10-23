from pathlib import Path
from hashlib import sha256
import json, shutil, subprocess

work = Path(__file__).resolve().parent
repo = work.parent.parent
target = repo/'doc/juxtopposed-references/native/home-paused-subtitle-delay-repair-20261023'
read = lambda p: json.loads(p.read_text(encoding='utf-8-sig'))
digest = lambda p: sha256(p.read_bytes()).hexdigest()


def write(p, value):
    p.write_text(json.dumps(value, indent=2)+'\n', encoding='utf-8', newline='\n')


def copy(p, destination):
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(p, destination)
    assert digest(p) == digest(destination)


assert not target.exists()
inputs = read(work/'inputs-ordinary-r1.json')
assert len(inputs['files']) == inputs['sourceCount'] == 2400
for name, h in inputs['files'].items():
    assert digest(repo/name) == h and digest(Path(inputs['sourceSnapshot'])/name) == h
application = read(work/'application-r1.json')
assert application['productionApplied']
assert all(digest(repo/p) == h for p, h in application['files'].items())
assert all(digest(work/'production-preimage-r1'/p) == h for p, h in application['preimages'].items())
assert (work/'build-ordinary-r1-exit.txt').read_text().strip() == '0'
core = digest(work/'ordinary-core-r1/libvlccore.dll')
assert core == '7ab8075014b11a72c5b51e41e0e067fcb8966505b19d23614d0c5ddc1f4ba53f'
build = (work/'build-ordinary-r1.log').read_text(encoding='utf-8', errors='replace')
assert 'candidate-r4' not in build and 'owned-subtitle-delay-core' not in build
for path in (work/'ordinary-core-r1').glob('*.Plo'):
    dependencies = path.read_text(encoding='utf-8', errors='replace')
    assert 'candidate-r4' not in dependencies and 'owned-subtitle-delay-core' not in dependencies
    assert '/src/' in dependencies
rows = []
timing_snapshots = 0
paired_frames = 0
for folder, count in [('native-ordinary-r2', 11), ('regressions-ordinary-r3', 24)]:
    summary = read(work/folder/'processes.json')
    assert summary['passed'] and summary['allProcessesNormal'] and len(summary['records']) == count
    for record in summary['records']:
        assert record['status'] == 'terminal' and record['normalExit'] and record['exitCode'] == 0
        assert record['passed'] and not record['privateCore'] and not record['forcedCleanupUsed']
        assert record['sourceInputsUnchanged'] and record['runtimeUnchanged'] and record['fixturesUnchanged']
        assert record['runtimeSha256']['libvlccore.dll'] == core
        assert all(digest(work/folder/'runtime'/p) == h for p, h in record['runtimeSha256'].items())
        result = record['result']
        if folder == 'native-ordinary-r2':
            assert result['status'] == 'terminal' and result['stopReached'] and result['teardownComplete']
            assert all(result['checks'].values()) and record['pairedFramesMatch']
            assert digest(repo/record['reference']) == record['referenceSha256']
            paired_frames += len(record['pairedFrames'])
            for observation in result['observations']:
                assert observation['snapshotAvailable']
                assert digest(work/folder/record['case']/'probe'/observation['snapshot']) == observation['snapshotSha256']
                timing_snapshots += 1
        else:
            assert record['parentUnchanged']
        rows.append(dict(phase=folder, case=record['case'], pid=record['pid'],
                         passed=True, coreSha256=core, checks=result.get('checks'),
                         diagnostics=result.get('diagnostics', [])))
assert len(rows) == 35 and timing_snapshots == 121 and paired_frames == 44
gui = read(work/'ui-observations-r4.json')
record = read(work/'ui-frame-marker-ass.mkv-r4/verification.json')
assert gui['ordinaryCore'] and gui['ordinaryGui'] and gui['status'] == 'terminal'
assert gui['normalExitVerified'] and gui['sourceRuntimeFixturesUnchanged'] and not gui['checkedGuiDiagnostics']
assert record['status'] == 'terminal' and record['normalExit'] and record['exitCode'] == 0
assert record['pid'] == gui['pid'] and not record['diagnostics']
assert not record['privateCore'] and not record['privateGui'] and not record['forcedCleanupUsed']
assert all(record[p] for p in ['sourceInputsUnchanged', 'runtimeUnchanged', 'fixturesUnchanged'])
assert record['runtimeSha256']['libvlccore.dll'] == core
assert all(digest(work/'ui-frame-marker-ass.mkv-r4/runtime'/p) == h for p, h in record['runtimeSha256'].items())
assert len(gui['cases']) == 2
ass, mp4 = gui['cases']
assert ass['delayedCaption'] == ass['adjustedCaptionAt600Ms'] == 'SUOMI ENSIMMÄINEN'
assert ass['firstResetCaption'] == ass['secondResetCaption'] == 'SUOMI KESKIMMÄINEN'
assert ass['initialPrimaryDelayMs'] == mp4['nextMediaPrimaryDelayMs'] == 1200
assert ass['savedNativeDefaultAfterReset'] == 12 and mp4['savedDefaultReappliedAfterPreviousFileReset']
assert mp4['delayedCaption'] == 'SUOMI ENSIMMÄINEN' and mp4['resetCaption'] == 'SUOMI KESKIMMÄINEN'
assert mp4['resumedCaption'] == 'SUOMI KESKIMMÄINEN' and mp4['normalQuitRequestedWhilePlaying']
assert not ass['unrequestedNextFrameFeedbackObserved'] and not mp4['unrequestedNextFrameFeedbackObserved']
assert 'sub-delay=12' in (work/'ui-frame-marker-ass.mkv-r4/config-after.txt').read_text(encoding='utf-8-sig')
subprocess.run(['git', 'diff', '--check'], cwd=repo, check=True)
assert not subprocess.check_output(['git', 'diff', '--cached', '--name-only'], cwd=repo, text=True).strip()

target.mkdir()
for p in work.iterdir():
    if p.is_file() and p.suffix in ['.py', '.sh', '.json', '.txt', '.log']:
        copy(p, target/p.name)
for p in (work/'ordinary-core-r1').glob('*.Plo'):
    copy(p, target/p.relative_to(work))
for folder, count in [('native-ordinary-r2', 11), ('regressions-ordinary-r3', 24)]:
    root = work/folder
    copy(root/'processes.json', target/folder/'processes.json')
    for out in root.iterdir():
        if out.is_file() and out.suffix in ['.py', '.json', '.txt', '.log']:
            copy(out, target/out.relative_to(work))
        if not out.is_dir() or out.name == 'runtime':
            continue
        for p in out.iterdir():
            if p.is_file() and p.suffix in ['.json', '.txt', '.log']:
                copy(p, target/p.relative_to(work))
        for p in (out/'probe').rglob('*'):
            if p.is_file() and p.suffix in ['.json', '.txt', '.log', '.png']:
                copy(p, target/p.relative_to(work))
for p in (work/'ui-frame-marker-ass.mkv-r4').iterdir():
    if p.is_file() and p.suffix in ['.json', '.txt', '.log']:
        copy(p, target/p.relative_to(work))
for name in application['files']:
    copy(repo/name, target/'sources/after'/name)
    copy(work/'production-preimage-r1'/name, target/'sources/before'/name)
for name in ['test/Makefile.am', 'test/libvlc/native_paused_subtitle_delay.py',
             'test/libvlc/native_paused_subtitle_delay.md']:
    copy(repo/name, target/'sources/maintained'/name)
limits = gui['limits'] + [
    'Accepted scope is delayed paused seeking and current primary delay/reset, including next-file saved defaults, in the recorded generated media. Complete subtitle workflows remain open.',
    'Per-track/secondary offsets and multiple selected tracks with independent delays, unknown-length/nonseekable/recording behavior, larger offsets, nonzero origins and more cue boundaries require separate qualification.',
    'Native dummy-window diagnostics and compiler warnings remain preserved; no error-free-native-log claim or physical audibility claim.',
    'Actual OS DPI, screen readers, Windows10 comparison, remaining consumer/design work, independent review and clean packaging remain full-beta gates.'
]
write(target/'comparison.json', dict(records=rows, ordinaryTargetedPasses=11,
      ordinaryLegacyPasses=24, nativeNormalExits=35, timingSnapshots=121,
      matchedCandidateOrdinarySeekFrames=44, desktopMediaJourneys=2, guiNormalExits=1,
      fullBetaAccepted=False))
write(target/'review.json', dict(productionApplied=True, selectedRepairAccepted=True,
      ordinaryBuildAccepted=True, fullBetaAccepted=False, coreSha256=core,
      sourceFiles=application['files'], sourceInputCount=2400,
      runtime='build-windows/beta-subtitle-delay-repair-20261023/ui-frame-marker-ass.mkv-r4/runtime',
      cause='Subtitle delay updates did not wake the decoder. Precise seek demux/packet '
            'preroll also used video time, discarding cues still active after a positive offset.',
      change='Wake decoder delay consumers, retain earlier delayed cue data while '
             'preserving video/audio display boundaries, and refill seekable paused '
             'subtitles at the actual held frame. Preserve track override, recording, '
             'seekability, current-input and pending-target policies.',
      publicAbiChanged=False, ordinaryNativePasses=35, guiNormalExits=1,
      limits=limits, next='Investigate the spinner accessibility value-write route and '
      'default-delay help, qualify secondary/current timing and other remaining '
      'consumer/workflow cases, then continue the complete thirteen-category/32-surface roadmap.'))
(target/'README.md').write_text('''# Paused subtitle delay repair, 23 October 2026

The ordinary build now retains the expected caption on a delayed paused seek
and refreshes it after a current-file primary delay change or Reset. All eleven
maintained timing cases and 24 prior startup/interval/track/frame-step/resume
regressions pass. All 35 native children exit normally; 121 timing snapshots
verify and 44 actual seek-frame comparisons match the tested private candidate.

One actual ordinary desktop session exercises ASS and mov_text/MP4. Delayed
Finnish cues appear after paused seeking; Reset shows the middle cue without
another user seek. Twelve real keyboard nudges through 50–600 ms restore the
earlier ASS cue while retaining the visible held picture. Opening MP4 in the
same instance reapplies the saved 1.2-second default after the previous file's
Reset. MP4 Reset, resumed caption-bearing playback and direct playing Quit pass;
the session exits zero without checked GUI diagnostics. Saved default remains
`sub-delay=12` (100 ms units).

The production sources match the tested seven-file candidate. Ordinary Core
`7ab80750` is built through the regular build with no private replacement objects
or headers. Current runtime is
`build-windows/beta-subtitle-delay-repair-20261023/ui-frame-marker-ass.mkv-r4/runtime`.
Source copies, compiler dependencies, native images/results and desktop review
are retained. The immutable earlier investigation preserves the failures and
private candidate provenance separately.

This accepts the selected timing repair, not the complete subtitle surface or
the beta. The spinner's direct accessibility value write did not change its
value; keyboard adjustment worked. Exact GUI numeric progress conservation,
complete focus/keyboard policies, secondary timing, broader stream/lifecycle
cases, design/accessibility/DPI, real-media consolidation, independent review
and clean packaging remain open. See `review.json` for the precise limits.
''', encoding='utf-8', newline='\n')
artifacts = {p.relative_to(target).as_posix(): digest(p)
             for p in sorted(target.rglob('*')) if p.is_file()}
write(target/'checkpoint.json', dict(artifacts=artifacts, selectedRepairAccepted=True,
      productionApplied=True, fullBetaAccepted=False, sourceInputCount=2400))
publication = dict(path=target.relative_to(repo).as_posix(), checkpointSha256=digest(target/'checkpoint.json'),
                   artifactCount=len(artifacts), sourceInputCount=2400, ordinaryNativePasses=35,
                   guiNormalExits=1, coreSha256=core, fullBetaAccepted=False,
                   producerSha256=digest(Path(__file__)))
write(work/'publication-r5.json', publication)
print(json.dumps(publication), flush=True)
