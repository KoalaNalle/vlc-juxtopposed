# Current status

Updated 27 October 2026. Beta work is **active**, desktop available.
Checkpoint and stop owned jobs by **08:00 Helsinki today** (06:00 UTC).
Tested nightly commits/pushes are authorized; release publication is separate.

## Selected checkpoint

- Branch: `juxtopposed-playback-bar`. Latest accepted ordinary GUI349 is
  `987539e3`; Core `fdfcfcd5`, library `13ccdc36`, recording `930205b1`,
  MP4 `df15e8ed`, subtitle demux `11f2b78e`, WASAPI `e5560794`, MKV `f46d8d15`.
- Runtime: `build-windows/beta-single-frame-20261025-night/ui-music-sidebar-fixed-r349/runtime`.
  This is an isolated test runtime, not a release package. GUI344/337/329/311
  remain recoverable in the same work root. Exact identities are in the
  [music sidebar receipt](../juxtopposed-references/native/home-music-sidebar-20261027/README.md).
- Active outputs: `build-windows/beta-single-frame-20261025-night`.
  `inputs-r349.json` predates this documentation checkpoint; freeze fresh inputs
  before another guarded run. All owned tests have exited normally. The four
  unrelated audio experiments remain untouched.

## Latest verified work

- Music sidebar open/closed states now follow Figma's paired frames. The title
  button toggles the inline music panel; video retains its drawer. Component
  before/fixed evidence, small/maximized native states, exact paused retention,
  closed Space and reopened Down/Enter, plus audio/video transitions pass.
  The customizer fixture's two-row bar does not establish canonical sizing or a
  whole-player visual score. Both apps exit0/602modules/zero checked errors/all guards.
- [Library defaults/query receipt](../juxtopposed-references/native/home-library-defaults-queries-20261027/README.md):
  Albums/Artists/Genres mark verified native defaults; explicit choices persist.
  Fourteen native query cases, sixteen component groups and ordinary/restart
  journeys pass. Nine model/service cases include three forced overlapping late
  results; supplied row operations are distinct from SQL/desktop timing.
- [Global shortcuts](../juxtopposed-references/native/home-global-shortcuts-20261027/README.md):
  two distinct Play/Pause mappings work from another app; B holds A's exact
  paused position and survives A's exit. Traditional Hotkeys keyboard navigation
  and Cancel pass. Full actions and same-chord registration feedback remain open.
- Earlier scoped repairs/evidence remain valid for their recorded tuples:
  [music library](../juxtopposed-references/native/home-music-library-20261027/README.md),
  [details](../juxtopposed-references/native/home-music-details-20261027/README.md),
  [history](../juxtopposed-references/native/home-library-history-20261027/README.md),
  [selection](../juxtopposed-references/native/home-music-selection-20261027/README.md),
  [music keys/metadata](../juxtopposed-references/native/home-music-metadata-keys-20261027/README.md),
  [shortcuts](../juxtopposed-references/native/home-controls-shortcuts-20261027/README.md),
  [toolbar routes/Quit](../juxtopposed-references/native/home-customizer-routes-20261027/README.md),
  [recording start](../juxtopposed-references/native/home-recording-start-20261027/README.md),
  [chapters/tooltips](../juxtopposed-references/native/home-chapter-tooltip-20261027/README.md),
  [subtitle clock/selection](../juxtopposed-references/native/home-subtitle-selection-clock-20261027/README.md),
  [late cues](../juxtopposed-references/native/home-late-preset-replay-20261027/README.md)
  and [fragment seeking](../juxtopposed-references/native/home-fragmented-mp4-20261026/README.md).
  Equal Evening Album names retain genuine distinct native folder identities.

## Next action and open questions

1. Continue W3 interaction/design and W2 native consumers/current-category journeys
   in [BETA](BETA.md) and [SETTINGS](SETTINGS.md). Qualify canonical music layout,
   theme, focus/accessibility, actual Windows DPI/monitor states and other categories'
   native defaults. Component scale simulations are not OS-DPI evidence.
2. Full shortcut actions/conflict feedback, ordinary rapid queries, remaining
   recording formats/policies/subtitle overlap and native HDR remain open. HDR
   stays truthfully disabled; Disc metadata is visible with unsupported sort disabled.
3. Secondary-screen fullscreen idle capture also loses captions on older Core204;
   physical display versus capture is unqualified. Sparse/long-cue seek cost,
   other MP4 indexes and live behavior remain open. The earlier cold RGB assertion
   stays failed; only the later exact dual-caption Reset pair matches.
4. Online subtitle search/download needs implementation and an owner API
   application decision. Windows10 AUDIOSES and physical endpoint/device-loss/
   receiver/casting checks retain explicit external deferrals.
5. All six beta gates remain open, including independent final review and a clean,
   matched ordinary package with consolidated regression/design evidence. A work
   session or component pass does not complete the beta objective.
