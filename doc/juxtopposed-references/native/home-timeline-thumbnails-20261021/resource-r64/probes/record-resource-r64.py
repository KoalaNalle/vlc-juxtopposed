"""Preserve completed runs without launching or changing the application."""
from pathlib import Path
from hashlib import sha256
import gzip
import json
import shutil
import statistics

repo = Path('C:/Modding/vlc-juxtopposed')
work = Path(__file__).resolve().parent
public = repo / 'doc/juxtopposed-references/native/home-timeline-thumbnails-20261021/resource-r64'

def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))

def digest(path):
    return sha256(path.read_bytes()).hexdigest()

def write(path, value):
    path.write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8')

assert not public.exists(), 'Preserve an existing receipt; never overwrite it.'
renderer = public.parent / 'renderer-r63/checkpoint.json'
sources = read(renderer)['sourceBindings']
inputs_path = work / 'inputs-resource-r64.json'
inputs = read(inputs_path)
assert digest(inputs_path) == digest(work / 'inputs-tuple-r65.json')
for path, expected in sources.items():
    assert inputs[path] == expected == digest(repo / path), path

ordinary = {t: read(work / f'ordinary-resource-{t}-r64/verification.json') for t in ['A1', 'B1']}
native = read(work / 'backend-startup-new-full-r65/verification.json')
for result in [*ordinary.values(), native]:
    assert result['status'] == 'terminal' and result['passed']
    assert result['normalExit'] and result['exitCode'] == 0
    assert result['sourceInputsUnchanged'] and result['identitiesUnchanged']
    assert not result['forcedCleanupUsed']
    assert result['sourceInputsSha256'] == digest(inputs_path)
assert native['counts'] == dict(tests=26, failures=0, errors=0, skipped=0)
assert ordinary['A1']['identities'] == ordinary['B1']['identities']
assert ordinary['A1']['frameLatencyEnvironment'] == '2'
assert ordinary['B1']['frameLatencyEnvironment'] is None
assert not ordinary['A1']['candidateDefaultApplied']
assert ordinary['B1']['candidateDefaultApplied']
for field in ['libvlccore.dll', 'plugins/gui/libqt_plugin.dll', 'plugins/demux/libmp4_plugin.dll']:
    assert native['identities'][field] == ordinary['B1']['identities'][field]

summaries = {}
for trial, result in ordinary.items():
    root = work / f'ordinary-resource-{trial}-r64'
    assert not result['diagnostics']
    order = result['quitLogOrder']
    assert order['wait'] < order['stopped'] < order['teardown']
    assert read(root / 'board-observer-r64.json')['status'] == 'terminal'
    summaries[trial] = {}
    for phase, expected_count in [('playing',25), ('paused',12), ('playing-preview',20), ('paused-preview',120)]:
        metric = read(root / f'{phase}-metrics.json')
        samples = metric['samples']
        assert metric['status'] == 'terminal' and len(samples) == expected_count
        record = result['phases'][phase]
        delta = {k:record['after'][k]-record['before'][k] for k in ['frames displayed','frames late','frames lost','buffers played','buffers lost']}
        assert delta['frames late'] == delta['frames lost'] == delta['buffers lost'] == 0
        fps = delta['frames displayed'] / record['durationSeconds']
        if phase.startswith('paused'):
            assert all(value == 0 for value in delta.values())
            assert record['timeBefore'] == record['timeAfter']
        else:
            assert 48 < fps < 52
        watts = [d['boardPowerW'] for s in samples if s.get('board',{}).get('available') for d in s['board']['devices'] if isinstance(d.get('boardPowerW'), (int,float))]
        summary = dict(samples=len(samples), durationSeconds=record['durationSeconds'], counterDelta=delta,
            displayedFps=fps, meanCpuMachinePercent=statistics.mean(s['cpuMachinePercent'] for s in samples),
            meanPrivateMB=statistics.mean(s['privateBytes']/1e6 for s in samples),
            meanWholeBoardPowerW=statistics.mean(watts),
            scope='Ordinary playback or paused after preview use; continuous card visibility not proven.')
        if phase == 'paused-preview':
            summary['privateMBThirtySecondBlocks'] = [statistics.mean(s['privateBytes']/1e6 for s in samples[i:i+30]) for i in range(0,120,30)]
        summaries[trial][phase] = summary

assert read(work / 'ordinary-resource-A1-r64/measurement-end-inspection.json')['picturePreviewObserved']
assert not read(work / 'ordinary-resource-B1-r64/measurement-end-inspection.json')['picturePreviewVisibleAtMeasurementEnd']
public.mkdir()

