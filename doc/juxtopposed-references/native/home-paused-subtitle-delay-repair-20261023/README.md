# Paused subtitle delay repair, 23 October 2026

The ordinary build now retains the expected caption on a delayed paused seek
and refreshes it after a current-file primary delay change or Reset. All eleven
maintained timing cases and 24 prior startup/interval/track/frame-step/resume
regressions pass. All 35 native children exit normally; 121 timing snapshots
verify and 44 actual seek-frame comparisons match the tested private candidate.

One actual ordinary desktop session exercises ASS and mov_text/MP4. Delayed
Finnish cues appear after paused seeking; Reset shows the middle cue without
another user seek. Twelve real keyboard nudges through 50–600 ms restore the
earlier ASS cue while retaining the visible held picture. Opening MP4 in the
same instance reapplies the saved 1.2-second default after the previous file's
Reset. MP4 Reset, resumed caption-bearing playback and direct playing Quit pass;
the session exits zero without checked GUI diagnostics. Saved default remains
`sub-delay=12` (100 ms units).

The production sources match the tested seven-file candidate. Ordinary Core
`7ab80750` is built through the regular build with no private replacement objects
or headers. Current runtime is
`build-windows/beta-subtitle-delay-repair-20261023/ui-frame-marker-ass.mkv-r4/runtime`.
Source copies, compiler dependencies, native images/results and desktop review
are retained. The immutable earlier investigation preserves the failures and
private candidate provenance separately.

This accepts the selected timing repair, not the complete subtitle surface or
the beta. The spinner's direct accessibility value write did not change its
value; keyboard adjustment worked. Exact GUI numeric progress conservation,
complete focus/keyboard policies, secondary timing, broader stream/lifecycle
cases, design/accessibility/DPI, real-media consolidation, independent review
and clean packaging remain open. See `review.json` for the precise limits.
