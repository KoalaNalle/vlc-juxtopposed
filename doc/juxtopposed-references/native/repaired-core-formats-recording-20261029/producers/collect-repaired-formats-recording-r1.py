"""Preserve affected new-Core rendered-format and recording-state evidence."""
import hashlib
import json
from pathlib import Path
import re
import shutil

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT / 'build-windows/beta-preset-frame-20261029'
PRODUCERS = ROOT / 'build-windows/beta-resume-20261029'
OUT = ROOT / 'doc/juxtopposed-references/native/repaired-core-formats-recording-20261029'


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    assert not OUT.exists()
    formats = json.loads((WORK / 'formats-r1/verification.json').read_text())
    recording = json.loads((WORK / 'recording-r1/verification.json').read_text())
    assert formats['status'] == recording['status'] == 'terminal'
    assert formats['passed'] and len(formats['cases']) == 12 and len(formats['comparisons']) == 8
    assert all(formats[key] for key in ('inputsUnchanged', 'runtimeUnchanged', 'fallbackUnchanged'))
    assert all(value == 1. for row in formats['comparisons'] for step in row['similarity'].values() for value in step.values())
    assert recording['exitCode'] == 0 and recording['normalTestResult'] and recording['inputsUnchanged'] and recording['runtimeUnchanged']
    assert 'RESULT cases=7 failures=0' in recording['stdout']
    errors = [line for line in (WORK / 'recording-r1/stderr.txt').read_text(errors='replace').splitlines()
              if re.search(r'\berror:|Assertion failed|QFATAL|QWARN', line)]
    assert len(errors) == 7 and all(line.endswith('main window error: window closed') for line in errors)
    OUT.mkdir(parents=True)

    def copy(source, relative):
        target = OUT / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
        assert sha(source) == sha(target)

    for phase in ('formats-r1', 'recording-r1'):
        copy(WORK / phase / 'verification.json', Path(phase) / 'verification.json')
    for run in sorted((WORK / 'formats-r1').iterdir()):
        if run.is_dir() and run.name not in ('runtime', 'cache'):
            for source in sorted(run.glob('*.png')):
                copy(source, Path('formats-r1') / run.name / source.name)
    for name in formats['inputs']:
        source = Path(name)
        if source.suffix in ('.vtt', '.ttml'):
            family = 'webvtt' if source.suffix == '.vtt' else 'ttml'
            copy(source, Path('references') / family / source.name)
    copy(WORK / 'recording-r1/stdout.txt', Path('recording-r1/stdout.txt'))
    copy(ROOT / 'test/src/player/recording.c', Path('recording.c'))
    for name in ('run-repaired-format-timing-r1.py', 'run-recording-preset-core-r1.py', 'run-inline-webvtt-smoke-r1.py'):
        copy(PRODUCERS / name, Path('producers') / name)
    copy(Path(__file__), Path('producers') / Path(__file__).name)
    (OUT / 'diagnostics.json').write_text(json.dumps({'recordingKnownWindowClose': 7,
        'recordingOtherErrors': [], 'formats': [{'family': row['family'], 'name': row['name'],
        'knownWindowClose': row['teardownWindowCloseDiagnostics'], 'otherErrors': row['otherErrors']}
        for row in formats['cases']]}, indent=2) + '\n')
    (OUT / 'README.md').write_text('''# Repaired-Core formats and recording — 29 October 2026

Actual Coreccb6/Qt7c7/WebVTT38/TTML9d modules are staged from qualified ordinary
R27. Twelve fresh native processes cover two generated formats: inline WebVTT
with past/current/future colours and declared tickRate=100 TTML. Each has fresh
normal and explicitly retimed reference renders, held FPS 0→50→0, inherited 50
and Reset, a positive two-second category delay, and a first playing FPS change.
All eight comparison groups match their reference colour masks exactly (Jaccard
1.0). Paused markers/clocks, native delay/state and advancing playing clocks pass.
Exact frames, commands, fixtures, client source/binary and runtime hashes are
recorded. All inputs/runtime/fallback guards pass and every child exits 0.

The maintained actual recording client also passes seven cases on Coreccb6:
manual Off while paused, natural EOS, same/other next media, paused replacement,
paused Stop, and Off-before-EOS without duplicate notifications. This qualifies
record-state notification/lifetime behavior; it does not establish output tails,
codec completeness, conversion, actual FFmpeg error propagation or ordinary UI.
Its child/observer exits 0 and input/runtime guards pass.

Known native test-window-close teardown diagnostics are classified separately;
there are seven in the recording process and no other captured errors. Native
format cases likewise retain their known teardown counts and no other errors.
The current Core client's new startup frame modes and ordinary hardware proof
are in the preset-retry-held-frame receipt. This affected sweep does not repeat
or inherit every older TTML parsing/profile, fractional FPS, source/role, design,
DPI, Windows 10 or final-package gate. Media/output binaries, private profiles
and raw logs stay ignored; only generated renders and scoped reports are here.
''', encoding='utf-8')
    files = {str(path.relative_to(OUT)).replace('\\', '/'): sha(path)
             for path in sorted(OUT.rglob('*')) if path.is_file()}
    (OUT / 'manifest.json').write_text(json.dumps(files, indent=2) + '\n')
    assert all(sha(OUT / name) == value for name, value in files.items())
    print(json.dumps({'passed': True, 'formatProcesses': 12, 'recordingCases': 7, 'files': len(files)}))


if __name__ == '__main__':
    main()
