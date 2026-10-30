from pathlib import Path
from hashlib import sha256
import json
import shutil
from PySide6.QtGui import QImage

root = Path(__file__).resolve().parent
repo = root.parents[1]
digest = lambda p: sha256(p.read_bytes()).hexdigest()
rows = []
for label in ('ui_ass_record_av2', 'ui_ass_replay_av2'):
    for path in sorted((root / label / 'observations').glob('*.png')):
        img = QImage(str(path))
        reason = ('Playback menu covers marker bits' if label == 'ui_ass_record_av2'
                  and path.name.startswith(('00-', '01-', '04-')) else
                  'Unsettled window geometry' if img.width() != 1920 or img.height() != 1032 else None)
        row = dict(run=label, file=path.name, sha256=digest(path),
                   markerValid=reason is None, excludedReason=reason)
        if reason is None:
            marker = sum(1 << bit for bit in range(11)
                         if img.pixelColor(round(125 + 1668 / 960 * (32 + 64 * bit)), 75).red() > 128)
            row.update(sourceFrameMarker=marker, sourceSeconds=marker / 25)
        rows.append(row)
analysis = dict(producerSha256=digest(Path(__file__)), observations=rows,
    scope='Marker decoding only in unobstructed settled video geometry. Caption visibility is recorded separately from visual inspection. Earlier v1 analysis included occluded markers and is superseded.')
(root / 'ordinary-recording-observations-v2.json').write_text(json.dumps(analysis, indent=2) + '\n')
out = repo / 'doc/juxtopposed-references/native/home-subtitle-refill-20261028/ordinary-recording-followup'
out.mkdir(exist_ok=False)
runs = {}
for label in ('ui_ass_record_av2', 'ui_ass_replay_av2'):
    path = root / label / 'verification.json'
    record = json.loads(path.read_text())
    assert record['status'] == 'terminal' and record['exitCode'] == 0
    assert all(record[k] for k in ('sourceUnchanged', 'runtimeUnchanged', 'packageUnchanged',
                                  'fixturesUnchanged', 'humanConfigUnchanged', 'producerUnchanged'))
    keys = ('status', 'exitCode', 'normalExit', 'sourceSnapshotSha256', 'sourceCommit',
            'sourcePatchSha256', 'producerSha256', 'runtimeSha256', 'mediaSha256',
            'moduleCounts', 'diagnostics', 'sourceUnchanged', 'runtimeUnchanged',
            'packageUnchanged', 'fixturesUnchanged', 'humanConfigUnchanged', 'producerUnchanged')
    runs[label] = {k: record[k] for k in keys if k in record}
    runs[label]['verificationSha256'] = digest(path)
    runs[label]['launcherExitCode'] = 1 if label == 'ui_ass_record_av2' else 0
    for name in ('ordinary-recording-decode.json', 'ordinary-ass-subtitle-check.json'):
        report = root / label / name
        if report.exists():
            runs[label][name] = json.loads(report.read_text())
evidence = dict(runs=runs, analysis=analysis,
    diagnosticReview=dict(recording='Rejected candidate muxers precede the selected Matroska writer, which accepts all three streams and writes its header. The final window-closed message accompanies normal title-bar closure. Strict launcher exit 1 is retained.',
                          replay='No checked diagnostics; normal process and launcher exit 0.'),
    observedPlayback=dict(initialFrame=250, selection='Embedded Track 1 manually enabled; no external subtitle argument',
        pausedMiddleFrame=250, pausedLastFrame=591, playingExpiredFrame=1004),
    openIssues=['Playback Record menu does not expose active recording state.',
                'No claim of AV continuity across the earlier FPS edit, exact recording boundaries or perceptual sync.',
                'The FLAC source regression selected MP4; a forced non-ASS libavformat mux regression remains pending.'])
(out / 'evidence.json').write_text(json.dumps(evidence, indent=2) + '\n')
producers = ['run-ui-ass-record-av2.py', 'run-ui-ass-replay-av2.py',
             'verify-ui-recording-av2.py', 'verify-ui-ass-recording-av2.py', Path(__file__).name]
for name in producers:
    shutil.copyfile(root / name, out / name)
(out / 'producer-hashes.json').write_text(json.dumps({name:digest(out/name) for name in producers}, indent=2)+'\n')
print(json.dumps(dict(evidence=str(out), capturedRuns=list(runs), validMarkers=sum(r['markerValid'] for r in rows))))
