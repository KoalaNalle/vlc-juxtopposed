# Native integration and live buffering - 25 October 2026 night

The continuing session now has a broader integration checkpoint. Beta readiness,
full design and final portable-package acceptance remain open.

Core 6ffceeb2 / GUI 5fda5ed4 passes 24 startup/subtitle/track cases, eleven detailed
timing cases and thirteen bare-player checks. Nine FLAC drain cases preserve every
source-rate sample, including exact/repeated 88,200 and block boundaries. The
resampled case also decodes 88,200 frames. Twelve native conversion-controller
cases cover real output playback, inherited-option isolation, encoder selection,
cancel/reuse, destination races, worker teardown and primary-player independence.

The maintained live time-shift test serves its own paced loopback raw PCM through
stock HTTP and raw-audio modules. Both 1 MiB and 2 MiB settings create and roll over
at least three bounded disk chunks while the paused clock and callbacks remain
frozen. The source keeps sending, buffered sample markers resume without jumping
to live data, and native files plus server threads are cleaned up normally.

The stricter desired test fails the old Core on a valid optional capability query
logged as unknown. Core e1767d37 adds DEMUX_CAN_CONTROL_RATE to the helper's
existing unsupported group, preserving its failure result. Both final live cases
then pass without native errors; all fourteen native subtitle functions also pass.
One generated-stream input-PCR clock correction remains recorded before Pause in
each case. No warning during buffered resume is accepted.

[checkpoint.json](checkpoint.json) records full runtime identities, frozen source
copies, original reports/log bytes, before failures and fixture corrections. The
earlier WAV prototype is not an accepted stream-format test. Broader cases retain
their own Core identity; three original-media dummy startup buffering diagnostics
remain explicit. Desktop picker, other live protocols, physical devices, visual
and final-package gates are separate. Continue from [STATUS](../../../juxtopposed/STATUS.md).
