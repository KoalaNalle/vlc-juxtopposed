# Subtitle selection and first-frame clock - 27 October 2026

Core `fdfcfcd5`, ordinary GUI `8e92470c` and MP4 `df15e8ed` activate a second
subtitle while paused at the opening80ms frame. Previously, the video source
existed but its timestamp cache waited for frame-rate metadata. The repair keeps
the video timestamp available and still withholds frame-number notifications
until that metadata is known. Sixteen source-level before/after cases qualify
that contract; the ordinary app separately qualifies real track activation.

Both ordinary timing editors reflect actual secondary availability. Disabled
controls are skipped by Tab. Native same-object marks cancel on subtitle
selection changes without a seek; the previous hooks fail the isolated control.
The timing-button animation uses the existing background color property.

Three opening-frame native exports keep marker0: both captions, Finnish delayed
333ms, and both after Reset. The first and last images are pixel-identical.
Playback then reaches both LAST cues and repauses, with normal app exit and no
error diagnostics. Native export and ordinary display caption placement differ;
the first analysis used the wrong image band and its correction is retained.

Both31-function software/hardware suites, real-WASAPI delay rows,13 startup
controls,12 MP4 cases and official A-B/timer programs pass. Warning counts remain
explicit in [checkpoint.json](checkpoint.json), alongside early test-loader
failures, the diagnostic trace and exact source/runtime identities. Temporary
trace code is absent from Core265. Raw streams, binaries, profiles and personal
media are excluded from these compressed generated artifacts.

The older ordinary late-cue preset replay and broader beta/display/DPI/package
gates remain open. This is a source nightly checkpoint, not beta readiness.
Continue from [STATUS](../../../juxtopposed/STATUS.md).
