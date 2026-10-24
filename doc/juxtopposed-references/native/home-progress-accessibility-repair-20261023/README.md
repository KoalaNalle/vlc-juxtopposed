# Progress accessibility repair, 23 October 2026

Direct accessibility value-setting now commits a seek in the ordinary player.
Qt's value interface writes the QML value property; the earlier progress bar
displayed that property but did not send it to VLC. The repair guards internal
display updates and routes an external value write through the existing seek
function once. Pointer/chapter state and the native seek-speed policy remain.

The unchanged build reproduces the missing native request. The candidate
passes39 actual-player checks, including forward/backward values, normalized
bounds, identical-value suppression, pointer compatibility, resume and no seek
feedback during normal playback updates. The private QtTest fixture stays out
of the ordinary DLL. Its first cold-pause setup failure remains separately.

Ordinary desktop accessibility requests show the expected blue25s, red5s and
green45s/15s video segments while paused. Pointer input, the observed global
arrow-key route, resume/Pause and Quit0 work. Final paused position stays exact.
These are coarse visual observations, not exact frame PTS or speech/focus tests.

Current runtime: `build-windows/beta-caption-accessibility-20261023/ordinary-ui-r4/runtime`.
Tuple: GUId368d5ec / Core000f1159 / MKVf46d8d15 / WASAPI4e4e.
The2,232-input freeze has actual copies in source-ordinary-r3. It is not a
translation-unit count. Three private native jobs and one ordinary GUI job
close normally; earlier successful receipts remain immutable.

Caption clearance is still open. The small-window bar covers the lowest line,
and redundant overlay shading darkens fullscreen captions. Saved subtitle
offsets and positioned ASS require care; no caption layout change is included.
Full accessibility/DPI, remaining track workflows and the full beta remain open.
No installs, commit/push, release/upload or shutdown occurred.

- [Checkpoint and hashes](checkpoint.json)
- [Review and remaining work](review.json)
- [Ordinary desktop observations](desktop-observations-r5.json)
- [Native candidate39 checks](native-after-r1b/journey.json)
- [Unchanged-build failure](native-before-r1b/journey.json)
- [Caption source investigation](caption-investigation-r5.json)
