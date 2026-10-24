# Current-media Favorite — 19 October 2026

The native Favorite workflow passes its scoped checks. The later
[ordinary direct-input replay](ordinary-direct-input-replay-20261019.json)
verifies pointer, Space and Return with independent database readback and
unchanged visible paused playback. It uses ordinary GUI `fd7f8af9b480…`, no
private hooks, normal Ctrl+Q shutdown, no Qt diagnostics/native errors and601
loaded modules. Its generated profile intentionally contains three Favorite
copies, all following the same confirmed value. Exact native state independence
belongs to the private receipt below; screen-reader focus and final beta
acceptance remain open.

An earlier [ordinary attempt](ordinary-direct-input-20261019.json) retained a
duplicate-hotkey startup error. Removing the test environment's duplicate
plugin path fixes that error on the identical binary/runtime. Two native media
warnings remain recorded for review. A Close tooltip crossed the monitor edge;
popup placement needs a targeted follow-up. No raw Computer Use capture was
saved to the repository.

## Behavior and repairs

Favorite uses a MainCtx-owned service and the existing media-library worker.
It resolves the current source, checks the native mutation result and reads the
record back. Source generations prevent an older queued write from marking a
newly selected track. Library events refresh external changes; Library off and
unknown media remain unavailable. Releasing the input clears the source even
though VLC retains its last URL. Library favorites stay outside player profiles.

Real native tests exposed missing checked/checkable accessibility semantics and
Space reaching playback after pointer-style focus. Shared custom tiles now use
a checkable control with the existing presentation, an owner-confirmed next
state and the existing native key-ownership seam. Pointer, Space, Return and
accessibility Press request the native action without guessing success. Busy
Favorite operations retain focus and guard repeated actions. Failed writes show
an error and permit retry.

Queue Clear also exposed a caption binding that formatted a null duration while
ListView.count lagged behind the empty native model. The caption now handles
that transition. The actual Clear regression exits without its former warning.

## Verification

- [Native receipt](native-verification.json): initial89, cold-process reopen30,
  Library off21 and genuine write failure/retry43; 183 assertions across four
  normal process exits, without Qt diagnostics. Actual decoded owned media,
  paused input/clock/rate/volume/mute preservation, media replacement, queued
  source ownership, external favorite changes, keyboard and Qt accessibility
  actions, persistence and released-input refusal are covered.
- The failure case reserves the writer in only its owned SQLite database,
  without SQL data mutations. The failed native write leaves the independent
  database value false; after releasing the lock, focused keyboard retry saves
  true. Native readback, error recovery and exact primary state also pass.
- [Component receipt](component-verification.json): 24 checks of the production
  customizer/shared control, including checked state, Qt accessibility Press,
  keyboard activation and no premature toggle. These use owned service contracts.
- [Ordinary playback receipt](ordinary-playback-verification.json): redesigned,
  traditional and Library-on video/audio, generated live HLS and one read-only
  `D:\Media` sample pass. These establish playback/resource regression coverage,
  not direct Favorite activation or visual/DPI acceptance.
- [Direct-input status](ordinary-input-pending.json): the ordinary app starts on
  monitor2 with the generated library and copied prior consent/profile data.
  A Windows firewall prompt blocks further input. No security action or raw
  Computer Use screenshot was saved to this repository.

Private GUI: `c01bfecec0a982f6374f5e9268dd13a3bbd7d1b4fdbdef45a0d726a4dbdde2a5`.
Ordinary GUI: `d944735cbabbf820cfa94301b44333fa0fa88020f35fd556560c0199e86762e4`.
Ordinary runtime: `build-windows/beta-current-media-20261019/ordinary-runtime-r6a`.
Local matched Qt source checkpoint: `source-r6b` under that work folder.
The protected prior runtime, core/audio/library modules and owned media remain
unchanged. Generated-only native captures are included here.

Rejected attempts remain in the ignored work folder: R1/R2/R3 accessibility
failures, R4 Space activation, R5 queue-caption warning, R6 private fixture
compilation, and R6a's main-case setup race. The R6a recovery cases independently
pass. R6b settles real event-driven refreshes before its intentional queue barrier
and passes all four cases. No rejected run substitutes for the accepted receipt.

## Remaining scope

Ordinary direct input and the scoped playlist-origin app replay now have their
receipts. Finish the HDR capability/consumer and the remaining tile
actions. Complete the broader roadmap, original-design comparison, screen-reader,
OS-DPI and clean final-package gates. Existing font glyphs/control presentation
remain documented functional adaptations. The deferred Windows10 comparison and
historical AUDIOSES causality are still open. No commit, push, tag, upload or
designer contact is authorized or performed by this checkpoint.
