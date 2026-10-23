from pathlib import Path
from hashlib import sha256
import json, shutil, subprocess

work = Path(__file__).resolve().parent
repo = work.parent.parent
target = repo/'doc/juxtopposed-references/native/home-paused-subtitle-delay-investigation-20261023'
read = lambda p: json.loads(p.read_text(encoding='utf-8-sig'))
digest = lambda p: sha256(p.read_bytes()).hexdigest()


def write(p, value):
    p.write_text(json.dumps(value, indent=2)+'\n', encoding='utf-8', newline='\n')


def copy(p, destination):
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(p, destination)
    assert digest(p) == digest(destination)


assert not target.exists()
inputs = read(work/'inputs-boundary-r6.json')
assert len(inputs['files']) == inputs['sourceCount'] == 2393
for name, h in inputs['files'].items():
    assert digest(repo/name) == h and digest(Path(inputs['sourceSnapshot'])/name) == h
candidate = read(work/'candidate-r4.json')
assert not candidate['productionApplied']
assert all(digest(repo/p) == h for p, h in candidate['preimages'].items())
assert all(digest(work/'candidate-r4'/p) == h for p, h in candidate['files'].items())
assert (work/'build-candidate-r4-exit.txt').read_text().strip() == '0'
core = digest(work/'candidate-r4-bin/libvlccore.dll')
assert core == '19a5438da4dcd74f3c20655ec8342ccaee5df6deb22073c3cd882c76ee7477cb'
paired = read(work/'paired-frames-r4.json')
assert paired['allPairedSeekFramesMatch'] and paired['allCandidateChecksPass']
position_before = {r['case']: r for r in read(work/'native-boundary-before-r6/processes.json')['records']}
position_pairs = []
for record in read(work/'native-boundary-candidate-r6/processes.json')['records']:
    old_rows = {r['step']: r for r in position_before[record['case']]['result']['observations']}
    new_rows = {r['step']: r for r in record['result']['observations']}
    for name in ['reference-first', 'reference-middle', 'boundary-seek', 'boundary-settled']:
        assert old_rows[name]['frameMarker'] == new_rows[name]['frameMarker']
        position_pairs.append(dict(case=record['case'], step=name,
                                   before=old_rows[name]['frameMarker'], candidate=new_rows[name]['frameMarker']))
assert len(position_pairs) == 12
phases = [('native-delay-ass-wasapi-r3', 2, False),
          ('native-delay-formats-r3', 6, False),
          ('native-delay-before-ass-wasapi-r4', 2, False),
          ('native-delay-candidate-ass-wasapi-r4', 2, True),
          ('native-delay-candidate-formats-r4', 6, True),
          ('regressions-candidate-r5', 24, True),
          ('native-boundary-before-r6', 3, False),
          ('native-boundary-candidate-r6', 3, True)]
rows = []
timing_snapshots = 0
for folder, count, expected in phases:
    summary = read(work/folder/'processes.json')
    assert len(summary['records']) == count and summary['allProcessesNormal']
    for record in summary['records']:
        assert record['status'] == 'terminal' and record['normalExit'] and record['exitCode'] == 0
        assert record['sourceInputsUnchanged'] and record['runtimeUnchanged'] and record['fixturesUnchanged']
        assert not record['forcedCleanupUsed'] and record['passed'] == expected
        result = record['result']
        if folder != 'regressions-candidate-r5':
            assert result['status'] == 'terminal' and result['teardownComplete'] and result['stopReached']
            assert result['checks']['executionComplete'] and result['checks']['audioBackend']
            out = work/folder/record['case']/'probe'
            for observation in result['observations']:
                assert observation['snapshotAvailable']
                assert digest(out/observation['snapshot']) == observation['snapshotSha256']
                timing_snapshots += 1
        rows.append(dict(phase=folder, family=record.get('family', 'delay'),
                         case=record['case'], pid=record['pid'], passed=record['passed'],
                         privateCore=record['privateCore'], checks=result.get('checks'),
                         diagnostics=result.get('diagnostics', [])))
assert len(rows) == 48 and sum(r['passed'] for r in rows) == 35 and timing_snapshots == 264
gui = read(work/'ui-observations-r2.json')
assert len(gui['cases']) == 2 and not gui['cases'][0]['wholeJourneyPassed']
assert gui['cases'][1]['reopenedDefaultSubtitleTrackEditor'] == '0'
assert gui['cases'][1]['reopenedPreferredLanguageEditor'] == 'Finnish'
assert gui['cases'][1]['reopenedSubtitleDelaySliderMs'] == 1200
for phase in ['initial-r2', 'reopen-r2']:
    record = read(work/phase/'verification.json')
    assert record['status'] == 'terminal' and record['normalExit'] and record['exitCode'] == 0
    assert not record['diagnostics'] and not record['forcedCleanupUsed']
    assert all(record[p] for p in ['sourceInputsUnchanged', 'runtimeUnchanged', 'fixturesUnchanged'])
    assert all(digest(work/'runtime'/p) == h for p, h in record['runtimeSha256'].items())
target.mkdir()
write(target/'position-frame-comparison.json', dict(rows=position_pairs, passed=True,
      scope='Twelve matched actual position-route seek pictures; no absolute PTS claim.'))
for p in work.iterdir():
    if p.is_file() and p.suffix in ['.py', '.sh', '.json', '.txt', '.log', '.patch', '.mk']:
        copy(p, target/p.name)
