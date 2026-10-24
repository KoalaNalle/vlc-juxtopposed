# Input cancellation and two-player continuation

21 October 2026. Selected Windows checks pass; the complete beta remains open.
The [previous Quit checkpoint](../home-quit-stop-20261021/README.md) retains its
old Core results and rejected fixtures. This checkpoint uses Core `d04e9134a43f…`,
cache filter `1190b018b5e8…`, unchanged ordinary GUI `d148035d54be…` and private
R5 GUI `1b641370bc6e…`. Full identities and 521 frozen Core/header/cache inputs
are bound by [verification.json](verification.json).

Cancellation now stops unsuccessful format probing before the next module;
cancelled cache prebuffering releases its allocations and returns the module
matching stop result without reporting a genuine empty-file failure. Successful
probes and uncancelled read/demux failures retain their existing behavior. No
diagnostic filter was relaxed. These are two production file changes; generic
input initialization and the five protected audio/library plugins are unchanged.

- The same private R5 rapid-Start fixture on old Core R8 reproduces 17 native
  cancellation errors, with normal exit and correct stop-before-Qt ordering.
  That strict negative is preserved. New Core/cache passes both rapid Start and
  restart-before-finalizer, with normal exit, no native/Qt diagnostics, unchanged
  sources/media/runtime and native STOPPED before interface teardown.
- Four console processes pass. Eighteen rapid cancellations cover no input bytes
  and nonzero input/prebuffer bytes; each waits for a fresh native Stop callback.
  Subsequent real WASAPI playback and seeking advance native time and played
  counters. A WAV named `.mp3` correctly falls back to WAV. Empty and invalid
  media still enter Error and report genuine errors (13 and 17 respectively).
  Each process loads one plugin cache / 601 modules. See the
  [reproduction guide](../../../../test/libvlc/native_input_cancellation.md).
- The current-Core settings regression passes 38 checks in 27 normal processes,
  including 600 concurrent commits and 1,200 nonoverlapping lock events. Expected
  failures remain explicit. Six selected controller/store model processes pass
  97 cases, with no failures, errors, skips or stderr diagnostics. Their XML is
  included. These model results do not qualify desktop interaction.
- Two ordinary windows play generated movie/music media on separate monitors,
  sharing a native config but using separate Qt stores. A remains fullscreen
  while B pauses, seeks, mutes and resumes. A's direct fullscreen Quit leaves B
  playing; B then quits directly. Both exit normally, preserve their normal
  rectangles, use one plugin search, and log native stop acknowledgment before
  Qt teardown, without native/Qt diagnostics. Real MMDevice/WASAPI opens in both
  and Direct3D11 opens in A. The fixtures are silent; audible quality and design
  fidelity are outside this result. Default and the explicit endpoint resolve
  to the same device, so two physical audio outputs remain unqualified.

The initial console runner's four test failures are preserved and classified in
[runner-history.json](runner-history.json). It inherited a duplicate plugin path,
polled a stale initial STOPPED state, and could miss transient Error callbacks.
The final runner removes the environment variable before child startup, uses the
complete version-0 public callback layout, and checks meaningful post-seek
progress. A `.flac`-named WAV produced real forced-FLAC diagnostics and is not a
clean fallback acceptance. The intermediate R2 success predates the final
post-seek/runtime-failure gates and is recorded separately.

Ignored raw logs, snapshots, generated media and binaries remain under
`build-windows/beta-input-cancellation-20261021`. Public receipts bind raw log and
source hashes without copying hardware endpoint names. Older popup313/geometry194
desktop checks remain qualified on Core R8, not replayed on this Core. Shared Qt
profile/geometry ownership, two physical outputs, other protocols/backends,
hotplug, remaining GUI/fidelity/accessibility/OS-DPI and clean-package checks
remain open. Windows 10 comparison is deferred until the guest machine returns.
