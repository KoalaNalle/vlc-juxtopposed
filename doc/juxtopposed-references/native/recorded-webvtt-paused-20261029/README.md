# Recorded WebVTT paused startup — 29 October 2026

Selecting an embedded caption on a paused input with a start-time offset could
leave it invisible until a seek or resume. This was reproduced in the ordinary
Windows 11/D3D11VA player and in actual LibVLC decoder snapshots. The recorded
cue itself was valid for the requested time: its MP4 sample/edit tables place it
at movie time **10.08–29.98 seconds**; native seeks show it at 12 and 25 seconds
and show no caption at 5 seconds.

The timestamp trace identifies the cause. A held decoder-preroll frame can be
slightly before the configured playback start. One failing run had normalized
video time `-39999` with a 12,000,000-microsecond start offset; successful runs had
time `1`. The refresh required a nonnegative normalized frame, so the valid
earlier source picture was rejected and its retry expired. A temporary Core
added logging only; it is not the selected runtime and its trace is not left in
production source.

The repair permits a retained preroll picture whose source time is still valid,
seeks that exact picture, and keeps the pending visible clock at the origin.
It preserves the positive-time path and the existing recording/seekability/state
checks. Its arithmetic also bounds the relative target by the start offset.

## Evidence and scope

- [Verification](verification.json) binds the builds, actual native observations,
  diagnostic points, ordinary runtimes and scope limits.
- `paused-selection-r2/` preserves the corrected native failure: selected track,
  unchanged frame/clock, zero caption pixels after the refresh window, then a
  caption after explicit seek. `paused-selection-r3/` adds timestamp traces:
  two positive-frame passes and one negative-frame failure.
- `paused-selection-r4/` has six candidate integer-start passes. All initially
  displayed the positive frame, so those results alone do not qualify the
  negative-frame repair. `paused-selection-r5/` contains three old-Core
  fractional-start **positive controls**, not intended failures.
- `paused-selection-r6/` has three candidate fractional-start passes. In the
  `12.019` case, marker **1048** is retained while caption pixels change from
  **0 to 1498**. The same recording's no-offset seek placed marker 1048 at file
  time 12, before that fractional start. This exercises the retained preroll
  picture without advancing it. Native held-time checks allow 50,000 microseconds;
  the frame-marker comparison is exact.
- [R14 ordinary failure](gui-r14/observations/) and
  [R15 ordinary repair](gui-r15/observations/) use the same generated recording,
  12-second start, Qt/WebVTT modules and copied test profile. R15 shows the actual
  caption while **Play** and visible elapsed **00:00** remain unchanged. Both
  use D3D11VA and exit 0 with 603 modules, no captured error diagnostics and all
  preservation guards passed.
- [Seven recording regressions](recording-regression.txt) pass again on this
  newer Core. [Sample tables](sample-tables.json) separate media times from movie
  edits; FFprobe's generic data classification does not replace that evidence.
- [Core build](core-build/), [source change](source.patch),
  [producers](producers/) and [copy checksums](copy-verification.json) preserve
  reproducible inputs and observations. Native dummy-window teardown's
  `window closed` diagnostic is disclosed separately from ordinary results.

Selected Core: `4a2865915e342744a5a7092171b92279b08f7b229eede231e00c3e86d88cff84`.
Source: `f85d9bfa8801096b0853c25875ae9e937454bbd3588560f2a931d133bdbde20e`.
Qt: `7c7db5cb675edc20b3366120dc55282b09d2db2dc731b7c6ed8c1815e0706624`.
The shared build tree and tested fallback are restored/preserved. The maintained
test is `test/libvlc/native_recorded_webvtt_pause.py`; its fixture-specific API
expects this generated recording, a staged runtime and an empty output directory.

The earlier R1 probe mistakenly sought beyond the start-offset playback interval;
it remains local and is not counted as a program failure. An initial receipt
collector also rejected the file logger's shorter spelling of the known window
teardown diagnostic; its partial output is preserved locally. Neither changes
the raw native results. This receipt does not establish all subtitle formats,
FPS/delay combinations, exact cue-end/recording tails, broad GPU/DPI behavior,
multi-instance stability or final-package readiness.
