# Paused subtitle replacement investigation: canonical review

The accepted ordinary build loses captions when an embedded subtitle is changed
or an external subtitle is explicitly selected while paused. The new candidate
restores all20 tested caption observations across five cases, but is rejected
for application: both no-audio cases advance the paused video-source timer point
by40ms on every selection refresh. The25fps fixtures make that one frame.
Matching unchanged controls retain the video point. WASAPI's best-source time
remains stale and cannot qualify exact video-frame conservation.

The working-tree player input and decoder sources remain unchanged. The accepted
runtime stays `build-windows/beta-caption-layout-20261023/ui-after-r1/runtime`,
GUId0ff14c6 / Core000f1159 / MKVf46d8d15 / WASAPI4e4e. Candidate Core5837a5b7
is separate. The queued-SPU-only candidate made no difference, and the raw-timer
candidate was corrected before execution. No production repair is accepted.

The [original evidence receipt](../home-paused-subtitle-replacement-investigation-20261023/README.md)
preserves341 hashed artifacts,21 normal native exits and188 snapshot records,
including failed checks and native diagnostics. This canonical review corrects
one overbroad target-conservation sentence in its review; the original remains
immutable. The checked comparison already records the40ms drift.

Next: trace the current displayed-video/SMPTE point and seek normalization, or
refill only subtitles. Repeated selection must preserve the paused picture/time
before maintained native regression, ordinary Core build and desktop validation.
Cold initial captions and automatic external selection remain separate open
flows. Full caption layout, accessibility/DPI, physical outputs, Windows10,
independent review, packaging and the full beta remain open. No install,
commit/push, release/upload or shutdown occurred.

- [Canonical review and limitations](review.json)
- [Paired comparison and time drift](comparison.json)
- [Correction record](erratum.json)
- [Checkpoint and original evidence reference](checkpoint.json)
