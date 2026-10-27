# Current status

Updated 27 October 2026. Beta work is **active**, with the desktop available.
Checkpoint and stop owned jobs by **27 October 2026, 08:00 Helsinki** (06:00 UTC).
The user authorized tested nightly commits/pushes; release publication is separate.

## Selected checkpoint

- Branch: `juxtopposed-playback-bar`. Current source identities are frozen in the
  [music detail receipt](../juxtopposed-references/native/home-music-details-20261027/README.md).
  The four unrelated audio experiments remain untouched.
- Selected tuple: Core `fdfcfcd5`, ordinary GUI `1a0c6c19`, library `13ccdc36`,
  MP4 `df15e8ed`, subtitle demux `11f2b78e`, WASAPI `e5560794`, MKV `f46d8d15`.
  Ordinary runtime:
  `build-windows/beta-single-frame-20261025-night/ui-music-details-fixed-r290/runtime`.
  Its isolated profile is not a release package. GUI286/Core265 remains recoverable
  at `ui-music-play-r286/runtime`; GUI260/Core265 at `ui-selection-fixed-r265/runtime`.
- Active outputs: `build-windows/beta-single-frame-20261025-night`.
  Inventory `inputs-r290.json` predates this documentation checkpoint; freeze
  fresh inputs before another guarded test. All owned tests have exited normally.

## What passed

- Songs now opens native indexed audio folders and an All songs fallback, using
  original Figma category artwork. Mixed video is excluded; nested/Unicode
  folders, search and native media-track order have scoped query/desktop evidence.
- Keyboard Play survives removal of its source model. Six native functions
  verify the real service/queue, offset/order/origin, read cancellation, short/
  invalid results and shutdown. Eight native query functions pass; the old Track
  mapping fails its intended control while artist collection-count sorting stays.
- Ordinary Enter plays Unicode/filtered songs and video. Search empty/clear,
  paused/playing navigation, traditional flat Tracks and cold folder/Track-sort
  restoration pass. Three accepted apps exit0/602modules/zero errors/all guards.
  Earlier setup failures and the r283 producer's false combined runtime guard are
  retained separately; that original result is not silently promoted to a pass.
- Artist details now have shared navigation/search/sort/view controls; traditional
  details retain their custom header. Genre breadcrumbs and missing-year album
  text are repaired. Single-artist table Enter now navigates without also playing.
  Explicit keyboard Play/Enqueue, artist search/view/history, genre/album drill-down
  and held clock/queue pass on GUI290. Both final apps exit0/602modules/zero errors/
  all guards. GUI289's earlier table-Enter failure remains in the receipt.
- The two Evening Album entries are distinct native containing-folder identities
  without year/multi-disc matching evidence. Native queries and a read-only
  generated database/source check support preserving both; no grouping policy changed.
- Prior subtitle/Core results remain scoped to their recorded tuples:
  [late-cue preset replay](../juxtopposed-references/native/home-late-preset-replay-20261027/README.md),
  [selection and first-frame clock](../juxtopposed-references/native/home-subtitle-selection-clock-20261027/README.md),
  [initial paused caption](../juxtopposed-references/native/home-cold-paused-caption-20261027/README.md)
  and [fragment seeking](../juxtopposed-references/native/home-fragmented-mp4-20261026/README.md).
  This music-only GUI change does not supply a consolidated final-package replay.

## Next action and open questions

1. Continue S13 with initial empty sort-criteria feedback and search-history
   semantics; then the next applicable W2 customizer/shortcut/library workflow.
   Multi-selection artist Play and rapid in-flight artist changes still need
   ordinary coverage. Banner buttons were activated with Space; an earlier Return
   attempt did not enqueue, and its misleading observation filename is qualified.
2. Finish remaining category journeys and
   W3 interaction/design/accessibility/Windows DPI in [BETA](BETA.md) and
   [SETTINGS](SETTINGS.md). Disc metadata is visible; independent Disc sorting
   has no native consumer and is disabled. All six beta gates remain open.
3. Secondary-screen fullscreen idle capture loses captions with hidden controls;
   older Core204 reproduces it before snapshots. Physical display versus capture,
   overlap, orientation/HDR and OS-DPI remain unqualified.
4. Sparse/long-cue seek cost, other MP4 index layouts and live behavior remain
   open. The earlier cold RGB assertion remains failed; only its later exact
   dual-caption before/Reset pair has pixel-equality evidence.
5. Online subtitle search/download needs implementation and an owner API
   application decision. Windows10 AUDIOSES and physical endpoint/device-loss/
   receiver/casting checks retain explicit external deferrals.
