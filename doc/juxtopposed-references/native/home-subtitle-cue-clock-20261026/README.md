# Subtitle cue clock and keyboard checkpoint - 26 October 2026

This scoped receipt preserves before failures and repairs for paused cue seeking,
decoder flush/shutdown, repeated redraw timing, before-first-cue timeline access,
external subtitle timestamp normalization and timeline keyboard ownership.
It is not full beta or final-package acceptance.

Native checks pass 25 subtitle functions, thirteen cue functions, eleven timeline
key rows, four main-startup rates, thirteen startup/audio cases and eleven
delay/boundary cases. Software and actual D3D11 frame checks include no caption
before the cue, the exact cue start, the middle of the cue and expiry afterward.
The focus component test uses queued completion and viewport recreation; native
key routing separately executes the exact production routing method.

The ordinary selected tuple, captures and normal exits are bound by
[checkpoint.json](checkpoint.json). Native fixtures use their separately recorded
support GUI binaries. See the checkpoint limits and setup corrections before
combining evidence. The earlier longer warm caption-loss observation remains
preserved and must not be silently treated as explained by the boundary repair.

The hang and forced cleanup, a later timer assertion with incomplete XML, and
diagnostic Cores are retained. They are not candidate runtimes. Frozen sources,
helpers, checksums, logs and captures are compressed alongside the receipt.
Continue from [STATUS](../../../juxtopposed/STATUS.md).
