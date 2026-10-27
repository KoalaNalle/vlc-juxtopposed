# Paused subtitle replacement investigation, 23 October 2026

Switching an embedded subtitle or explicitly selecting an external subtitle
while paused clears the current caption in the accepted ordinary build. Resume
restores it. The earlier Matroska repair fixes the backward-seek cue gap, but
does not itself supply data for a newly selected subtitle decoder.

A separate Core candidate requests a precise refill at the existing seek-aware
paused time after a newly selected subtitle is reported. Five unchanged controls
have twenty missing-caption observations. Five matching candidate runs show all
twenty captions, with verified English/Finnish/external selected identities.
Generated colored video retains the expected coarse color regions, and native
images show the Finnish/external text correctly. However, both no-audio cases
advance the paused video-source timer point by40ms on each selection refresh.
At25fps this is one frame. The matching unchanged controls retain the point.
This violates frame conservation, so the candidate is rejected for application.
The publication preflight caught this defect before writing the receipt; its
failed assertion is preserved rather than waived as an acceptance check.

This is candidate evidence, not an accepted production repair. The working-tree
player input and decoder sources remain unchanged. The accepted runtime is still
`build-windows/beta-caption-layout-20261023/ui-after-r1/runtime`, with Core000f.
The diagnostic candidate Core5837a5b7 remains separate and must not be promoted. A queued-SPU candidate
was rejected after three native runs; a raw-timer candidate was corrected before
execution because it could choose an old point after an ordinary play/Pause.

All21 native processes close normally, with188 snapshot records checked where
available. Native window-closed teardown errors and initial buffer warnings are
retained. A normal exit is not a whole-workflow pass. Cold initial captions,
automatic external selection, exact output time/frame conservation, ordinary
build verification and desktop checks remain open. No install, commit/push,
release/upload or shutdown occurred. The full beta goal stays active.

- [Paired candidate comparison](comparison.json)
- [Source reasoning, limitations and next validation](review.json)
- [Checkpoint and artifact hashes](checkpoint.json)
