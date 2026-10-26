# Fragmented MP4 seeking - 26 October 2026 nightly checkpoint

MP4 `df15e8ed` with Core `ccf72399` and GUI `4d754f60` starts decoding at the
resolved fragment and includes earlier packets for selected long subtitle cues.
The retained before case has corrupt pictures and clock drift; the intermediate
repair fixes video/time, and the final repair also restores the covering cue.

All twelve generated WASAPI seek/delay/Reset/resume cases pass, including four
rates, later cues, either/both/disabled captions and four other MP4 layouts. The
checked-in public-API regression helper also passes its separate dual-caption run.
The first dual-caption probe had an incorrect image-band assumption; later probes
use explicit placement and preserve that fixture correction.

The ordinary app qualifies middle-cue seek, +5s/Reset and playback to LAST. Three
native exports are pixel-identical with marker 799. Candidate and prior-plugin
comparison exit 0 with 602 modules, zero error diagnostics and unchanged guards.

Initial paused app captures still lack a caption before seeking, both before and
after this repair. The cause, native cold-export outcome and physical display
remain unqualified. Long/sparse-cue seek cost, other index/streaming layouts and
broader beta gates also remain open. This is a source nightly checkpoint, not a
beta or release-package declaration. [checkpoint.json](checkpoint.json) contains
exact identities, source copies, evidence and limits. Raw streams stay local.
Continue from [STATUS](../../../juxtopposed/STATUS.md).
