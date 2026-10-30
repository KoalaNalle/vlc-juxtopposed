"""Save generated-media evidence; no personal profile or raw desktop log export."""
import hashlib
import json
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT / 'build-windows/beta-inline-webvtt-20261029'
GUI = ROOT / 'build-windows/beta-resume-20261029/gui-r16'
OUT = ROOT / 'doc/juxtopposed-references/native/inline-webvtt-fps-20261029'


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def load(p):
    return json.loads(Path(p).read_text(encoding='utf-8-sig'))


def main():
    assert not OUT.exists()
    matrix = load(WORK / 'renderer-r2/verification.json')
    gui = load(GUI / 'verification.json')
    assert matrix['passed'] and matrix['negativeControl']['qualified']
    assert matrix['inputsUnchanged'] and matrix['runtimeUnchanged'] and matrix['baselineUnchanged']
    assert gui['status'] == 'terminal' and gui['exitCode'] == 0
    assert gui['normalExit'] and gui['preservationPassed'] and not gui['diagnostics']
    OUT.mkdir(parents=True)
    copied = []

    def copy(source, target):
        dest = OUT / target
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, dest)
        assert sha(source) == sha(dest)
        copied.append(dict(path=str(target).replace('\\', '/'), sha256=sha(dest)))

    for directory in ['renderer-r1', 'renderer-r2']:
        copy(WORK / directory / 'verification.json', Path(directory) / 'verification.json')
        for case in sorted((WORK / directory).iterdir()):
            if case.is_dir() and (case / 'stdout.txt').exists():
                for p in sorted(case.iterdir()):
                    if p.suffix == '.png' or p.name in ['stdout.txt', 'stderr.txt']:
                        copy(p, Path(directory) / case.name / p.name)
    for p in sorted((WORK / 'media-r1').glob('*.vtt')):
        copy(p, Path('fixtures') / p.name)
    copy(WORK / 'media-r1/verification.json', Path('fixtures/verification.json'))
    for name in ['verification.json', 'dependencies.d', 'actual.d', 'inputs.json',
                 'dependencies.command.json', 'compile.command.json', 'link.command.json',
                 'dependencies.exit.txt', 'compile.exit.txt', 'link.exit.txt', 'compile.log', 'link.log']:
        copy(WORK / 'test-build-r1' / name, Path('test-build') / name)
    copy(ROOT / 'test/src/player/subtitle_fps.c', Path('source/subtitle_fps.c'))
    for name in ['build-inline-subtitle-test-r1.py', 'prepare-inline-webvtt-r1.py',
                 'run-inline-webvtt-smoke-r1.py', 'run-inline-webvtt-matrix-r2.py', 'run-gui-r16.py',
                 Path(__file__).name]:
        copy(Path(__file__).with_name(name), Path('producers') / name)
    for p in sorted((GUI / 'observations').iterdir()):
        if p.suffix in ['.json', '.png', '.jpg']:
            copy(p, Path('ordinary') / p.name)
    public = {k: v for k, v in gui.items() if k in [
        'status', 'scope', 'runtimeSha256', 'sourceSha256', 'fixtures', 'exitCode', 'normalExit',
        'forcedCleanupUsed', 'fixturesUnchanged', 'sourcesUnchanged', 'runtimeUnchanged',
        'fallbackBytesUnchanged', 'priorProfileUnchanged', 'inputsUnchanged',
        'humanConfigUnchanged', 'moduleCounts', 'diagnostics', 'preservationPassed']}
    (OUT / 'ordinary/verification.json').write_text(json.dumps(public, indent=2) + '\n')
    (OUT / 'README.md').write_text('''# Inline WebVTT subtitle FPS — 29 October 2026

This receipt qualifies the recorded Core/native-renderer cases and one ordinary
Windows 11 GUI journey. It does not close the beta, every WebVTT feature, TTML,
secondary tracks, recording boundaries, OS-DPI/design acceptance or packaging.

## Identities and native observations

Core `4a2865915e34`, Qt `7c7db5cb675e`, WebVTT `38eec724ba22`; exact identities
are in the verifications. The maintained `test/src/player/subtitle_fps.c` calls
the actual player FPS/delay/seek/pause APIs and snapshots the actual renderer.
Its direct cross build passed with 109 compiler dependencies, no compiler output
and stable inputs; executable SHA256 is recorded in `test-build/verification.json`.
It is separately registered as the opt-in `test_src_player_subtitle_fps` target.
That later Makefile registration does not alter the previously built executable.

The generated 60-second marker video has a black lower half. The VTT cue contains
10/20-second inline timestamps and lime `:past` / magenta `:future` styles. The
independent reference files use rational 25/FPS timestamp conversion and normal
FPS; their fixture producer and input video identity are preserved. No personal
media is included. Video bytes remain ignored in the local fixture directory.

The R2 matrix has fifteen new native processes and three calibrated R1 cases.
Held 50, 29.97, 23.976 FPS, two transition locations, a positive two-second delay,
inherited 50 FPS, Reset and a fresh first change during playback passed. Every
candidate colour-mask comparison has Jaccard similarity 1.0. Held frame markers
are identical; time spread is below 50 ms. Playing clocks increase normally.

The original WebVTT DLL `e6d599aabf0e` is a qualified negative control: with the
same Core and other runtime modules, its initial/Reset images match normal timing
exactly but its 50-FPS image fails the intended retimed colour comparison.
All native children exited 0 and completed; only the disclosed dummy-output
window-close teardown diagnostic appears. Input and both runtime guards passed.

## Preserved calibration failure

The frozen R1 observer exited 1 because it incorrectly required past-green text
before any word had completed. Its three native children succeeded. R2 corrects
that expectation without rewriting R1. Current text can match neither pseudo-
class and stays white; future text is magenta and completed text is green.
See the [W3C WebVTT pseudo-class definition](https://www.w3.org/TR/2026/CRD-webvtt1-20260520/#the-past-and-future-pseudo-classes).
The controls ensure nonempty contrasting reference phases before accepting matches.

## Ordinary GUI R16

Using default D3D11 video output, paused at source second 8 (visible clock zero),
Playback settings > Subtitles > Subtitle frame rate 0 to 50 changed ALPHA from
white to green and BETA from magenta to white; GAMMA stayed magenta. The held
marker and visible clock did not move. Tracks > Subtitle > Synchronize displayed
50 FPS; its Reset returned the original colours and 0 FPS. Reopening playback
settings reflected 0.000 FPS. Captures and accessibility exports are in `ordinary`.
These are observed UI facts, separate from the native exact-frame assertions.

Normal Ctrl+Q closed the owned app: child/observer exit 0, 603 modules, no captured
error diagnostics and all preservation guards passed. Personal configuration,
prior generated profile and fallback were preserved. The ordinary verification
omits raw personal logs and profile contents.

## Reproduction scope

Frozen producers document the exact run and are not commands to rerun unchanged.
Adapt fresh output paths and revalidate their inventories. The native driver's
arguments are video, external subtitle, empty output folder, FPS, mode
(`reference`, `hold`, `inherited`, `playing`), target seconds and category delay
in microseconds. Use a matching staged Core/plugins and isolated environment as
in the runner. Generated reference masks, not the API getter alone, establish
rendered timing. The executable and test profiles do not belong in a release.
''', encoding='utf-8')
    generated = [p for p in OUT.rglob('*') if p.is_file()]
    manifest = {str(p.relative_to(OUT)).replace('\\', '/'): sha(p) for p in sorted(generated)}
    (OUT / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    assert all(sha(OUT / p) == h for p, h in manifest.items())
    print(json.dumps(dict(passed=True, copied=len(copied), files=len(manifest), out=str(OUT))))


if __name__ == '__main__':
    main()
