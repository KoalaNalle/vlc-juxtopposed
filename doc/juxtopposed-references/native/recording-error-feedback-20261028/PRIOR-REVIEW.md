# Recording/export finalization feedback: scoped review

## Evidence and current flow

- The qualified avformat tail queue records a sticky failure on header/packet/
  finalization failure. `DelStream` calls `Mux` without a return value reaching
  Core, and `avformat_CloseMux` only logs `could not finalize muxed output`.
  `sout_MuxDeleteStream`, `sout_MuxDelete`, std Del/Close, record Close and
  `sout_StreamChainDelete` are void; the current call chain cannot return a
  finalization result to record or a conversion job.
- `record.c::OutputSend` also ignores `sout_StreamIdSend` failures. The native
  record state represents recording enabled/stopped, not a verified file commit;
  `vlc_player_SetRecordingEnabled` can display 'Recording done' when a request is
  queued, prior to native finalization. Do not label the checkmark a save receipt.
- Existing user-facing conventions include `vlc_dialog_display_error` for
  record output creation failure (`modules/stream_out/record.c`) and stream
  output setup failure (`modules/stream_out/es.c`). It returns immediately;
  under `src/interface/dialog.c`, provider callback runs under a provider mutex.
  Qt's DialogErrorModel copies title/text and invokes its own QObject, then
  `Dialogs.qml` shows the title and Show Details opens the Errors page. Its
  preference may suppress the popup; messages remain separately accessible.
- With no interactive provider, `no_interact`, or VLC shutdown, display_error
  falls back to log messages; it is not a durable completion result. At the
  avformat Close callback the mux object is still alive: std Close calls
  sout_MuxDelete before deleting its access; module_unneed invokes mux Close
  before Core deletes the mux. The Qt callback copies strings, not the mux.
  Thread/reentrancy must nevertheless be exercised, especially GUI teardown.
- ConversionController's current designed form uses isolated native FLAC and raw
  std output, not this avformat mux. It treats a native EOS without player error
  as stage success, then decodes and checks a duration tolerance before publishing
  the staged file. It removes its listener before `vlc_player_Delete`, which
  drains/closes the output. A generic avformat dialog cannot inform this worker,
  prevent publication, detect raw/FLAC close errors or prove sample completeness.

## Recommended bounded first slice: an avformat notification, not a commit result

After the frozen build completes, consider a narrowly scoped, separately reviewed
candidate which emits one generic 'Output could not be completed' error through
`vlc_dialog_display_error` on the first genuine avformat header/packet/final
trailer failure. Include a useful general description that output may be
incomplete and directs the user to Messages/log; avoid assuming every mux is a
recording or exposing a file path. Keep low-level diagnostic logs. An explicit
per-mux report-once flag should cover failure during normal Mux/DelStream and a
new trailer-only failure at Close, and should avoid a second notice at Close for
an earlier already reported failure. The queue's sticky error alone does not
identify which consumer owns the output. Successful close, never-started output,
and deliberate cancellation must not produce a false success or a spurious
failure. Verify exactly how the plugin learns cancellation before asserting the
latter: its current generic mux interface does not carry a cancellation reason.

IMPORTANT probe caveat: record.c can create temporary chains for mux capability
probing, deleting each test ES and its chain before choosing a real destination.
The probe list includes mkv; `standard.c` aliases mkv to avformat when applicable.
A generic close-time dialog can therefore report an expected probe failure as a
user-visible production failure. Before adding the notice, prove suppression or
mark the probe's parent noninteractive BEFORE creating its descendants, with a
parent whose lifetime covers the full probe chain; do not change global user
privacy choices. Probe behavior and fallback mux selection need real test evidence.
Multiple independent muxes, other higher-layer dialogs and sink failures can
still duplicate messages; define and test the intended one-per-owned-output scope.

Because a public error callback may run synchronously with the dialog provider
mutex held, do not introduce a handler that reenters the provider, calls player
APIs under prohibited locks, blocks on the GUI thread or retains the dying mux.
Check other consumers' error callbacks, not just Qt's queued implementation.
A report inside a locked stream output callback may be acceptable by analogy but
requires explicit deadlock/teardown tests; no lock-safety proof is claimed here.

## What needs stronger Core/API propagation

A popup alone cannot guarantee a correct conversion or recording completion
status. For S24 correctness, evaluate a scoped explicit terminal failure result
that survives mux/stream close, reaches the *specific* job after all writes and
trailer/access cleanup, distinguishes user cancel/probe from failure, and gates
publication atomically. Map stream-output error propagation, player EOS and sink
close/flush behavior before selecting an API; the current void chain means this
is more than an avformat-only dialog. Ensure no other player instance's failure
can affect the worker. Conversion's existing independent decode/duration checks
remain important but do not prove all bytes/samples or detect all close errors.
For S01, show the recording error while preserving playback, correct active
feedback and an honest output-completion state. W4/W5 still require full candidate
and final-package evidence; neither a dialog nor a scoped native writer test
closes the beta gates.

## Meaningful next tests

1. In an isolated *separate* native/libvlc instance, register
   `libvlc_dialog_set_error_callback` (VLC 4 registers this separately), not the
   shared Qt provider; use generated media and a controlled writable sink. Test
   one callback for header failure, first normal-write failure, failure arising
   only during Delete/tail drain, and trailer-only failure. Verify detail text,
   exact callback count, sticky behavior, no retry, normal teardown and preservation
   of retained input/config. A closed pipe may fail early; do not claim it proves
   the final-tail-only case without a deterministic failure seam or phase evidence.
2. Exercise expected format-probe failures before a successful actual recording,
   successful recording/close, zero-data close, unavailable-provider/no_interact,
   cancellation and successive separate outputs. Distinguish one-per-mux from
   one-per-operation and retain negative controls. Test provider callback and
   shutdown ordering for no deadlock, use-after-free or duplicate message.
3. On a matched ordinary Windows candidate, force a controlled finalization
   failure using an owned generated destination/fault sink without modifying
   system-wide security settings. Confirm the actual Record interaction, audio/
   video continue, an actionable notice appears once, Show Details exposes the
   corresponding entry, and a subsequent good recording succeeds. Repeat with
   notification preference disabled; verify expected Messages behavior separately.
   Test no-interact/fallback independently; do not promise an on-screen notice
   while the GUI is exiting or its provider has already been removed.
4. For S24, introduce a deterministic stage-finalization failure in the actual
   conversion output path (FLAC/raw, not only avformat), verify Failed instead
   of Succeeded, no destination publication, temporary-file cleanup, no false
   success, primary player unaffected, cancellation/retry and clean close. Add
   exact decoded sample/count regression, not duration tolerance alone.

This is a read-only review/proposal; no proposed change or test was executed.