# Startup rate and ordinary conversion - 26 October 2026

This scoped checkpoint uses regular Core `05ec8a7a` and GUI `ed1dc5f8`.
The beta session continues; this is not full W1, design or package acceptance.

The ordinary player previously lost its first caption when starting paused at
1.5x. Embedded-only, software decoding, dummy audio, vdummy output and clean
configuration controls retain the failure. Native LibVLC/main-player controls
pass the preceding Core too and therefore do not reproduce this Qt startup order.
Private clock logging shows the first audio block converting at 1x while its
subtitle uses 1.5x, scheduling the cue about 7 ms after the frozen render date.
A rate request can arrive while that decoded block waits for buffering. The
decoder now applies the pending output rate before releasing that block to its
output and establishing a reference. No arbitrary timestamp margin is added.

Regular ordinary startup shows the first caption while paused at 0.5x, 1x, 1.5x
and 2x. The 1.5x run resumes, pauses and seeks to the middle cue. The half-speed
software snapshot contains 1,483 bright caption pixels. Normal-rate GUI speed
selection and a paused seek retain the expected on-screen caption. Fifteen native
subtitle functions, thirteen rate/audio controls, eleven delay/boundary cases and
24 startup/seek/track cases pass with normal exits and unchanged input guards.

The ordinary conversion form opens from Media > Convert/Save over the paused
player. A generated source and destination with an accented character, spaces
and a literal percent sign convert successfully. Fully decoding the published
FLAC returns exactly 88,200 samples, byte-identical to the source, including its
nonzero tail. Cancel in the destination picker preserves the completed result.
A file created after picker selection is rejected with a useful existing-file
error and remains byte-identical. A new filename retries successfully with the
same exact decoded PCM. The header moves the popup within its owning window,
and Done returns to the unchanged paused player. Nine strict FLAC cases and
twelve native conversion-controller functions separately pass this Core.

Hardware DX11 snapshots have an open caption-blending defect: the screen shows
captions but the saved image omits them and logs blend/converter errors. Software
snapshot success does not close that path. Escape from a selected speed radio
also needs further keyboard-routing work. Rate/seek estimator notification
acceptance remains distinct from journeys that pop and recreate their subpage.
Ordinary busy cancellation, primary playing independence and invalid-source
recovery remain conversion follow-ups, along with broader visual/DPI/package work.

[checkpoint.json](checkpoint.json) binds full binary identities, frozen inputs,
source copies, ordered reports and private diagnostic traces. Diagnostics are
explicitly separate from the selected regular Core. Gzip preserves original
artifact bytes; no personal media is included. Continue from
[STATUS](../../../juxtopposed/STATUS.md).
