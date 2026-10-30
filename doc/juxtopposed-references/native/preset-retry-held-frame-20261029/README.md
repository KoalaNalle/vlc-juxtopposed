# Preset retry and exact held-picture repair — 29 October 2026

Ordinary R25 on Core4a/Qt7c7 demonstrates a real Windows INI replacement failure.
The generated profile is held with read/write sharing but no delete sharing.
Save Preset reports a useful error, preserves the name/form and exact disk hash,
and does not invent a selected saved entry. Releasing that handle and clicking
the same Save button succeeds, clears the error and stores exactly one named
preset with 1,200,000-us timing. A new reader verifies its durable record.

Applying Default then exposed a separate paused-picture defect. The generated
MP4 starts at 20.5 s and initially holds source frame 512 (20.48 s), just before
the visible start origin. Direct delay editing and failed/successful storage
keep frame 512; Default's batched timing reset advances it to 513 while the
elapsed display remains zero. Stored capture analysis decodes the actual drawn
marker independently of that elapsed display. Caption changes themselves work.

The maintained Core consumer test adds startup-single/startup-batch cases,
using real media, decoders and vout PNG snapshots. Before R3 on Core4a reproduces
512→513 for the batched 20.5-s case; the single reset and frame-aligned controls
preserve their own initial frame. Startup's retained frame can vary with callback
ordering, so each oracle compares its actual initial picture, not a forced frame
number. A later user seek changes the frame and a subsequent redraw retains that
new target. All four child processes exit 0; the observer records the intended
held-frame failure. Initial R1 omitted test_init's plugin-path environment and
failed before playback; that setup failure is retained and is unqualified.

The repair caches the exact media-relative target while its own paused redraw
seek is pending. The visible seek origin may remain zero for preroll. Explicit
time/position/chapter seeks invalidate the cached target centrally; resume,
stop, recording and new-input lifetimes also clear it. The field is appended to
the private input structure. The normal two-worker Core build covers 21 private
header consumers and 359 actual dependency paths, with its full shared-src tree,
inputs, source and tested fallback restored/unchanged. Core candidate is ccb6a2.

The unchanged extended native test executable passes all four after cases:
held-frame preservation, fractional/aligned starts and later user-seek precedence.
Its exact test source, compiler inputs, commands and PNGs are included. The sole
native diagnostic per child is the known test-window-close teardown message;
it is classified separately from ordinary GUI diagnostics.

Ordinary R27 uses Coreccb6 with Qt7c7/WebVTT38/TTML9d. The saved preset survives
a fresh process and is manually applied to the newly copied generated media.
It renders the first Finnish cue at live delay 1.200 s; Default restores the
middle cue and delay 0.000 s. Both actions preserve drawn frame 512 and paused
elapsed zero. This proves explicit saved-preset application, not automatic
file-association matching across changed fixture paths. R25 and R27 each exit
child/observer 0, load 603 modules, emit no captured error diagnostics, and pass
all source/runtime/fixture/fallback/profile/input/human-config guards.

Caption refresh is asynchronous: R25 capture 02 is the transitional old caption;
capture 03 records the settled first cue. Capture filenames are not assertions.
No private profiles, media binaries or raw personal logs are included. These
named storage/native/ordinary scopes do not close all formats, role/estimator,
recording, visual/DPI, multi-instance or final-package gates. New-Core regression
results must be recorded separately rather than inherited from Core4a receipts.
