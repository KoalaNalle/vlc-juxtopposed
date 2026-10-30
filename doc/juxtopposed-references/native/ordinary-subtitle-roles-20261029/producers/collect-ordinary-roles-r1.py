"""Freeze generated ordinary multi-subtitle observations and exit evidence."""
import hashlib
import json
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[2]
RUN = ROOT / 'build-windows/beta-resume-20261029/gui-r19'
OUT = ROOT / 'doc/juxtopposed-references/native/ordinary-subtitle-roles-20261029'


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    assert not OUT.exists()
    record = json.loads((RUN / 'verification.json').read_text(encoding='utf-8'))
    assert record['status'] == 'terminal' and record['exitCode'] == 0
    assert record['normalExit'] and record['preservationPassed']
    assert not record['forcedCleanupUsed'] and not record['diagnostics']
    assert record['moduleCounts'] == ['603']
    assert all(value for key, value in record.items() if key.endswith('Unchanged'))
    assert all(sha(RUN / 'runtime' / name) == value for name, value in record['runtimeSha256'].items())
    assert all(sha(name) == value for name, value in record['fixtures'].items())
    OUT.mkdir(parents=True)

    def copy(source, relative):
        target = OUT / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
        assert sha(source) == sha(target)

    public = {key: value for key, value in record.items()
              if key in ('status', 'scope', 'exitCode', 'normalExit', 'started', 'ended',
                         'forcedCleanupUsed', 'preservationPassed', 'runtimeSha256',
                         'sourceSha256', 'moduleCounts', 'diagnostics', 'inputs', 'fixtures')
              or key.endswith('Unchanged')}
    (OUT / 'verification.json').write_text(json.dumps(public, indent=2) + '\n', encoding='utf-8')
    for source in sorted((RUN / 'observations').iterdir()):
        assert source.is_file() and source.suffix in ('.json', '.jpg')
        copy(source, Path('observations') / source.name)
    for name in ('frame-marker.mp4', 'subtitle-style.srt'):
        assert (RUN / 'media' / name).is_file()
    copy(RUN / 'media/subtitle-style.srt', Path('subtitle-style.srt'))
    copy(Path(__file__).with_name('run-gui-r19.py'), Path('producers/run-gui-r19.py'))
    copy(Path(__file__), Path('producers') / Path(__file__).name)
    (OUT / 'README.md').write_text('''# Ordinary simultaneous subtitle roles — 29 October 2026

The ordinary Qt7c7 GUI, Core4a, WebVTT38 and TTML9d run together in R19.
The generated marker video has embedded English/Finnish tracks; an external
three-cue SRT is selected as primary. The visible Select multiple command adds
Finnish as secondary. Exact source, runtime and fixture identities are recorded.

Observed through the normal visible controls:

- Both first cues initially render together: upper SUOMI ENSIMMÄINEN and lower
  EXTERNAL FIRST CUE, with playback paused at the generated source's 12 seconds.
  The start-time origin makes the ordinary elapsed display 00:00:00.
- Setting only secondary delay to -9 seconds changes the upper caption to SUOMI
  KESKIMMÄINEN while the external primary stays on its first cue (capture 07).
- Setting primary to -9 independently retains secondary -9; both middle captions
  render (09/10). Resetting primary to 0 preserves the explicit secondary -9
  value (11); subsequently setting secondary to 0 restores both delays (12).
- External subtitle FPS 0-to-50 changes only the external caption to its middle
  cue; embedded Finnish remains on its first cue (13/14).
- Tracks > Subtitle > Synchronize reports 50. Its FPS Reset restores both first
  captions and 0; reopening Playback settings reflects FPS 0 and both delays 0
  (15 through 18). The visible paused frame marker and elapsed display stay fixed.

Capture 08's filename records an attempted +15-second primary entry, not an
accepted setting. The compact control supports -10 through +10 seconds and
correctly restored primary 0 on commit. Its screenshot/text show primary 0 and
secondary -9. Preserve the original filename and interpret the observed values.

The normal Quit action exits the child and observer with code 0. The runtime
loads 603 modules; no captured error diagnostics, forced cleanup or preservation
failures occur. Fixtures, compiler-source inventory, selected runtime, tested
fallback, previous generated profile, helper inputs and human configuration
remain unchanged. Private profiles and raw logs are excluded from this receipt.

This is an ordinary held-frame SRT-primary/embedded-secondary journey on the
identified tuple. It complements the separately scoped existing native client's
eight role/estimator processes; it does not establish all formats, playing and
source/restart combinations, timing estimators, design/DPI or final-package gates.
The screenshots are actual generated-media app observations, not a visual score.
''', encoding='utf-8')
    files = {str(path.relative_to(OUT)).replace('\\', '/'): sha(path)
             for path in sorted(OUT.rglob('*')) if path.is_file()}
    (OUT / 'manifest.json').write_text(json.dumps(files, indent=2) + '\n', encoding='utf-8')
    assert all(sha(OUT / name) == value for name, value in files.items())
    print(json.dumps({'passed': True, 'files': len(files)}))


if __name__ == '__main__':
    main()
