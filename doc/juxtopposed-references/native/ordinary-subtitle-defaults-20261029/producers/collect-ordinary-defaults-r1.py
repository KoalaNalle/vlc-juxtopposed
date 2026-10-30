"""Preserve the ordinary saved-default/cold-restart subtitle journey."""
import hashlib
import json
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT / 'build-windows/beta-resume-20261029'
OUT = ROOT / 'doc/juxtopposed-references/native/ordinary-subtitle-defaults-20261029'


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    assert not OUT.exists()
    rows = {}
    for number in range(20, 25):
        run = WORK / f'gui-r{number}'
        record = json.loads((run / 'verification.json').read_text(encoding='utf-8'))
        assert record['status'] == 'terminal' and record['exitCode'] == 0
        assert record['normalExit'] and record['preservationPassed']
        assert not record['forcedCleanupUsed'] and not record['diagnostics']
        assert record['moduleCounts'] == ['603']
        assert all(value for key, value in record.items() if key.endswith('Unchanged'))
        assert all(sha(run / 'runtime' / name) == value for name, value in record['runtimeSha256'].items())
        rows[number] = record
    first = rows[20]['runtimeSha256']
    assert all(record['runtimeSha256'] == first for record in rows.values())
    OUT.mkdir(parents=True)

    def copy(source, relative):
        target = OUT / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
        assert sha(source) == sha(target)

    for number, record in rows.items():
        run = WORK / f'gui-r{number}'
        public = {key: value for key, value in record.items()
                  if key in ('status', 'scope', 'exitCode', 'normalExit', 'started', 'ended',
                             'forcedCleanupUsed', 'preservationPassed', 'runtimeSha256',
                             'sourceSha256', 'moduleCounts', 'diagnostics', 'inputs', 'fixtures', 'command')
                  or key.endswith('Unchanged')}
        target = OUT / f'gui-r{number}'
        target.mkdir()
        (target / 'verification.json').write_text(json.dumps(public, indent=2) + '\n', encoding='utf-8')
        for source in sorted((run / 'observations').iterdir()):
            assert source.is_file() and source.suffix in ('.json', '.jpg')
            copy(source, Path(f'gui-r{number}/observations') / source.name)
        config = (run / 'vlcrc').read_text(encoding='utf-8')
        selected = [line for line in config.splitlines()
                    if any(line.startswith(key + '=') for key in ('sub-language', 'sub-delay', 'spu'))]
        (target / 'saved-default-lines-on-exit.json').write_text(json.dumps(selected, indent=2) + '\n', encoding='utf-8')
        copy(WORK / f'run-gui-r{number}.py', Path(f'producers/run-gui-r{number}.py'))
    copy(Path(__file__), Path('producers') / Path(__file__).name)
    (OUT / 'README.md').write_text('''# Ordinary subtitle defaults and cold restart — 29 October 2026

Five ordinary VLC processes use the same Core4a/Qt7c7/WebVTT38/TTML9d runtime.
Generated profiles are copied forward only after the preceding child/observer
has exited and its preservation checks pass. No personal media is used.

R20 starts paused with an English default explicitly seeded in its generated
configuration. Saving Finnish through Global Settings > Subtitles preserves the
current English caption and held video. The saved native key is sub-language=fin.
R21 cold-starts with that configuration and automatically renders Finnish; the
settings dropdown also retains Finnish. Saving Enable subtitles Off preserves
the current Finnish caption and displays the restart notice.

R22 cold-starts with captions disabled, at the same generated source position as
R21. Global Settings retains Off. Saving On shows the restart notice and leaves
the current input disabled. The ordinary saved-delay slider writes 1,200 ms,
represented by native sub-delay=12 (tenths of a second).

R23 cold-starts with subtitles enabled and the 1,200 ms delay. At source 20.5 s
it renders SUOMI ENSIMMÄINEN, while R21 with zero delay rendered SUOMI
KESKIMMÄINEN. Both show the same blue video and marker, with visible elapsed
00:00:00 because of the start-time origin. Playback settings reports 1.200 s.
Global Settings retains 1,200 ms. Saving its default back to zero does not show
a restart notice, change the current cue or change the live 1.200 s delay.

R24 cold-starts with the saved zero default, renders SUOMI KESKIMMÄINEN again at
the same source/frame marker, and reports live delay 0.000 s. No timing edits
or manual track selection are needed at these cold starts. The unavailable
secondary-delay control remains disabled with only one selected subtitle.

Each child and observer exits normally with code 0, loads 603 modules and has
no captured error diagnostics or forced cleanup. All fixture, source, runtime,
fallback, previous generated profile, helper-input and human-config guards pass.
The exact commands and hashes are recorded. Only generated target-window
captures, selected nonprivate settings lines and scoped reports are published;
private profiles, media binaries and raw logs remain ignored locally.

This qualifies ordinary embedded-track language selection and cold default
ownership, including an actual cue/held-frame comparison for delay. A cold
process is not proof of every same-player Next/Previous or source-reuse path;
external automatic-language fallback, saved presets and all format/role
combinations remain separate. It does not close beta/design/DPI/package gates.
''', encoding='utf-8')
    files = {str(path.relative_to(OUT)).replace('\\', '/'): sha(path)
             for path in sorted(OUT.rglob('*')) if path.is_file()}
    (OUT / 'manifest.json').write_text(json.dumps(files, indent=2) + '\n', encoding='utf-8')
    assert all(sha(OUT / name) == value for name, value in files.items())
    print(json.dumps({'passed': True, 'processes': len(rows), 'files': len(files)}))


if __name__ == '__main__':
    main()
