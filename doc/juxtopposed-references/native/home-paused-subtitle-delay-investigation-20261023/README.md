# Paused subtitle delay investigation, 23 October 2026

The ordinary app saves and reopens the selected Finnish language, default
subtitle index zero and +1.2-second default delay. Changing defaults preserves
current playback; the next file uses the saved language/delay or explicit index.
Two actual desktop sessions close normally without checked GUI diagnostics.

The same app loses a delayed cue after seeking while paused, and changing the
live delay does not refresh it. Native controls reproduce both failures across
SubRip, ASS and mov_text, with WASAPI and in a no-audio case. Playback across
the boundary behaves correctly. The decoder can sleep through a delay update;
demux/packet preroll also uses video time without retaining earlier delayed cues.

A private seven-source candidate wakes delay consumers, obtains earlier cue
data while retaining the requested display boundary and refills seekable paused
subtitles at the actual held video timestamp. All eleven targeted candidate
cases pass, including the progress-bar position route and a negative offset;
all 24 prior startup/interval/track/frame-resume regressions pass. All 44 paired
seek-frame comparisons (32 time-route and 12 position-route) match the unchanged build. The 48 native processes
close normally; 13 before cases retain their behavioral failures. There are
264 timing snapshots, in addition to the legacy regression evidence.

This is an investigation checkpoint. Production Core sources remain unchanged;
the next gate is an ordinary rebuild plus the actual desktop timing workflow.
The complete thirteen-category/32-surface beta remains active. See `review.json`
for limits, `comparison.json` for per-case checks and the preserved raw logs,
source preimages, candidate patch and compiler dependency records for provenance.
