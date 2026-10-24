# Application Quit: stop playback before interface teardown

21 October 2026. **Selected ordinary audio/video behavior passes; complete shutdown
and beta acceptance remain open.** This continues the
[shared native settings work](../home-settings-concurrency-20261021/README.md).

This earlier checkpoint uses ordinary GUI `d148035d54be…` and repaired Core `3a7b0a360e3c…`. The production
change registers a native state listener and requests Stop under the same player
lock. It waits for stop completion before calling application Quit. It handles
rapid Start despite a stale GUI state, repeated Quit requests, and a new Start
before the queued stop finalizer. Listener cleanup is tied to the dialog
provider. Generic interface unloading is unchanged.

The [ordinary pair](ordinary-pair-r3.json) has two real VLC windows with separate
owned WAV files, a common native settings file and separate portable Qt stores.
A plays fullscreen on the primary monitor; pausing/resuming B on the second
monitor leaves A playing. A quits directly from fullscreen while B continues,
then B quits directly. Both exit normally with native stop acknowledgment before
Qt teardown, no native errors, no Qt diagnostics, and unchanged media/runtime
identities. Only one explicit render endpoint is available: Default aliases it.
The generated WAVs are silent; real MMDevice/WASAPI output opens, but audible
quality is not part of this check.
This does not qualify two physical outputs or shared Qt profile/window storage.

The [ordinary movie-plus-music pair](ordinary-av-pair-r3.json) repeats this with
H264/AAC video fullscreen in A and a WAV in B. Native MMDevice/WASAPI opens in
both, and Direct3D11 opens in A. B pauses/resumes independently, continues after
A's direct fullscreen Quit, then quits normally. Both logs remain clean and both
saved normal window rectangles remain unchanged. The generated video is an
owned test clip, not visual fidelity or audible-quality acceptance.

Four private R3 audio cases pass: idle, established playback, paused playback
after entering fullscreen, and six repeated Quit calls. The R3 fullscreen
fixture checked entry before playback, so immediate-before-Quit player
fullscreen needs the corrected fixture. Source and binary identities, individual
results and failed candidates are bound by [verification.json](verification.json).
Private R5 adds clean established fullscreen video, paused video and fullscreen
combined audio/video cases. It checks actual player fullscreen immediately
before Quit and detaches its listener at MainCtx destruction; queued native
events share that owner. All three exit normally with native STOPPED before
interface teardown and no Qt/native diagnostics.

Rejected scopes are retained:

- Rapid Start and restart-before-finalizer reach native STOPPED before Qt
  teardown and exit normally, but cancellation produces cache/demux probing
  errors. They fail the strict diagnostic gate. No error filter was relaxed.
- Private R3 video hits a player-listener assertion and is ended with Abort in
  its owned runtime dialog. The fixture detached its listener at QApplication
  exit; a video window can retain Qt after interface teardown and main-player
  deletion. That fixture lifetime error is not evidence of a production listener
  leak. The old file log also contains Qt teardown warnings; neither the
  assertion nor those warnings reproduces with the corrected R5 fixture or the
  ordinary movie-plus-music pair.
- R1 used the queued GUI state and was superseded before qualification. R2's
  ordinary build succeeded, but its private fixture missed an include and did
  not compile. Production is unchanged in later private fixture revisions.

R4 was built but superseded before execution because queued fixture events
still targeted the longer-lived QApplication. R5 ties both observer and queued
work to MainCtx. QApplication exit only updates the report. Production GUI bytes
are unchanged across these fixture revisions.

The initial R5 video-only runner result was rejected for missing WASAPI. That
clip has no audio track. The raw failure is retained alongside a qualified
video-only receipt requiring a selected video track and real Direct3D11 output.
The runner now accepts an explicit audio/video/combined media contract; diagnostic
checks were not relaxed. The paused video and combined-media cases use that
corrected contract directly. See the opt-in
[test instructions](../../../../modules/gui/qt/tests/native_ui/quit_stop_journey.md).

These results do not cover every video backend, rapid-start clean diagnostics,
hardware hotplug, shared Qt ownership, Windows 10, OS DPI, or a final beta package.

The [later input cancellation checkpoint](../home-input-cancellation-20261021/README.md)
qualifies rapid Start and restart on new Core/cache with unchanged GUI/R5 fixture.
The rejected R8 results here remain historical evidence, not current failures.
