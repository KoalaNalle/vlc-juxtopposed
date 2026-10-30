"""Freeze the ordinary storage retry and before/after paused-frame repair."""
import hashlib
import json
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT / 'build-windows/beta-preset-frame-20261029'
GUI = ROOT / 'build-windows/beta-resume-20261029'
OUT = ROOT / 'doc/juxtopposed-references/native/preset-retry-held-frame-20261029'


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def load(path):
    return json.loads(path.read_text(encoding='utf-8'))


def main():
    assert not OUT.exists()
    before = load(WORK / 'before-r3/verification.json')
    after = load(WORK / 'after-r1/verification.json')
    build = load(WORK / 'core-build-r1/verification.json')
    assert before['status'] == after['status'] == 'terminal'
    assert not before['allFramesPreserved'] and after['allFramesPreserved']
    assert len(before['cases']) == len(after['cases']) == 4
    assert build['sharedTreeRestored'] and build['inputsUnchanged'] and build['sourcesUnchanged'] and build['fallbackUnchanged']
    assert all(data[key] for data in (before, after) for key in ('inputsUnchanged', 'runtimeUnchanged', 'fallbackUnchanged'))
    for data in (before, after):
        for row in data['cases']:
            assert row['status'] == 'terminal' and row['exitCode'] == 0
            assert row['userSeekSupersedesRedraw']
            assert row['diagnostics'] == ['main error: window closed']
    lock = load(GUI / 'gui-r25/preset-storage-lock-r1/verification.json')
    stored = load(GUI / 'gui-r25/stored-presets-r1.json')
    assert lock['status'] == 'terminal' and lock['released'] and lock['failedSaveKeptDiskBytes']
    assert len(stored['presets']) == 1 and stored['presets'][0]['name'] == 'Ordinary retry preset'
    assert stored['presets'][0]['delay'] == 1200000
    observations = {}
    for number in (25, 27):
        run = GUI / f'gui-r{number}'
        record = load(run / 'verification.json')
        assert record['status'] == 'terminal' and record['exitCode'] == 0 and record['preservationPassed']
        assert not record['diagnostics'] and record['moduleCounts'] == ['603']
        assert all(value for key, value in record.items() if key.endswith('Unchanged'))
        observations[number] = record
    markers = load(GUI / 'gui-r27/marker-analysis-r2.json')
    assert len(markers['rows']) == 4 and all(row['marker'] == 512 for row in markers['rows'])
    OUT.mkdir(parents=True)

    def copy(source, relative):
        target = OUT / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
        assert sha(source) == sha(target)

    for phase in ('before-r3', 'after-r1'):
        copy(WORK / phase / 'verification.json', Path(phase) / 'verification.json')
        for run in sorted((WORK / phase).iterdir()):
            if run.is_dir() and run.name not in ('runtime', 'cache'):
                for source in sorted(run.glob('*.png')):
                    copy(source, Path(phase) / run.name / source.name)
    copy(WORK / 'before-r1/setup-failure.json', Path('initial-runner-setup-failure.json'))
    for name in ('verification.json', 'before.json', 'command.json', 'build.log', 'exit.txt'):
        copy(WORK / 'core-build-r1' / name, Path('core-build') / name)
    for source in sorted((WORK / 'core-build-r1/actual-dependencies').rglob('*.Plo')):
        copy(source, Path('core-build/actual-dependencies') / source.relative_to(WORK / 'core-build-r1/actual-dependencies'))
    for name in ('verification.json', 'inputs.json', 'compile.command.json', 'compile.log', 'link.command.json', 'link.log', 'subtitle_fps.c'):
        copy(WORK / 'test-build-r2' / name, Path('test-build') / name)
    for name in ('src/player/input.c', 'src/player/player.h', 'src/player/timer.c'):
        copy(WORK / 'preimage-r1' / name, Path('preimage') / name)
        copy(ROOT / name, Path('source') / name)
    for number, record in observations.items():
        public = {key: value for key, value in record.items()
                  if key in ('status', 'scope', 'exitCode', 'normalExit', 'forcedCleanupUsed',
                             'preservationPassed', 'runtimeSha256', 'sourceSha256', 'inputs',
                             'fixtures', 'moduleCounts', 'diagnostics', 'command') or key.endswith('Unchanged')}
        target = OUT / f'gui-r{number}'
        target.mkdir()
        (target / 'verification.json').write_text(json.dumps(public, indent=2) + '\n', encoding='utf-8')
        for source in sorted((GUI / f'gui-r{number}/observations').iterdir()):
            assert source.is_file() and source.suffix in ('.json', '.jpg')
            copy(source, Path(f'gui-r{number}/observations') / source.name)
        copy(GUI / f'gui-r{number}/marker-analysis-r2.json', Path(f'gui-r{number}/marker-analysis.json'))
    copy(GUI / 'gui-r25/preset-storage-lock-r1/verification.json', Path('storage-lock.json'))
    copy(GUI / 'gui-r25/stored-presets-r1.json', Path('stored-preset.json'))
    for name in ('run-gui-r25.py', 'run-gui-r27.py', 'lock-ordinary-presets-r1.py',
                 'build-preset-frame-core-r1.py', 'build-preset-frame-test-r2.py',
                 'run-preset-frame-before-r3.py', 'run-preset-frame-after-r1.py'):
        copy(GUI / name, Path('producers') / name)
    copy(Path(__file__), Path('producers') / Path(__file__).name)
    (OUT / 'README.md').write_text('''# Preset retry and exact held-picture repair — 29 October 2026

Ordinary R25 on Core4a/Qt7c7 demonstrates a real Windows INI replacement failure.
The generated profile is held with read/write sharing but no delete sharing.
Save Preset reports a useful error, preserves the name/form and exact disk hash,
and does not invent a selected saved entry. Releasing that handle and clicking
the same Save button succeeds, clears the error and stores exactly one named
preset with 1,200,000-us timing. A new reader verifies its durable record.

Applying Default then exposed a separate paused-picture defect. The generated
MP4 starts at 20.5 s and initially holds source frame 512 (20.48 s), just before
the visible start origin. Direct delay editing and failed/successful storage
keep frame 512; Default's batched timing reset advances it to 513 while the
elapsed display remains zero. Stored capture analysis decodes the actual drawn
marker independently of that elapsed display. Caption changes themselves work.

The maintained Core consumer test adds startup-single/startup-batch cases,
using real media, decoders and vout PNG snapshots. Before R3 on Core4a reproduces
512→513 for the batched 20.5-s case; the single reset and frame-aligned controls
preserve their own initial frame. Startup's retained frame can vary with callback
ordering, so each oracle compares its actual initial picture, not a forced frame
number. A later user seek changes the frame and a subsequent redraw retains that
new target. All four child processes exit 0; the observer records the intended
held-frame failure. Initial R1 omitted test_init's plugin-path environment and
failed before playback; that setup failure is retained and is unqualified.

The repair caches the exact media-relative target while its own paused redraw
seek is pending. The visible seek origin may remain zero for preroll. Explicit
time/position/chapter seeks invalidate the cached target centrally; resume,
stop, recording and new-input lifetimes also clear it. The field is appended to
the private input structure. The normal two-worker Core build covers 21 private
header consumers and 359 actual dependency paths, with its full shared-src tree,
inputs, source and tested fallback restored/unchanged. Core candidate is ccb6a2.

The unchanged extended native test executable passes all four after cases:
held-frame preservation, fractional/aligned starts and later user-seek precedence.
Its exact test source, compiler inputs, commands and PNGs are included. The sole
native diagnostic per child is the known test-window-close teardown message;
it is classified separately from ordinary GUI diagnostics.

Ordinary R27 uses Coreccb6 with Qt7c7/WebVTT38/TTML9d. The saved preset survives
a fresh process and is manually applied to the newly copied generated media.
It renders the first Finnish cue at live delay 1.200 s; Default restores the
middle cue and delay 0.000 s. Both actions preserve drawn frame 512 and paused
elapsed zero. This proves explicit saved-preset application, not automatic
file-association matching across changed fixture paths. R25 and R27 each exit
child/observer 0, load 603 modules, emit no captured error diagnostics, and pass
all source/runtime/fixture/fallback/profile/input/human-config guards.

Caption refresh is asynchronous: R25 capture 02 is the transitional old caption;
capture 03 records the settled first cue. Capture filenames are not assertions.
No private profiles, media binaries or raw personal logs are included. These
named storage/native/ordinary scopes do not close all formats, role/estimator,
recording, visual/DPI, multi-instance or final-package gates. New-Core regression
results must be recorded separately rather than inherited from Core4a receipts.
''', encoding='utf-8')
    files = {str(path.relative_to(OUT)).replace('\\', '/'): sha(path)
             for path in sorted(OUT.rglob('*')) if path.is_file()}
    (OUT / 'manifest.json').write_text(json.dumps(files, indent=2) + '\n', encoding='utf-8')
    assert all(sha(OUT / name) == value for name, value in files.items())
    print(json.dumps({'passed': True, 'files': len(files)}))


if __name__ == '__main__':
    main()
