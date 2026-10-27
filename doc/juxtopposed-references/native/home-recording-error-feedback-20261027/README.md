# Recording destination error feedback and recovery

27 October 2026. Source `9bc894ae8e83c3443402205e9bd9950a316eff54`. A recording whose output could not be opened
previously failed only in the log. The ordinary player showed no save-error notice.
The recording module now reports final output-open failures through VLC's existing
dialog service, with the destination and folder/write-access/free-space guidance.
The message is emitted once for this failure, while ordinary logging is preserved.
No new dialog framework, Core ABI, GUI rebuild or dependencies are introduced.

## Evidence

- Ordinary before r474: a generated Matroska recording points at an owned file
  instead of a directory. The output fails, no file is saved and no error notice
  appears. The source keeps playing and pauses normally. The three expected
  output-open log errors and normal exit/all ending guards are retained.
- Native public API before/after r475/r477: the same generated WAV failure/retry
  test registers LibVLC's public error callback. Before, it receives no dialog;
  after, it receives exactly one Recording failed error with the blocked path and
  guidance. The primary input survives and a valid-folder retry in the same player
  decodes. Markers stay unchanged; normal exits and every guard pass.
- All22 direct recording production checks still pass after the module change.
- Ordinary GUI441/Core265/record476
  `fb4e9c06`
  /mux464 r478: the failed output produces the visible red Recording failed
  alert while the one-hour generated WAV continues playing. Ctrl+M / Errors shows
  the complete retained destination and guidance. Playback pauses and its exact
  position holds while the details are inspected and closed.
- The owned marker is renamed, preserving its bytes, and a directory is created
  at the same test path. The existing player then records successfully without a
  restart or settings change. The recovered file contains177,600mono8kHz16-bit
  PCM samples (22.2seconds), all matching the generated silent input, and FFmpeg
  decodes it with zero errors. Final pause holds at
  `0.03411735361111111` before normal quit. Native config and the
  complete player-profile catalogue remain identical.

Both ordinary runs exit0 with602plugins and all source/runtime/base/fixture/
producer/cache guards. Each has the same three controlled output-open diagnostic
lines and no unexpected checked diagnostics. This is an intentional failure test,
not a claim of zero log errors. The ordinary negative uses Matroska; the recovery
uses a one-hour WAV to avoid EOF during inspection. Native before/after use the
same WAV scenario. One Record tile is preseeded only in each isolated test profile;
this does not qualify a customizer gesture.

## Remaining limits

The first Show Details click did not establish that route opening: the following
window observations show no details dialog. The existing notice hides after five
seconds, which can expire during automation. Details were verified through the
retained Errors history instead. Do not claim the toast button or its keyboard/
focus behavior accepted from that attempt. The current source routes that button
to the same messages service, but source inspection alone is not desktop evidence.

This repair reports final output-open failure. Mid-write disk-full/disconnection,
every error/retry combination, localization, actual DPI, screen-reader/focus,
complete visual scoring, independent review and clean-package/beta gates remain
open. Older subtitle-overlap and other-format receipts retain their own exact
module identities; this six-line error-report change does not silently replace them.
No exact recording-boundary or strict conversion-tail claim is added here.

Media and binaries remain ignored. The receipt preserves generated-fixture
identities, callback/ordinary captures, diagnostics, normal exits, producers and
the reversible destination-repair journal. `checkpoint.json` binds original and
compressed SHA-256; every artifact was decompressed and verified. No firewall
rules, security/privacy or Defender settings changed. This is a frozen scoped
record, not full beta acceptance.
