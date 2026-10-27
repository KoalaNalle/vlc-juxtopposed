# Current status

Updated 27 October 2026. Beta work is **active**, with the desktop available.
Checkpoint and stop owned jobs by **27 October 2026, 08:00 Helsinki** (06:00 UTC).
The user authorized tested nightly commits/pushes; release publication is separate.

## Selected checkpoint

- Branch: `juxtopposed-playback-bar`. Exact source/binary identities are in the
  [library defaults/query receipt](../juxtopposed-references/native/home-library-defaults-queries-20261027/README.md).
  The four unrelated audio experiments remain untouched.
- Selected tuple: Core `fdfcfcd5`, ordinary GUI `b5e5bd29`, library `13ccdc36`,
  MP4 `df15e8ed`, subtitle demux `11f2b78e`, WASAPI `e5560794`, MKV `f46d8d15`,
  recording `930205b1`. Ordinary isolated runtime:
  `build-windows/beta-single-frame-20261025-night/ui-collection-sort-fixed-r344/runtime`.
  This is not a release package. GUI337, GUI329 and GUI311 remain recoverable
  at `ui-tooltip-fixed-r337/runtime`, `ui-music-keys-fixed-r330/runtime` and `ui-record-fixed-r323/runtime`.
- Active outputs: `build-windows/beta-single-frame-20261025-night`.
  Inventory `inputs-r344.json` predates this documentation checkpoint; freeze
  fresh inputs before another guarded test. All owned tests have exited normally.

## Verified progress and scope

- Songs uses native indexed audio folders/All songs, original category artwork
  and native TrackId sorting. Native Play survives source-model removal.
  [Library receipt](../juxtopposed-references/native/home-music-library-20261027/README.md)
  separates six Play-lifetime cases, queries and ordinary cold/traditional routes.
- Artist headers, genre breadcrumbs, missing-year album text and table Enter
  are repaired. Eight native query functions and scoped desktop journeys pass.
  [Details receipt](../juxtopposed-references/native/home-music-details-20261027/README.md).
  Equal Evening Album titles have distinct native containing-folder identities;
  no unsupported grouping/deduplication policy changed.
- Library Back/Forward restores independent filters; Clear and cold native sort
  persist. [History receipt](../juxtopposed-references/native/home-library-history-20261027/README.md).
  [Multi-selection](../juxtopposed-references/native/home-music-selection-20261027/README.md)
  verifies exact appended tracks and held input; successive navigation is not
  proof of overlapping in-flight queries.
- Shortcut rows are visible; conflict Cancel, failed-save retry, saved reopen,
  restart mapping, movable editor and Reset have native/ordinary evidence.
  [Shortcut receipt](../juxtopposed-references/native/home-controls-shortcuts-20261027/README.md).
  Intentional configuration errors and strict producer failures remain explicit.
- First Playlists and Quit tiles are repaired; ten callback cases, fifteen
  PageLoader groups and ordinary GUI311 route/Quit checks pass. Snapshot Add/Save
  and additional toolbar consumers have scoped evidence.
  [Customizer receipt](../juxtopposed-references/native/home-customizer-routes-20261027/README.md).
- Sparse subtitles no longer delay A/V recording. Eleven calculation cases pass
  (three fail before); actual ordinary/native files decode aligned AV. Passthrough
  still waits for a usable keyframe; no frame-exact start is claimed.
  [Recording receipt](../juxtopposed-references/native/home-recording-start-20261027/README.md).
- Mouse-selected music rows now own Enter/Space/navigation. GUI329 verifies real
  source changes, artwork/artist/Unicode metadata, native playlist origin and live
  literal rename with a held clock. The rename popup moves by its title bar.
  Native routing passes28/0/0; the music component suite passes. The accepted app
  exits0/602modules/zero errors/all guards. The earlier metadata source-guard
  failure remains retained; relevant journeys were repeated on GUI329.
- Genuine0/20/40-second chapters pass native and ordinary paused/playing seeks,
  literal names and plain-source clearing/restoration. Pointing tooltips now
  preserve literal tags; three layout failures before become five passes after.
  GUI337 verifies rendered text/edge placement and exits0/602modules/zero errors/
  all guards. The chapter receipt qualifies EOF, setup failures and stale captures.
- Two saved global Play/Pause combinations work from a separate foreground app.
  B leaves A's paused position exact and keeps its shortcut after A exits.
  Traditional Simple Preferences Hotkeys is reached by keyboard and cancels with
  the player clock held. Both ordinary apps exit0/602modules/zero checked errors/
  all original guards. [Global shortcut receipt](../juxtopposed-references/native/home-global-shortcuts-20261027/README.md)
  retains the failed accessibility click and stale/occluded capture limitations.
- Native model/service checks pass9 cases, including three forced overlapping
  search/parent/sort-order queries with late abandoned results. Albums, Artists
  and Genres now mark verified native defaults;14 native query cases,16 component
  groups and ordinary GUI344/restart checks pass. Explicit sort choices persist.
  [Library defaults/query receipt](../juxtopposed-references/native/home-library-defaults-queries-20261027/README.md)
  keeps the supplied-row race fixture distinct from native SQL/desktop timing.
- Prior subtitle/Core evidence remains scoped:
  [late-cue replay](../juxtopposed-references/native/home-late-preset-replay-20261027/README.md),
  [selection/first-frame clock](../juxtopposed-references/native/home-subtitle-selection-clock-20261027/README.md),
  [initial paused caption](../juxtopposed-references/native/home-cold-paused-caption-20261027/README.md)
  and [fragment seeking](../juxtopposed-references/native/home-fragmented-mp4-20261026/README.md).

## Next action and open questions

1. Continue W3 interaction/design checks and W2 remaining native
   consumers and current-tuple category workflows. Preserve earlier layout/draft/
   failure evidence. HDR is truthfully disabled pending a native consumer.
   Other recording formats/policies and subtitle overlap remain distinct.
2. Shortcut full actions and same-chord registration feedback, ordinary rapid
   overlapping library queries and every category's native Default-sort policy
   remain open. Two distinct Play/Pause global mappings and traditional Hotkeys
   keyboard navigation now have scoped evidence; they do not close all shortcuts.
3. Continue W3 design, focus/accessibility, theme, Windows OS-DPI and monitor work
   in [BETA](BETA.md) and [SETTINGS](SETTINGS.md). Simulated component scale is
   not OS-DPI proof. Disc metadata is visible; unsupported Disc sort is disabled.
   All six beta gates remain open, including consolidated final-package replay.
4. Secondary-screen fullscreen idle capture loses captions on older Core204 too;
   physical display versus capture remains unqualified. Sparse/long-cue seek
   cost, other MP4 indexes and live behavior remain open. The earlier cold RGB
   assertion stays failed; only the later exact dual-caption Reset pair matches.
5. Online subtitle search/download needs implementation and an owner API
   application decision. Windows10 AUDIOSES and physical endpoint/device-loss/
   receiver/casting checks retain explicit external deferrals.
