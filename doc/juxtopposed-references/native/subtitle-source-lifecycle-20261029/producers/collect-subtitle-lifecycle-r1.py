"""Preserve scoped native restoration and ordinary same-player language policy."""
import hashlib
import json
from pathlib import Path
import re
import shutil

ROOT = Path(__file__).resolve().parents[2]
RUN = ROOT / 'build-windows/beta-subtitle-roles-20261029/lifecycle-r1'
GUI = ROOT / 'build-windows/beta-resume-20261029/gui-r28'
OUT = ROOT / 'doc/juxtopposed-references/native/subtitle-source-lifecycle-20261029'
PRODUCERS = ROOT / 'build-windows/beta-resume-20261029'


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    assert not OUT.exists()
    native = json.loads((RUN / 'verification.json').read_text())
    ordinary = json.loads((GUI / 'verification.json').read_text())
    assert native['status'] == ordinary['status'] == 'terminal'
    assert native['passed'] and [row['dataRows'] for row in native['cases']] == [4, 6, 2]
    assert all(native[key] for key in ('inputsUnchanged', 'runtimeUnchanged', 'fallbackUnchanged'))
    assert ordinary['exitCode'] == 0 and ordinary['preservationPassed'] and not ordinary['diagnostics']
    assert all(value for key, value in ordinary.items() if key.endswith('Unchanged'))
    assert ordinary['moduleCounts'] == ['603']
    assert 'sub-language=fin' in (GUI / 'vlcrc').read_text()
    diagnostics = []
    for index in range(3):
        lines = (RUN / f'case-{index}/stderr.txt').read_text(errors='replace').splitlines()
        errors = [line for line in lines if re.search(r'\berror:|Assertion failed|QFATAL|QWARN|ReferenceError|TypeError', line)]
        assert len(errors) == 1 and errors[0].endswith('main window error: window closed')
        diagnostics.append({'case': index, 'teardownWindowClosed': 1, 'otherErrors': []})
    OUT.mkdir(parents=True)

    def copy(source, relative):
        target = OUT / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
        assert sha(source) == sha(target)

    copy(RUN / 'verification.json', Path('native/verification.json'))
    for index in range(3):
        for name in ('result.xml', 'result.txt'):
            copy(RUN / f'case-{index}' / name, Path(f'native/case-{index}') / name)
        for source in sorted((RUN / f'case-{index}/snapshots').glob('*.png')):
            copy(source, Path(f'native/case-{index}/snapshots') / source.name)
    public = {key: value for key, value in ordinary.items()
              if key in ('status', 'scope', 'exitCode', 'normalExit', 'forcedCleanupUsed',
                         'preservationPassed', 'runtimeSha256', 'sourceSha256', 'inputs',
                         'fixtures', 'moduleCounts', 'diagnostics', 'command') or key.endswith('Unchanged')}
    (OUT / 'ordinary').mkdir()
    (OUT / 'ordinary/verification.json').write_text(json.dumps(public, indent=2) + '\n')
    for source in sorted((GUI / 'observations').iterdir()):
        assert source.is_file() and source.suffix in ('.json', '.jpg')
        copy(source, Path('ordinary/observations') / source.name)
    for name in ('run-current-subtitle-lifecycle-r1.py', 'run-gui-r28.py'):
        copy(PRODUCERS / name, Path('producers') / name)
    copy(Path(__file__), Path('producers') / Path(__file__).name)
    copy(ROOT / 'modules/gui/qt/tests/test_player_subtitle_delay.cpp', Path('test_player_subtitle_delay.cpp'))
    (OUT / 'diagnostics.json').write_text(json.dumps(diagnostics, indent=2) + '\n')
    (OUT / 'README.md').write_text('''# Subtitle source changes and language ownership — 29 October 2026

Three selected native processes on Coreccb6/current codecs pass twelve data rows:
four A→B→A attachment cases (fresh/reused input objects, with/without preferred
English), six attachment/preset callback-order and before-cue/FPS Reset cases,
and two cold-origin callback-order cases. These check actual saved association
reselection plus decoded caption presence, preset/timing state and held-picture
assertions in the identified compiled client. They do not establish every cue's
content, every language fallback, unavailable source or every attachment scope.

Ordinary R28 on Coreccb6/Qt7c7 starts a generated two-item playlist with an English
automatic subtitle preference. Saving Finnish through Global Settings preserves
the active English caption and paused clock, with no restart notice. Next opens
the second item and renders Finnish automatically; Previous returns to the first
item and also renders Finnish. Both inputs retain their explicitly requested
paused startup at source 12 seconds, with elapsed zero. No manual track selection
or process restart is used between these operations. Filename overlays in
captures 04/06 are transitional; settled captions are visible in 05/07.

All native/ordinary children and observers exit 0, with input/runtime/fallback
guards unchanged. The ordinary app loads 603 modules with no captured errors;
each native process has one known test-window-close teardown diagnostic and no
other captured errors. The native client retains the explicit source-closure
limitation: executable 64846efd9fab owns its compiled controller, with exact
test source 80838e7b8198. R28 is actual ordinary GUI evidence on the named tuple.

Warnings from metadata/probing/generated audio remain distinct from the error
classification and from audio-content acceptance; see the separate diagnostic
review note. No PCM/tail/physical endpoint or final-package claim is made. Media,
private profiles and raw logs remain ignored. The full beta scope remains open.
''', encoding='utf-8')
    files = {str(path.relative_to(OUT)).replace('\\', '/'): sha(path)
             for path in sorted(OUT.rglob('*')) if path.is_file()}
    (OUT / 'manifest.json').write_text(json.dumps(files, indent=2) + '\n')
    assert all(sha(OUT / name) == value for name, value in files.items())
    print(json.dumps({'passed': True, 'nativeRows': 12, 'ordinaryProcesses': 1, 'files': len(files)}))


if __name__ == '__main__':
    main()