def copy(source, relative):
    target = public / relative
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, target)

def compress(source, relative):
    target = public / relative
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(gzip.compress(source.read_bytes(), mtime=0))

for trial in ordinary:
    root = work / f'ordinary-resource-{trial}-r64'
    for name in ['verification.json','initial-inspection.json','preview-inspection.json',
                 'measurement-end-inspection.json','after-preview-inspection.json','cli-observations.txt']:
        copy(root / name, f'ordinary/{trial}-{name}')
    for phase in ['playing','paused','playing-preview','paused-preview']:
        copy(root / f'{phase}-metrics.json', f'ordinary/{trial}-{phase}-metrics.json')
    for name in ['vlc.log','stderr.txt','board-observer-r64.json']:
        compress(root / name, f'ordinary/{trial}-{name}.gz')
root = work / 'backend-startup-new-full-r65'
for name in ['verification.json','results.txt','results.xml','stdout.txt','stderr.txt']:
    copy(root / name, 'native/' + name)
for path in sorted((root / 'runtime').glob('*native.log')):
    compress(path, 'native/' + path.name + '.gz')
for name in ['run-resource-r64.py','observe-board-r64.py','resource_metrics_r45.py',
             'run-tuple-r65.py','inputs-resource-r64.json','inputs-tuple-r65.json','record-resource-r64.py']:
    copy(work / name, 'probes/' + name)
artifacts = {p.relative_to(public).as_posix():digest(p) for p in sorted(public.rglob('*')) if p.is_file()}
write(public / 'checkpoint.json', dict(
    date='2026-10-21', scope='Final candidate tuple: native26 and selected ordinary resource/normal-Quit runs on home Windows11. Preserved after interruption; no new application run.',
    sourceBindings=sources, artifacts=artifacts,
    rendererCheckpointSha256=digest(renderer), identities=ordinary['B1']['identities'],
    ordinaryCases={t:f'ordinary/{t}-verification.json' for t in ordinary},
    nativeCase='native/verification.json', resourceSummary=summaries,
    nativeSelectedFormatsAccepted=True, ordinarySelectedPlaybackPauseResumeQuitAccepted=True,
    resourceMeasurementIntegrityAccepted=True, continuousCardVisibilityAccepted=False,
    comparableColdFootprintAccepted=False, resourceCostAccepted=False, powerAttributionResolved=False,
    frameWaitWorkaroundPromoted=False, backdropBlurAccepted=False, physicalDpiAccepted=False,
    fullFeatureAccepted=False, fullBetaAccepted=False,
    notes=[
        'A1 is explicit frame latency2, B1 leaves it unset and receives the scoped production candidate default0. Same final ordinary binaries/media; no private journey hooks.',
        'A1 initially displayed a hover card during warmup; B1 did not. Both baselines start without a card, but this prevents attributing absolute cold memory differences.',
        'Both native playing intervals deliver approximately50 displayed frames per second, with no newly late/lost frames or lost audio buffers. These aggregate counters are not per-frame pacing proof.',
        'Both 120-second paused intervals preserve primary time/counters. At the end, A1 shows picture41/primary66, while B1 card is absent. Cause of hover change is unknown; continuous picture visibility is not proven.',
        'A1 paused memory30-second means continue434.7,441.4,448.1,454.7MB. B1 means450.9,461.4,461.4,461.4MB plateau after30seconds. Different observed card states prevent a causal memory-growth comparison or general leak-free claim.',
        'Whole-board preview means are46.1/46.3W playing and44.8/48.7W paused for control/candidate. These include other applications/capture; prior97W result stays preserved and unattributed.',
        'Selected observed picture previews, real resumed advancement and normal playing Quit pass in both ordinary processes; both observers terminate and no forced cleanup is used.',
        'Native26 uses the same new ordinary GUI/Core/MP4 tuple. Native warning review remains separate; passing counts do not claim all native logs are warning-free.',
        'No package/push/release promotion. Broader visible-card resource cost, native legacy composition, blur, layout/DPI/accessibility and full beta gates remain open.'
    ]))
for path, expected in artifacts.items():
    assert digest(public / path) == expected, path
write(work / 'resource-r64-summary.json', dict(public=str(public), sources=len(sources), artifacts=len(artifacts), resourceSummary=summaries))
print(f'Saved {len(sources)} source bindings / {len(artifacts)} artifacts; ordinary2 normal exits, native26; resource cost acceptance remains false.')
