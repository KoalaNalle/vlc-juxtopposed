"""Save parameter parsing controls, native matrix and ordinary generated-media facts."""
import hashlib
import json
from pathlib import Path
import shutil

ROOT=Path(__file__).resolve().parents[2]
WORK=ROOT/'build-windows/beta-ttml-fps-20261029'
GUI=ROOT/'build-windows/beta-resume-20261029/gui-r18'
OUT=ROOT/'doc/juxtopposed-references/native/ttml-time-parameters-20261029'


def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def load(p):return json.loads(Path(p).read_text(encoding='utf-8-sig'))


def main():
    assert not OUT.exists()
    accepted=load(WORK/'after-r5/verification.json')
    before=load(WORK/'unit-before-parameters-r1/verification.json')
    unit=load(WORK/'unit-after-parameters-r1/verification.json')
    gui=load(GUI/'verification.json')
    assert accepted['passed'] and accepted['preservedR4MatrixCases']==45
    assert before['exitCode']==1 and before['results'][-1]['failures']==47
    assert unit['passed'] and unit['results'][-1]['checks']==538 and unit['results'][-1]['failures']==0
    assert gui['status']=='terminal' and gui['exitCode']==0 and gui['preservationPassed'] and not gui['diagnostics']
    OUT.mkdir(parents=True)
    def copy(source,target):
        dest=OUT/target;dest.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(source,dest);assert sha(source)==sha(dest)
    for name in ['parameters-before-r1','after-r4','after-r5']:
        copy(WORK/name/'verification.json',Path(name)/'verification.json')
        for case in sorted((WORK/name).iterdir()):
            if case.is_dir() and (case/'stdout.txt').exists():
                for p in sorted(case.iterdir()):
                    if p.suffix=='.png' or p.name in ['stdout.txt','stderr.txt']:
                        copy(p,Path(name)/case.name/p.name)
    for name in ['unit-build-r4','unit-build-r5','unit-before-parameters-r1','unit-after-parameters-r1','plugin-build-r4']:
        for p in sorted((WORK/name).iterdir()):
            if p.is_file() and (p.suffix in ['.json','.log','.d'] or p.name in ['stdout.txt','stderr.txt']):
                copy(p,Path(name)/p.name)
    for p in sorted((WORK/'media-r3').iterdir()):
        if p.suffix in ['.ttml','.json']:copy(p,Path('fixtures')/p.name)
    for name in ['modules/codec/ttml/ttml.c','modules/codec/ttml/ttml.h','test/modules/demux/ttml_fps.c']:
        copy(ROOT/name,Path('source')/name)
        previous=WORK/'parameters-preimage-r1'/name
        if previous.exists():copy(previous,Path('preimage')/name)
    for name in ['probe-ttml-parameters-r1.py','prepare-ttml-r3.py','build-ttml-unit-r4.py',
                 'build-ttml-unit-r5.py','run-ttml-unit-before-parameters-r1.py',
                 'run-ttml-unit-after-parameters-r1.py','build-ttml-plugin-r4.py','run-ttml-after-r4.py',
                 'qualify-ttml-parameters-r5.py','run-gui-r18.py','run-inline-webvtt-smoke-r1.py',Path(__file__).name]:
        copy(Path(__file__).with_name(name),Path('producers')/name)
    for p in sorted((GUI/'observations').iterdir()):
        if p.suffix in ['.json','.jpg','.png']:copy(p,Path('ordinary')/p.name)
    public={k:v for k,v in gui.items() if k in ['status','scope','runtimeSha256','sourceSha256','fixtures',
            'exitCode','normalExit','forcedCleanupUsed','fixturesUnchanged','sourcesUnchanged','runtimeUnchanged',
            'fallbackBytesUnchanged','priorProfileUnchanged','inputsUnchanged','humanConfigUnchanged',
            'moduleCounts','diagnostics','preservationPassed']}
    (OUT/'ordinary/verification.json').write_text(json.dumps(public,indent=2)+'\n')
    (OUT/'README.md').write_text('''# TTML document time parameters — 29 October 2026

This receipt qualifies media-time frame/tick expressions on the identified
source/native/ordinary tuple. The beta remains unfinished. Wall-clock
synchronization, SMPTE drop-frame behavior, all document profiles, embedded
containers/secondary roles, recording and final-package coverage are separate.

## Repair and independent before controls

The source parser previously assumed 30 FPS, ignored frameRateMultiplier and
subFrameRate, and had no ticks branch. The five-process `parameters-before-r1`
control shows a declared-25-FPS caption too early and a tick-based caption missing
against independently authored clock references. The first six source cases
already passed; the expanded tests then reproduced 47 failures in declared-unit
and malformed-input rows, with child exit 1. That expected negative is preserved.

Parsed nodes now inherit the root's timing context. The XML reader's namespace
identity selects significant root parameters, independent of prefix/attribute
order; similarly named unrelated attributes and child declarations do not replace
them. Frame, multiplier, subframe and tick values are normalized to media ticks
before resolving parent-relative timings. The tick defaults follow whether the
root explicitly supplied a frame rate. Complete clock/offset syntax and positive
integer parameters are checked, including conversion range. A failed node is
discarded before it is linked into its parent, so rejected child parsing can
clean up the partial document normally.

See the [W3C TTML2 parameter definitions](https://www.w3.org/TR/ttml2/#parameter-attribute-vocabulary)
and [media-time expression rules](https://www.w3.org/TR/ttml2/#time-expression-semantics-media).
The preceding interval/parent-frame carry repair and its scope are preserved in
the [earlier receipt](../ttml-fps-nested-clock-20261029/README.md).

## Source and ordinary build

The unchanged expanded test source passes eight cases/538 checks after the parser
repair, versus 47 intended before failures. It checks declared rates, NTSC-style
multipliers, subframes, explicit/derived/default ticks, fractional units,
namespace aliases, root-only scope, microsecond precision, large hours, malformed
parameters/expressions and the previous interval/rate/Reset/cleanup regressions.
Actual XML reader, production parser/demux/serializer and real Core objects are
used. Unused decoder callbacks are failing stubs; rendering is a separate result.
The direct source build has 288 guarded inputs and no compiler/link diagnostics.

The regular two-worker TTML plugin build produces
`9d74556528c6e44edd91a9b919f886f099f4e501696d7ed0aa4461edaffe0b58`.
All 271 actual compiler dependencies are qualified. The frozen inventory also
binds the source-test receipt. Original shared plugin/object/dependency files
were restored, fallback and inputs unchanged, no forced cleanup, lock released.
The shared TTML DLL remains original `ddf83a39a6bd`; staging explicitly selects
the candidate. Full identities are in the recorded inputs and verifications.

## Native rendering and corrected frame-boundary calibration

`after-r4` contains 45 normally completed decoder processes, with every actual
colour-mask comparison equal to its independent reference (Jaccard 1.0), held
markers unchanged, clock/state/delay checks passing and no unexpected errors.
It covers the prior 22 cases plus declared 25 FPS before/after its start,
frameRateMultiplier, subframes, explicit/derived/default ticks, inherited FPS,
held FPS changes and Reset. Positive references contain actual caption pixels.
The dummy-output window-close teardown diagnostic is disclosed separately.

The R4 observer nevertheless exited 1: it incorrectly required two nominal
pre-boundary samples to be empty after the 25-FPS video seek rounded them forward.
Requested 10.005 seconds became 10.040001; requested 10.21 became 10.240001.
Both actual clocks are after the intended cue starts (10.01 and 10.22), and both
subject and independent reference correctly contained BETA. R4 remains unchanged.

`after-r5` retains and validates the full R4 matrix, then runs four corrected
subject/reference pre-boundary probes. Their actual clocks are checked below
each boundary, while R4's nonempty positive samples are checked above it. All
checks pass, preserving actual frame granularity rather than claiming nominal
seek positions are exact. The full source test independently checks parsed
microsecond values. All 49 rendering processes exited 0; the combined R5
qualification passes with stable inputs/runtime. The parser source is explicitly
matched to the ordinary build's frozen source inventory.

## Ordinary GUI R18

Core `4a2865915e34`, Qt `7c7db5cb675e`, WebVTT `38eec724ba22`, TTML `9d74556528c6`;
other runtime identities are recorded. Avcodec `545f715418c4` does not inherit
the separately qualified codec26 duration behavior.

With default D3D11 output, a declared-100-ticks-per-second caption renders BETA
while paused at source second 12 (visible elapsed zero). Playback settings >
Subtitles > 50 FPS makes it disappear because its retimed end is before the held
position. Tracks > Subtitle > Synchronize shows 50; Reset restores BETA and 0.
Playback settings then reflects 0.000 FPS. Marker and visible clock remain fixed.
These are visible observations, distinct from native exact-frame assertions.

Normal Ctrl+Q closes the owned app: child/observer exit 0, 603 modules, no captured
error diagnostics, all preservation guards pass. Prior generated profile,
personal settings and fallback were preserved. No dependency was installed or
security/privacy setting changed. Raw personal logs and profile contents are
omitted. Tests and references use generated media only.

Frozen producers document exact executions; adapt fresh output names and current
inventories before reuse. The marker video remains ignored locally with its
recorded identity/generator in the linked earlier receipts. Executables, test
profiles and private media do not belong in a release.
''',encoding='utf-8')
    manifest={str(p.relative_to(OUT)).replace('\\','/'):sha(p) for p in sorted(OUT.rglob('*')) if p.is_file()}
    (OUT/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    assert all(sha(OUT/p)==h for p,h in manifest.items())
    print(json.dumps(dict(passed=True,files=len(manifest),out=str(OUT))))


if __name__=='__main__':main()
