# Current status

Updated 27 October 2026. Beta work is **paused at the checkpoint before the
08:00 Helsinki cutoff**. All owned desktop/native/build jobs have finished;
test applications closed normally. The full beta objective remains unfinished.

## Selected checkpoint

- Branch: `juxtopposed-playback-bar`. Latest pushed code: `f0e2c9ba89`.
  This session also pushed `0e6468a003` and the earlier scoped nightly commits.
- Ordinary GUI357 `caa02e28`; Core `fdfcfcd5`, library `13ccdc36`, recording
  `930205b1`, MP4 `df15e8ed`, subtitle demux `11f2b78e`, WASAPI `e5560794`,
  MKV `f46d8d15`. Exact full identities are in the linked receipts.
- Runtime: `build-windows/beta-single-frame-20261025-night/ui-music-toolbar-fixed-r360/runtime`.
  This is an isolated test runtime, not a release package. GUI349/344/337/329/311
  remain recoverable. Active outputs stay in the same work root.
- `inputs-r377.json` predates this final documentation checkpoint. Freeze fresh
  inputs before resuming guarded jobs. The four unrelated audio experiments remain
  untouched. No test/build is being left running at the pause.

## Latest verified work

- [Compact music toolbar](../juxtopposed-references/native/home-music-toolbar-20261027/README.md):
  fresh/reset audio defaults follow the54px reference with real15-second controls.
  Twelve native cases, four component groups and25customizer checks pass. Ordinary
  paused pointer/Return/Space seeking, small/maximized/fullscreen, track/queue,
  audio/video, cold layout, preserved alternative and theme/cold-theme journeys pass.
  Existing saved layouts stay intact. System selection does not prove Windows
  theme-change propagation; full visual/OS-DPI/accessibility/package gates stay open.
- [Playback finish action](../juxtopposed-references/native/home-finish-action-20261027/README.md):
  Save preserves paused0; Quit VLC survives restart and exits normally at final
  queue completion or native Stop. Clearing it to Stop keeps the app open on Home.
  All three ordinary runs pass0/602/no checked errors/all guards.
- [Music sidebar](../juxtopposed-references/native/home-music-sidebar-20261027/README.md)
  open/closed interaction and keyboard handoff, plus [library defaults/query races](../juxtopposed-references/native/home-library-defaults-queries-20261027/README.md)
  and [global shortcuts](../juxtopposed-references/native/home-global-shortcuts-20261027/README.md)
  retain their recorded scopes. Earlier [music library](../juxtopposed-references/native/home-music-library-20261027/README.md),
  [details](../juxtopposed-references/native/home-music-details-20261027/README.md),
  [history](../juxtopposed-references/native/home-library-history-20261027/README.md),
  [selection](../juxtopposed-references/native/home-music-selection-20261027/README.md),
  [keys/metadata](../juxtopposed-references/native/home-music-metadata-keys-20261027/README.md),
  [shortcuts](../juxtopposed-references/native/home-controls-shortcuts-20261027/README.md),
  [toolbar routes/Quit](../juxtopposed-references/native/home-customizer-routes-20261027/README.md),
  [recording](../juxtopposed-references/native/home-recording-start-20261027/README.md)
  and [chapters/tooltips](../juxtopposed-references/native/home-chapter-tooltip-20261027/README.md)
  are scoped evidence, not full beta acceptance.

## Resume here

1. Repair the **music navigation rail** using the established shared navigation
   service/component. The [open review and ordinary failure](../juxtopposed-references/native/home-home-navigation-review-20261027/README.md)
   retain the exact Figma subtree, seven exports and source/binary identities.
   Clicking rail Add opens only its tooltip; Home/Recent and original route icons
   are missing from the current four-item music list. Source-panel Add is separate.
   Qualify the Video/Music asset-state variants,50px rail/427px total geometry,
   short-window scrolling/focus, pins and paused/playing independence. Avoid two
   active pin/collection handlers in the hidden video drawer and music pane.
2. Continue W3/W2 in [BETA](BETA.md) and [SETTINGS](SETTINGS.md): music header/title-bar
   source context, remaining native/category actions, full shortcuts/conflict
   feedback, ordinary rapid queries, recording formats/subtitle overlap and HDR.
   HDR remains truthfully disabled; unsupported independent Disc sort stays disabled.
3. W1 remains qualified by its exact receipts: secondary-screen fullscreen idle
   capture also loses captions on older Core204; physical output versus capture
   is unqualified. The earlier cold RGB assertion stays failed; only the later
   dual-caption Reset pair matches. Sparse/long-cue cost, per-track combinations,
   other MP4 indexes and live behavior remain open.
4. Online subtitle search/download still needs implementation and an owner API
   application decision. Windows10 AUDIOSES and physical endpoint/device-loss/
   receiver/casting checks retain explicit external deferrals.
5. All six beta gates remain open, including full design/accessibility/Windows DPI,
   integrated regression, independent final review and a clean matched package.

## Saved question

Should Continue Watching retain all recently watched videos, or only unfinished
items? Its native recent-history query currently includes the fixture that reached
EOF; the read-only owned database has position0.995987,duration-1,play_count0.
This is a policy observation, not an accepted completion filter. If resume-only
is intended, qualify native count/order/search/events before pagination/limiting.
