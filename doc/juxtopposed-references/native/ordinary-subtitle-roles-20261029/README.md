# Ordinary simultaneous subtitle roles — 29 October 2026

The ordinary Qt7c7 GUI, Core4a, WebVTT38 and TTML9d run together in R19.
The generated marker video has embedded English/Finnish tracks; an external
three-cue SRT is selected as primary. The visible Select multiple command adds
Finnish as secondary. Exact source, runtime and fixture identities are recorded.

Observed through the normal visible controls:

- Both first cues initially render together: upper SUOMI ENSIMMÄINEN and lower
  EXTERNAL FIRST CUE, with playback paused at the generated source's 12 seconds.
  The start-time origin makes the ordinary elapsed display 00:00:00.
- Setting only secondary delay to -9 seconds changes the upper caption to SUOMI
  KESKIMMÄINEN while the external primary stays on its first cue (capture 07).
- Setting primary to -9 independently retains secondary -9; both middle captions
  render (09/10). Resetting primary to 0 preserves the explicit secondary -9
  value (11); subsequently setting secondary to 0 restores both delays (12).
- External subtitle FPS 0-to-50 changes only the external caption to its middle
  cue; embedded Finnish remains on its first cue (13/14).
- Tracks > Subtitle > Synchronize reports 50. Its FPS Reset restores both first
  captions and 0; reopening Playback settings reflects FPS 0 and both delays 0
  (15 through 18). The visible paused frame marker and elapsed display stay fixed.

Capture 08's filename records an attempted +15-second primary entry, not an
accepted setting. The compact control supports -10 through +10 seconds and
correctly restored primary 0 on commit. Its screenshot/text show primary 0 and
secondary -9. Preserve the original filename and interpret the observed values.

The normal Quit action exits the child and observer with code 0. The runtime
loads 603 modules; no captured error diagnostics, forced cleanup or preservation
failures occur. Fixtures, compiler-source inventory, selected runtime, tested
fallback, previous generated profile, helper inputs and human configuration
remain unchanged. Private profiles and raw logs are excluded from this receipt.

This is an ordinary held-frame SRT-primary/embedded-secondary journey on the
identified tuple. It complements the separately scoped existing native client's
eight role/estimator processes; it does not establish all formats, playing and
source/restart combinations, timing estimators, design/DPI or final-package gates.
The screenshots are actual generated-media app observations, not a visual score.
