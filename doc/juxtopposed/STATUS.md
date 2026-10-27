# Current status

Updated 27 October 2026. Beta work is **active**, with the desktop available.
Checkpoint and stop owned jobs by **27 October 2026, 08:00 Helsinki** (06:00 UTC).
The user authorized tested nightly commits/pushes; release publication is separate.

## Selected checkpoint

- Branch: `juxtopposed-playback-bar`. Current source identities are frozen in the
  [recording receipt](../juxtopposed-references/native/home-recording-start-20261027/README.md).
  The four unrelated audio experiments remain untouched.
- Selected tuple: Core `fdfcfcd5`, ordinary GUI `19ab9d95`, library `13ccdc36`,
  MP4 `df15e8ed`, subtitle demux `11f2b78e`, WASAPI `e5560794`, MKV `f46d8d15`,
  recording `930205b1`.
  Ordinary runtime:
  `build-windows/beta-single-frame-20261025-night/ui-record-fixed-r323/runtime`.
  Its isolated profile is not a release package. GUI301/Core265 remains recoverable
  at `ui-controls-shortcuts-cold-r302/runtime`; earlier music runtimes are retained.
- Active outputs: `build-windows/beta-single-frame-20261025-night`.
  Inventory `inputs-r323.json` predates this documentation checkpoint; freeze
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
- Library search now follows its session Back/Forward entry, restoring before
  selection, with independent detail filters and persisted Clear. Traditional
  popup initialization preserves the restored query. Songs marks its actual
  native Alphabetic fallback and toggles order; cold order persists with a fresh
  query. Three GUI293 apps exit0/602modules/zero errors/all guards. The staging
  helper copies only link-limit failures; accepted runtime identities stay intact.
- Table/grid multi-artist Play appends the exact four tracks and starts the first
  new item. Successive artist detail changes preserve the held input/queue and
  show the correct tracks. GUI293 exits0/zero errors/all guards; this does not
  prove overlapping in-flight requests. See the
  [selection receipt](../juxtopposed-references/native/home-music-selection-20261027/README.md).
- The blank unfiltered shortcut table is repaired on GUI301. Native row geometry
  fails before (14pass/1fail) and passes after (15pass/0fail). Ordinary conflict
  Cancel, failed-save draft/retry, saved reopen, restart mapping, movable popup,
  Reset and cold traditional Space playback pass. All four apps exit0/all guards;
  the intentional failure run retains its one expected configuration error and
  strict runner assertion. Three ordinary runs have zero error diagnostics.
- Prior subtitle/Core results remain scoped to their recorded tuples:
  [late-cue preset replay](../juxtopposed-references/native/home-late-preset-replay-20261027/README.md),
  [selection and first-frame clock](../juxtopposed-references/native/home-subtitle-selection-clock-20261027/README.md),
  [initial paused caption](../juxtopposed-references/native/home-cold-paused-caption-20261027/README.md)
  and [fragment seeking](../juxtopposed-references/native/home-fragmented-mp4-20261026/README.md).
  These scoped GUI checks do not supply a consolidated final-package replay.
- First Playlists now survives library construction instead of opening Home;
  Quit uses the working native provider. Ten callback cases and fifteen PageLoader
  groups pass. Ordinary GUI311 verifies first Playlists, Home, held input and Quit,
  exiting0/602modules/zero errors/all guards. Snapshot Add/Save and additional
  paused transport, bookmark/favorite/sidebar/settings/PiP tiles have scoped
  GUI301 evidence. See the customizer receipt for the retained before failures.
- Sparse subtitle packets no longer advance the audio/video recording start.
  Eleven direct-production calculation cases pass (three fail before). The fixed
  ordinary recording decodes about19.5seconds of aligned AV; three native outputs
  also decode aligned AV. App exits0/602modules/zero errors/all guards. Compressed
  recording still waits for a usable keyframe; no frame-exact start is claimed.
  Original missing-video/misalignment controls remain in the recording receipt.

## Next action and open questions

1. Continue W2 with remaining customizer metadata/playlist-origin tiles, native
   consumers and current-tuple journeys. Preserve earlier tested layout/draft/
   failure work and the scoped recording fix; other formats/recording policies
   and subtitle overlap remain distinct. Shortcut global
   journeys, preserving earlier tested layout/draft/failure work. Shortcut global
   keys/full action coverage remains open. Traditional Simple Preferences opened
   on the other monitor; its Hotkeys category was not reached because Computer
   Use rejected a target outside the owner's bounds. Do not claim that table's
   ordinary acceptance. Rapid overlapping library queries and every category's
   native Default-sort policy remain separate from successive navigation checks.
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
