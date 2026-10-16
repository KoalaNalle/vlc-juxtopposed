# Live window clocks and deferred pause — 16–17 October 2026 guest

Native controller/core checks and ordinary GUI playback pass within the scope
below. Public-stream, independent visual/DPI and final package acceptance remain
open. This checkpoint does not accept the complete beta.

VLC's live timer contract describes `length` as a seek range and `position` as
the position within that range. Its stream timestamp can be much larger. The
retained public-stream observation had a timestamp over ten hours and a four-hour
window; that earlier diagnostic timed out and is not an accepted lifecycle test.

The controller now publishes a separate `displayTime`: finite media keeps its
engine clock; seekable live media uses its position within the current window;
unusable/unknown/unseekable live ranges remain unknown. The main and traditional
bars, focused seek tooltip and artwork caption share those semantics. Live media
shows LIVE instead of presenting the seek window as a fixed media duration.
The duration toggle stays available for finite media and the subtitle-window
action retains its own behavior.

Seek callbacks carry a requested zero-based time. A paused finite seek to zero
must publish a valid zero clock rather than retain the previous time. A live
position request supplies no absolute stream timestamp, so raw `Player.time`
stays unknown until an actual timer point arrives. Its pending timer snapshot
also remains unknown: a following pause must not interpolate a relative offset
as an absolute subtitle/cue timestamp. The window position remains presentable.

The real local HLS check exposed a second problem in `ControlPause`: the demuxer
was paused before checking whether output buffering required deferral. Adaptive
live pause stopped the downloader, leaving the buffer unable to finish. The
core now makes that deferral decision while the demuxer is still feeding data;
it pauses the demuxer/output after buffering is ready.

The tightened fixture queues seek and pause together under one NORMAL player
lock, exercising the buffer deferral directly. The **same** maintained native
test executable fails the live pause case against
the original core, then passes all four cases against the repaired core. Both
processes terminate normally. See [original-core.json](original-core.json) and
[native.json](native.json). Five additional fresh-process live seek/pause,
resume/window/reset journeys pass with the repaired core, all normal exits:
[repeated-live-pause.json](repeated-live-pause.json). Their Qt totals include
initialization and cleanup; these are five repeated journeys, not fifteen
different functional cases. Three further fresh-process runs of the tightened
fixture also pass: [queued-pause-repeats.json](queued-pause-repeats.json).
The 25 timer cases and eight existing native
subtitle-controller cases also pass with the repaired core, zero failures,
errors or skips.

The ordinary Qt GUI and affected QML resources rebuilt successfully. All 1,245
frozen Qt/core source inputs remained unchanged through the GUI build; see
[gui-source-verification.json](gui-source-verification.json). A separate ordinary
runtime combines that GUI, the repaired core and the saved-preference launcher.
Default redesigned controls and explicit traditional controls both pass local
video/silent audio playback with decoded frame output, Direct3D 11, WASAPI and
normal exit. Local HLS also decodes and quits normally with no QML diagnostic.
See [ordinary-runtime.json](ordinary-runtime.json). The finite checks retain
seven delayed-callback QML warnings during teardown; they are still open.

The matching tuple replays the existing native metadata/opening/process/Subtitle
preferences suites at **18/42/9/37**, zero failures/errors/skips, normal exits.
See [Library regression](library-regression.json) and
[Subtitle preferences](subtitle-preferences.json). The unchanged 37-case suite
includes actual subtitle pixel/consumer checks. All five saved-Library/explicit
CLI startup cases also pass: [startup.json](startup.json). The Library driver now
creates a portable directory in every owned Windows runtime, rather than relying
on APPDATA alone for Windows Known Folder isolation.

The owned fixture uses generated silence, a loopback-only HLS server, real
adaptive demux and the actual NORMAL-lock main-playlist player/controller. Its
direct signal observers re-lock that player to verify notification ownership.
See [maintained reproduction instructions](../../../../modules/gui/qt/tests/README.native-timeline.md).
The playback component checks include LIVE/no-toggle behavior, finite controls,
keyboard/seek synchronization, five width/scale variants and eleven QML parses.
Component renders have not received an independent fidelity score.
Native live coverage here is owned seekable HLS. Unknown, unseekable, invalid
and overflow ranges have helper-level coverage; broader native protocol/state
coverage remains a beta gate.

Only unobserved bounded native runs count toward acceptance. Earlier failing
fixtures/build attempts and debugger observations are retained under ignored
local work; none is relabeled as a successful test. Raw logs/dumps, generated
media and private paths are excluded from this public proof. The ordinary
package is preserved. [verification.json](verification.json) records source and
binary identities and remaining limits. Automake core/native/GUI builds passed;
Meson registration has syntax review but no native Meson build. This is an
incremental Windows 10 tuple, not the final clean licensed beta package.
