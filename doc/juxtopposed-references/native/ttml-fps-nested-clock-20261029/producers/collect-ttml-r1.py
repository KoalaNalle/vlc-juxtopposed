"""Publish generated-only evidence for TTML rate, nested intervals and frame carry."""
import hashlib
import json
from pathlib import Path
import shutil

ROOT=Path(__file__).resolve().parents[2]
WORK=ROOT/'build-windows/beta-ttml-fps-20261029'
GUI=ROOT/'build-windows/beta-resume-20261029/gui-r17'
OUT=ROOT/'doc/juxtopposed-references/native/ttml-fps-nested-clock-20261029'


def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def load(p):return json.loads(Path(p).read_text(encoding='utf-8-sig'))


def main():
    assert not OUT.exists()
    native=load(WORK/'after-r3/verification.json')
    unit=load(WORK/'unit-run-r2/verification.json')
    gui=load(GUI/'verification.json')
    assert native['passed'] and native['mixedClockMatchesReference'] and unit['passed']
    assert gui['status']=='terminal' and gui['exitCode']==0 and gui['preservationPassed'] and not gui['diagnostics']
    assert load(WORK/'mixed-clock-before-r1/verification.json')['qualifiedFailure']
    OUT.mkdir(parents=True)
    def copy(source,target):
        dest=OUT/target;dest.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(source,dest);assert sha(source)==sha(dest)
    for name in ['before-r1','after-r3']:
        copy(WORK/name/'verification.json',Path(name)/'verification.json')
        for case in sorted((WORK/name).iterdir()):
            if case.is_dir() and (case/'stdout.txt').exists():
                for p in sorted(case.iterdir()):
                    if p.suffix=='.png' or p.name in ['stdout.txt','stderr.txt']:
                        copy(p,Path(name)/case.name/p.name)
    for p in sorted((WORK/'mixed-clock-before-r1').iterdir()):
        if p.suffix in ['.json','.png'] or p.name in ['stdout.txt','stderr.txt']:
            copy(p,Path('mixed-clock-before-r1')/p.name)
    for name in ['plugin-build-r1','plugin-build-r2','plugin-build-r3','unit-build-r2','unit-build-r3','unit-run-r1','unit-run-r2','after-r2']:
        for p in sorted((WORK/name).iterdir()):
            if p.is_file() and (p.name in ['verification.json','inputs.json','actual-dependencies.json','stdout.txt','stderr.txt']
                                or p.suffix in ['.log','.d']):
                copy(p,Path(name)/p.name)
    for p in sorted((WORK/'media-r2').iterdir()):
        if p.suffix in ['.ttml','.json']:
            copy(p,Path('fixtures')/p.name)
    for name in ['modules/demux/ttml.c','modules/codec/ttml/genttml.c','modules/codec/ttml/ttml.h',
                 'test/modules/demux/ttml_fps.c','test/src/player/subtitle_fps.c']:
        copy(ROOT/name,Path('source')/name)
        before=WORK/'preimage-r1'/name
        if before.exists():copy(before,Path('preimage')/name)
    for name in ['prepare-ttml-r1.py','prepare-ttml-r2.py','run-ttml-before-r1.py','run-ttml-after-r2.py',
                 'run-ttml-after-r3.py','run-ttml-mixed-clock-r1.py','build-ttml-plugin-r1.py','build-ttml-plugin-r2.py',
                 'build-ttml-plugin-r3.py','build-ttml-unit-r2.py','build-ttml-unit-r3.py',
                 'run-ttml-unit-r1.py','run-ttml-unit-r2.py','run-gui-r17.py',Path(__file__).name,
                 'run-inline-webvtt-smoke-r1.py']:
        copy(Path(__file__).with_name(name),Path('producers')/name)
    for p in sorted((GUI/'observations').iterdir()):
        if p.suffix in ['.json','.jpg','.png']:copy(p,Path('ordinary')/p.name)
    public={k:v for k,v in gui.items() if k in ['status','scope','runtimeSha256','sourceSha256','fixtures',
            'exitCode','normalExit','forcedCleanupUsed','fixturesUnchanged','sourcesUnchanged','runtimeUnchanged',
            'fallbackBytesUnchanged','priorProfileUnchanged','inputsUnchanged','humanConfigUnchanged',
            'moduleCounts','diagnostics','preservationPassed']}
    (OUT/'ordinary/verification.json').write_text(json.dumps(public,indent=2)+'\n')
    (OUT/'README.md').write_text('''# TTML FPS, nested output and mixed clock/frame timing — 29 October 2026

This receipt qualifies the identified source/native/ordinary scopes. All beta
gates remain open. It does not establish every TTML profile, declared frame/tick
parameter, secondary-role combination, recording boundary, physical DPI or package.

## Problems and changes

The original TTML demuxer did not expose rate control. A 50-FPS getter left its
rendered cue unchanged. Its serializer also repeated resolved absolute parent
and child times, so reparsing added parent offsets again and hid nested captions.
Generated parallel and sequential documents failed against a nonempty flat cue.

The demuxer now retains its parsed tree/change points in source time and scales
emitted timestamps. Rate changes seek the containing interval, mark discontinuity
and permit refill after subtitle EOF. Invalid rates and overflowing transforms
are rejected before state changes. Time/position and slave deadlines use the same
presentation timeline. The filtered subtree is constant until the next change:
its presentation interval is emitted once on body, while descendant timing and
sequential-container attributes are removed. Encoder serialization keeps its prior
entry point and behavior.

A separate default-30-FPS mixed-clock fixture exposed double-counting of remaining
frames during parent addition. Only whole frame-seconds are now carried into the
base; remaining frames are added once by Convert. A uint64 intermediate preserves
the sum before carry. This does not add support for arbitrary declared frame rates.
Nested timing expectations follow the [W3C TTML2 timing-container rules](https://www.w3.org/TR/ttml2/#timing-attribute-timeContainer).

## Build and native evidence

The normal two-worker plugin build produced `e980c858d139` (full SHA256 in the
verification), with 271 actual compiler dependencies inside the frozen inventory.
All owned shared plugin/object/dependency artifacts were restored; fallback and
protected inputs stayed unchanged, lock released and no forced cleanup used.
R1 compiled/linked but its check rejected equivalent absolute paths containing
`..`; that failed result and restoration are preserved. R2 fixed canonical path
comparison and passed. R3 rebuilt after the independently reproduced frame fix.

The unchanged maintained player snapshot driver calls real Core APIs and the
actual decoder. `before-r1` qualifies flat and explicit-retimed references, then
reproduces rate and nested failures. `mixed-clock-before-r1` uses the intermediate
plugin `bbc20e97a6ca`: at source 10.5 seconds the mixed-offset caption is absent,
while the independently timed BETA reference has 618 magenta pixels.

The final `after-r3` matrix has 22 successful native processes: held 50/29.97/
23.976 FPS, inherited FPS, a positive two-second delay, first change during
playback, Reset after retimed EOF, nested parallel/sequential captions and mixed
fractional-second/frame offsets. All compared colour masks match references with
Jaccard similarity 1.0. Held frame markers match exactly, time spreads stay under
50 ms, and playing clocks advance. The empty mixed-caption reference is supported
by a nonempty positive reference for the same source. Only the disclosed dummy-
output window-close teardown diagnostic appears; other-error and input/runtime
guards pass. These cases use generated media only.

The maintained `test/modules/demux/ttml_fps.c` directly compiles the production
demux, parser and serializer. Its final six cases/215 checks pass: containing-cue
seek, immutable index/rates/Reset, invalid controls leaving state unchanged, EOF
revival/ending position, actual XML parser roundtrip and mixed-frame carry.
The source build has 288 guarded inputs. Unused descriptor callbacks are deliberate
failing stubs; this source test makes no decoder claim. Actual rendering is covered
separately by the native plugin matrix. The later opt-in Makefile registration is
distinct from the qualified direct build. No toolchain dependencies were installed.

## Ordinary GUI R17

Core `4a2865915e34`, Qt `7c7db5cb675e`, WebVTT `38eec724ba22`, TTML `e980c858d139`;
other exact runtime identities are in the ordinary verification. The selected
avcodec `545f715418c4` does not inherit the independent codec26 duration result.

Using default D3D11 video output, source second 8 (visible elapsed zero) initially
shows ALPHA in green. Playback settings > Subtitles > 50 FPS changes it to magenta
BETA while the video remains paused at the same marker/clock. Tracks > Subtitle >
Synchronize shows 50. Its Reset restores green ALPHA and 0; reopening playback
settings reflects 0.000 FPS. The immediate post-click accessibility snapshot was
stale at 50; the fresh settled capture verifies 0. Both observations are preserved.
These visible facts are separate from exact native frame/time assertions.

Normal Ctrl+Q closed the owned app: child/observer exit 0, 603 modules, no captured
error diagnostics and all preservation guards passed. Personal settings and prior
generated profile stayed unchanged. Raw personal logs/profile contents are omitted.

## Reproduction

Frozen producers preserve the specific run and must be adapted to fresh outputs
and revalidated inputs. The opt-in source test takes the maintained fixture folder;
the Core driver takes video, subtitle, empty output, FPS, mode, source seconds and
category delay in microseconds. The generated black-bottom marker video is kept
ignored locally; its identity and generator are in the linked inline WebVTT receipt.
No executable, private media or test profile belongs in the ordinary package.
''',encoding='utf-8')
    files={str(p.relative_to(OUT)).replace('\\','/'):sha(p) for p in sorted(OUT.rglob('*')) if p.is_file()}
    (OUT/'manifest.json').write_text(json.dumps(files,indent=2)+'\n')
    assert all(sha(OUT/p)==h for p,h in files.items())
    print(json.dumps(dict(passed=True,files=len(files),out=str(OUT))))


if __name__=='__main__':main()
