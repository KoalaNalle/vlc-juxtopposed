# Current status

Updated 27 October 2026. Beta work is **active**, with the desktop available.
Checkpoint and stop owned jobs by **27 October 2026, 08:00 Helsinki** (06:00 UTC).
The user authorized tested nightly commits/pushes; release publication is separate.

## Selected checkpoint

- Nightly source: `c2ac863b46c44a092d004e69de7b2ecd82fc0cc7` on
  `juxtopposed-playback-bar`. The four unrelated audio experiments remain untouched.
- Selected tuple: Core `fdfcfcd5`, ordinary GUI `8e92470c`, MP4 `df15e8ed`,
  subtitle demux `11f2b78e`, WASAPI `e5560794`, MKV `f46d8d15`.
  Ordinary runtime:
  `build-windows/beta-single-frame-20261025-night/ui-selection-fixed-r265/runtime`.
  Its isolated profile is not a release package.
- Latest evidence: [selection and first-frame clock](../juxtopposed-references/native/home-subtitle-selection-clock-20261027/README.md).
  [Initial paused caption](../juxtopposed-references/native/home-cold-paused-caption-20261027/README.md)
  and [fragment seeking](../juxtopposed-references/native/home-fragmented-mp4-20261026/README.md)
  retain their distinct earlier scopes.
- Active outputs: `build-windows/beta-single-frame-20261025-night`.
  Inventory `inputs-r267.json` predates this documentation checkpoint; freeze
  fresh inputs before the next guarded test. All owned tests have exited normally.

## What passed

- The prior nightly retains repaired delay/rate units, transient buffering,
  per-source external preroll, estimator scaling and MP4 edit/fragment seeking.
- Core265 retains video timestamps before frame-rate metadata is known, while
  withholding frame-number callbacks until it is known. Sixteen source-level
  cases fail the previous implementation and pass the repair.
- Secondary controls reflect native availability in both editors; Tab skips
  unavailable timing. Native same-object marks cancel on selection changes
  without seeking. The same-binary previous-hook control fails; current hooks pass.
- At the opening paused 80 ms frame, both actual subtitles activate without a
  user seek. Secondary +333 ms suppresses only Finnish; Reset restores a
  pixel-identical dual-caption export, with encoded frame0 throughout. Ordinary
  playback reaches both LAST cues, repauses and exits0/602 modules/zero errors.
- Both 31-function software/hardware suites, actual-WASAPI own/inherited delay
  rows, thirteen startup controls, twelve MP4 cases and official A-B/timer
  programs pass. The software suite retains one late-picture warning; hardware
  and WASAPI delay runs have none, with zero avcodec-late errors. All guards pass.
- The animation targets the existing background color property. Earlier loader
  and image-band fixture failures remain recorded separately. Native exports
  place captions differently from the ordinary display; no broader visual claim.

## Next action and open questions

1. Finish the older ordinary late-cue preset/default/FPS/boundary/A-B-A/cold replay
   on this tuple. Retained generated profile: `ui-cue-r118`; media:
   `attachment-media-r85` and `late-media-r104`. Use the fresh inventory with
   `run-ui-late-r265.py`, attachment mode, fixed redesigned controls and rate1.5.
   Confirm actual FPS and selected track rather than inferring them from a name.
2. Continue W2 consumers and W3 interaction/design/accessibility/Windows DPI in
   [BETA](BETA.md) and [SETTINGS](SETTINGS.md). All six beta gates remain open.
3. Secondary-screen fullscreen idle capture still loses captions with hidden
   controls; older Core204 reproduces it before snapshots. Physical display
   versus capture, overlap, orientation/HDR and OS-DPI remain unqualified.
4. Sparse/long-cue seek cost, other MP4 index layouts and live behavior remain
   open. The earlier cold RGB assertion remains failed; only the new dual-caption
   before/Reset pair has exact pixel equality.
5. Online subtitle search/download needs real implementation and an owner API
   application decision. Windows10 AUDIOSES and physical endpoint/device-loss/
   receiver/casting checks retain explicit external deferrals.
