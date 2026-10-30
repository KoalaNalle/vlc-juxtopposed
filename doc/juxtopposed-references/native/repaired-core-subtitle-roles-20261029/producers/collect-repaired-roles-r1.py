"""Record qualified new-Core roles and estimator rows; retain the setup failure."""
import hashlib
import json
from pathlib import Path
import re
import shutil

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT / 'build-windows/beta-subtitle-roles-20261029'
PRODUCERS = ROOT / 'build-windows/beta-resume-20261029'
OUT = ROOT / 'doc/juxtopposed-references/native/repaired-core-subtitle-roles-20261029'


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    assert not OUT.exists()
    r2 = json.loads((WORK / 'current-r2/verification.json').read_text())
    r3 = json.loads((WORK / 'current-r3/verification.json').read_text())
    assert r2['status'] == r3['status'] == 'terminal'
    assert not r2['passed'] and len(r2['cases']) == 9
    assert all(row['passed'] and row['exitCode'] == 0 for row in r2['cases'][:8])
    failure = r2['cases'][8]
    assert failure['selector'] == 'estimatorAppliesMediaTime' and failure['exitCode'] == 24
    assert 'Supply frozen production synchronization QML pages' in (WORK / 'current-r2/case-8/result.txt').read_text()
    assert r3['passed'] and [row['dataRows'] for row in r3['cases']] == [24, 16]
    assert r2['runtimeSha256'] == r3['runtimeSha256']
    assert all(data[key] for data in (r2, r3) for key in ('inputsUnchanged', 'runtimeUnchanged', 'fallbackUnchanged'))
    diagnostics = []
    for phase, count in (('current-r2', 8), ('current-r3', 2)):
        for index in range(count):
            lines = (WORK / phase / f'case-{index}/stderr.txt').read_text(encoding='utf-8', errors='replace').splitlines()
            errors = [line for line in lines if re.search(r'\berror:|Assertion failed|QFATAL|QWARN|Binding loop|ReferenceError|TypeError', line)]
            assert len(errors) == 1 and errors[0].endswith('main window error: window closed')
            diagnostics.append({'phase': phase, 'case': index, 'teardownWindowClosed': 1, 'otherErrors': []})
    OUT.mkdir(parents=True)

    def copy(source, relative):
        target = OUT / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
        assert sha(source) == sha(target)

    for phase, count in (('current-r2', 9), ('current-r3', 2)):
        copy(WORK / phase / 'verification.json', Path(phase) / 'verification.json')
        for index in range(count):
            for name in ('result.xml', 'result.txt'):
                copy(WORK / phase / f'case-{index}' / name, Path(phase) / f'case-{index}' / name)
            for source in sorted((WORK / phase / f'case-{index}/snapshots').glob('*.png')):
                copy(source, Path(phase) / f'case-{index}/snapshots' / source.name)
    for source in sorted((WORK / 'current-r3/frozen-pages').iterdir()):
        copy(source, Path('frozen-pages') / source.name)
    for name in ('run-current-subtitle-roles-r2.py', 'run-current-subtitle-roles-r3.py'):
        copy(PRODUCERS / name, Path('producers') / name)
    copy(Path(__file__), Path('producers') / Path(__file__).name)
    copy(ROOT / 'modules/gui/qt/tests/test_player_subtitle_delay.cpp', Path('test_player_subtitle_delay.cpp'))
    (OUT / 'diagnostics.json').write_text(json.dumps(diagnostics, indent=2) + '\n')
    (OUT / 'README.md').write_text('''# Repaired-Core subtitle roles and estimators — 29 October 2026

Ten qualified selected processes use Coreccb6/current SRT/ASS/WebVTT38/TTML9d.
R2's first eight processes pass category-relative adjustment/Reset callbacks,
own/inherited large secondary delays and resume, held/playing FPS preservation
of embedded secondary captions, opposing per-track delay retiming, estimator
directions and cancellation without publishing a late adjustment.

R2's ninth process failed all 24 rows at the initial requirement for production
QML pages: the runner omitted VLC_TRACKS_SOURCE_DIR. That failed result is
preserved; it does not qualify estimator speed behavior or a product failure.
The context-cancellation selector was not reached in R2.

Fresh R3 supplies two frozen production TracksPage QML files and full two-track
initialization rather than the FPS-only fresh setup. The same identified native
client passes all 24 estimator rows (audio/primary/secondary, 0.5/1/1.5/2 rates,
both timing directions) and all 16 context-cancellation rows (Qt/native seeks,
rate changes, pause/resume, subtitle/audio selection changes). Exact data-row
counts and executed functions are checked; none is skipped. The handlers and
connection code are extracted from the frozen production pages and executed
against the actual compiled controller/native player, not a whole ordinary UI.

All ten qualified children/observers exit 0 and all source/input/runtime/fallback
guards pass. Each emits one known native test-window-close teardown diagnostic,
with no other captured errors. Raw logs, test binary and profiles stay ignored.

Executable 64846efd9fab is the previously identified compiled regression client
with exact test source 80838e7b8198. It owns its compiled controller; these are
new Coreccb6 interaction results, not a fresh full controller-source rebuild or
ordinary page/design/DPI/package acceptance. Ordinary Qt7c7 preset/held-picture
proof is recorded separately in the preset-retry-held-frame receipt. Preserve
the earlier Core4a evidence and R2 setup failure as distinct scopes.
''', encoding='utf-8')
    files = {str(path.relative_to(OUT)).replace('\\', '/'): sha(path)
             for path in sorted(OUT.rglob('*')) if path.is_file()}
    (OUT / 'manifest.json').write_text(json.dumps(files, indent=2) + '\n')
    assert all(sha(OUT / name) == value for name, value in files.items())
    print(json.dumps({'passed': True, 'qualifiedProcesses': 10, 'speedRows': 24, 'cancellationRows': 16, 'files': len(files)}))


if __name__ == '__main__':
    main()
