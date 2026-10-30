# TTML document time parameters — 29 October 2026

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
