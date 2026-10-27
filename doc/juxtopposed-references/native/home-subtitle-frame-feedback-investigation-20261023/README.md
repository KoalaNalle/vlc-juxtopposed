# Subtitle frame-feedback investigation, 23 October 2026

The misleading Next frame OSD comes from a paused decoder flush borrowing the
user frame-step counter for one internal refresh picture. Its completion emits
a user-facing frame status. The maintained optional callback probe reproduces
ten unsolicited successes in each of seven unchanged controls.

A private decoder candidate distinguishes that flush-owned picture without
changing the rendering request. In one paired case it removes all unsolicited
events while two real next-frame requests still each return success and advance
the actual marker125 to126 to127. This is limited private evidence, not acceptance.

The extended test also exposes a separate failure: the external caption is
visible before/during the steps but disappears after resume. All seven old
controls and the private candidate fail the unchanged externalDistinct check.
SubRip/ASS/MP4, WASAPI/no audio and two-track MKS reproduce it. Whether stepping
or the extra paused wall time causes the loss is not established. No failing
caption assertion or threshold was removed, and no complete case passes.

All8 native children exit normally;136 snapshot hashes verify. The first actual
control parent and private parent stop on the retained behavior failure. A
separate early launcher preflight failure occurred before staging/child launch
while source copying was still active; its recovery is recorded. The matching
C header companion supplies callback layout rather than guessed offsets.
There are2,355 actual frozen candidate-input copies and2,347 prior input copies.

No production Core source is applied. Accepted runtime remains
`build-windows/beta-external-subtitle-acceptance-20261023/ui-external.srt-r3/runtime`,
Coreea14300c / GUId0ff14c6 / MKVf46d8d15 / WASAPI4e4e. Candidate77670ec8 is not
qualified for ordinary/desktop use. Earlier paused external-selection results
retain their earlier no-step scope. No install, commit/push, release/upload or
shutdown occurred. Native expected diagnostics and compiler warnings remain.

Next compare an equal-duration pause without steps and trace subtitle clock/
queue lifetime across resume. Resolve the caption failure, then qualify feedback
and ordinary/desktop behavior before application. The complete13-category/
32-surface beta, original latest-runtime preview-hover check, remaining workflows,
design/accessibility/DPI, platform/review and clean-package gates remain active.

- [Per-case failures, callbacks and markers](comparison.json)
- [Source reasoning and remaining work](review.json)
- [Proposed decoder source](candidate-r2/decoder.c)
- [Artifact hashes](checkpoint.json)
