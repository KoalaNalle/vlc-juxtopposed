# Current status

Updated 26 October 2026. Beta work is **active**, with the desktop available.
Checkpoint and stop owned jobs by **27 October 2026, 08:00 Helsinki** (06:00 UTC).
The user authorized a tested nightly commit/push. Beta gates remain open;
release publication is not authorized.

## Selected checkpoint

- Implementation checkpoint: `32160081114d5918835eb985e0dd2d1cd48bdc3b` on
  `juxtopposed-playback-bar`. The four unrelated audio experiments remain untouched.
- Selected tuple: Core `ccf72399`, GUI `4d754f60`, MP4 `df15e8ed`, subtitle demux
  `11f2b78e`, WASAPI `e5560794`, MKV `f46d8d15`. Ordinary development runtime:
  `build-windows/beta-single-frame-20261025-night/ui-fragment-r243/runtime`.
  Its isolated test profile is not a release package.
- Latest evidence: [fragment seek and caption restoration](../juxtopposed-references/native/home-fragmented-mp4-20261026/README.md).
  Companion receipts cover [edit-list held frames](../juxtopposed-references/native/home-mp4-held-frame-20261026/README.md),
  [estimator speed/defaults](../juxtopposed-references/native/home-estimator-speed-defaults-20261026/README.md),
  [rate/buffering](../juxtopposed-references/native/home-subtitle-rate-buffering-20261026/README.md),
  [clock/source refill](../juxtopposed-references/native/home-subtitle-clock-refill-20261026/README.md)
  and [snapshot/display](../juxtopposed-references/native/home-snapshot-display-20261026/README.md).
- Active outputs: `build-windows/beta-single-frame-20261025-night`.
  Inventory `inputs-r245.json` predates this documentation checkpoint; freeze
  fresh inputs before the next build or guarded test.

## What passed

- Core224 repairs delay/rate units, temporary track buffering and per-source
  external preroll, retaining paused clock recovery and saved-source refill.
  Frozen before/after evidence includes 48 clock rows, 13 clock scenarios,
  official A-B/timer tests, eleven format/delay cases and actual WASAPI resume.
- GUI230 passes 24 signed estimator rows and 12 seek/rate/pause cancellation rows;
  the 31-function software suite passes. Ordinary timed clicks at 1.5x and
  independent Reset pass. Saved-FPS/default/restart has separate older-GUI proof.
- MP4 edit-list and fragment repairs retain held frames and covering captions.
  Before regressions fail; hardware own/inherited-delay rows, 31 hardware
  functions, four startup rates and thirteen audio/rate cases pass in their
  recorded tuples. Twelve final MP4 public-API cases and the checked-in helper
  rerun pass with actual WASAPI. Startup warnings remain scoped in receipts.
- Ordinary inherited/independent delays produce pixel-identical frame301 exports.
  Final fragment seek/+5s/Reset exports are pixel-identical frame799; playback
  reaches LAST. Ordinary processes exit0/602 modules/zero diagnostics with guards.
  These distinct scopes do not establish a final matched package.

## Next action and open questions

1. Finish the documentation checkpoint and push the authorized nightly. Continue
   the beta afterward; a push does not end this session.
2. Investigate the initial paused fragmented-MP4 caption. App captures lack it
   before any seek on both prior r233 and r240; English is selected. Seeking
   restores it. Obtain native cold exports before assigning a decoder/display
   cause. Broader sparse-cue seek cost and other fragment/index layouts are open.
3. Finish the older ordinary cue-preset replay and no-secondary disabled state,
   then continue W2/W3 consumers, interaction, design/accessibility and OS DPI in
   [BETA](BETA.md) and [SETTINGS](SETTINGS.md).
4. Secondary-screen fullscreen idle capture still loses captions with hidden
   controls; older Core204 reproduces it before snapshots. Physical display
   versus capture, input/overlap, orientation/HDR and OS-DPI remain unqualified.
5. All six beta gates remain open. Online subtitle search/download needs real
   implementation and an owner API-application decision. Windows10 AUDIOSES,
   physical endpoint/device-loss and casting retain explicit external deferrals.
