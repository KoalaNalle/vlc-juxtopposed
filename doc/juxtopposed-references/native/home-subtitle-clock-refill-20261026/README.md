# Subtitle clock recovery and saved-source refill - 26 October 2026

Core `54e3368f` with ordinary GUI `3530c5cc` passes the scoped cold-caption,
paused replacement, saved-source warm-return and cross-editor FPS journeys.
This is a development checkpoint; beta, design and package gates remain open.

A deferred refresh now waits for a current paused video clock, preserves its
encoded frame, and survives selection immediately before pause. A fresh paused
video point can replace an invalid audio master until audio resumes. Pending
refresh expiry preserves the independent A-B loop deadline.

The ordinary A-B-A failure is reproduced with a reused media item and preferred
embedded English: the saved external cue was discarded before its track was
selected. A source-only refill repairs the unchanged four-row native comparison
and the actual Next/Previous journey without seeking audio/video or their PCR.

Both hardware and software pass 29 subtitle functions. Thirteen cue functions,
four main startup rates, thirteen audio/rate cases, eleven format/delay/boundary
cases and the official A-B/timer suites also pass. The ordinary1.5x paused
seek/Reset pair retains exact native frame613. A separate0.5x playing Reset
revives an exhausted subtitle while the clock continues.

[checkpoint.json](checkpoint.json) binds exact binaries, sources, tests, native
snapshots, actual captures and normal exits. It preserves failed comparisons,
fixture corrections and harness cleanup qualifications. Raw output streams remain
local with hashes; generated-media captures and structured evidence are retained
here. Native fixtures use a separate support GUI and private C-test modules.

DX11 snapshot blending/preview, the earlier longer warm-caption observation and
the remaining W1/consumer/design/platform/package gates are still open. Read the
receipt's limits before combining evidence, then continue from
[STATUS](../../../juxtopposed/STATUS.md).