for folder in ['initial', 'initial-r2', 'reopen-r2', 'worker-preimage-r6', 'candidate-r4',
               'candidate-r4-bin']:
    for p in (work/folder).rglob('*'):
        if p.is_file() and p.suffix in ['.py', '.md', '.c', '.h', '.d', '.json', '.txt', '.log']:
            copy(p, target/p.relative_to(work))
for folder, count, expected in phases:
    root = work/folder
    copy(root/'processes.json', target/folder/'processes.json')
    for out in root.iterdir():
        if not out.is_dir() or out.name == 'runtime':
            continue
        for p in out.iterdir():
            if p.is_file() and p.suffix in ['.py', '.json', '.txt', '.log']:
                copy(p, target/p.relative_to(work))
        for p in (out/'probe').rglob('*'):
            if p.is_file() and p.suffix in ['.json', '.txt', '.log', '.png']:
                copy(p, target/p.relative_to(work))
for name in candidate['preimages']:
    copy(repo/name, target/'sources/production-preimage'/name)
for name in ['test/Makefile.am', 'test/libvlc/native_paused_subtitle_delay.py',
             'test/libvlc/native_paused_subtitle_delay.md']:
    copy(repo/name, target/'sources/maintained'/name)
write(target/'comparison.json', dict(records=rows, nativeNormalExits=48,
      baselineFailures=13, targetedPrivatePasses=11, legacyPrivatePasses=24,
      timingSnapshots=timing_snapshots, guiNormalExits=2, productionApplied=False,
      fullBetaAccepted=False))
limits = [
    'Seven production Core sources/headers are unchanged. Core19a5438d is a private candidate; ordinary rebuild and actual desktop timing acceptance are still required.',
    'Two ordinary GUI journeys qualify saved default values, current-versus-next-media behavior and cold explicit-index priority; the initial timing journey fails and is preserved.',
    'The first strict hash comparison misclassified one SubRip and three mov_text glyph-edge pixels in correct playing controls. Raw results and images remain unchanged; later workers allow at most 0.5 percent differing caption-union pixels and keep mask hashes.',
    'No absolute requested PTS, physical audibility, whole subtitle workflow, screen-reader/OS-DPI or full beta acceptance.',
    'Per-track/secondary timing, multiple selected subtitle delays, unknown-length/nonseekable/recording behavior, large offsets, nonzero time origins and more cue edges remain to be qualified.',
    'Native dummy-window diagnostics and compiler warnings are preserved; no error-free-native-log claim.',
    'Windows10 comparison, design/consumer gaps, independent review and final clean-package gates remain in the full thirteen-category/32-surface roadmap.'
]
write(target/'review.json', dict(productionApplied=False, selectedRepairAccepted=False,
      privateCandidatePassed=True, privateCoreSha256=core, sourceInputCount=2393,
      guiDefaultPersistencePassed=True, guiTimingWorkflowPassed=False,
      nativeNormalExits=48, targetedPrivatePasses=11, legacyPrivatePasses=24,
      pairedTimeSeekFrames=32, pairedPositionSeekFrames=12, limits=limits, fullBetaAccepted=False,
      next='Review/apply the seven-source candidate, build ordinary Core and qualify '
           'these same timing/regression journeys and actual current-app synchronization '
           'controls. Continue all remaining beta work; do not ship the private candidate.'))
(target/'README.md').write_text('''# Paused subtitle delay investigation, 23 October 2026

The ordinary app saves and reopens the selected Finnish language, default
subtitle index zero and +1.2-second default delay. Changing defaults preserves
current playback; the next file uses the saved language/delay or explicit index.
Two actual desktop sessions close normally without checked GUI diagnostics.

The same app loses a delayed cue after seeking while paused, and changing the
live delay does not refresh it. Native controls reproduce both failures across
SubRip, ASS and mov_text, with WASAPI and in a no-audio case. Playback across
the boundary behaves correctly. The decoder can sleep through a delay update;
demux/packet preroll also uses video time without retaining earlier delayed cues.

A private seven-source candidate wakes delay consumers, obtains earlier cue
data while retaining the requested display boundary and refills seekable paused
subtitles at the actual held video timestamp. All eleven targeted candidate
cases pass, including the progress-bar position route and a negative offset;
all 24 prior startup/interval/track/frame-resume regressions pass. All 44 paired
seek-frame comparisons (32 time-route and 12 position-route) match the unchanged build. The 48 native processes
close normally; 13 before cases retain their behavioral failures. There are
264 timing snapshots, in addition to the legacy regression evidence.

This is an investigation checkpoint. Production Core sources remain unchanged;
the next gate is an ordinary rebuild plus the actual desktop timing workflow.
The complete thirteen-category/32-surface beta remains active. See `review.json`
for limits, `comparison.json` for per-case checks and the preserved raw logs,
source preimages, candidate patch and compiler dependency records for provenance.
''', encoding='utf-8', newline='\n')
artifacts = {p.relative_to(target).as_posix(): digest(p)
             for p in sorted(target.rglob('*')) if p.is_file()}
write(target/'checkpoint.json', dict(artifacts=artifacts, fullBetaAccepted=False,
      productionApplied=False, privateCandidatePassed=True, sourceInputCount=2393))
publication = dict(path=target.relative_to(repo).as_posix(),
                   checkpointSha256=digest(target/'checkpoint.json'), artifactCount=len(artifacts),
                   nativeNormalExits=48, guiNormalExits=2, productionApplied=False,
                   fullBetaAccepted=False, producerSha256=digest(Path(__file__)))
write(work/'publication-r7.json', publication)
print(json.dumps(publication), flush=True)
