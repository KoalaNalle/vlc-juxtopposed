# Paused subtitle seek investigation, 23 October 2026

This investigation is complete; its candidate is **not accepted or promoted**.
The ordinary Core000f1159/GUI716862a1/MKV469751a1 tuple remains unchanged.

Active long cues can precede the video-led seek position and are then not read.
Packet locations, decoder trace and ordinary follow-up agree in eight controls.
A private Matroska candidate re-reads the selected subtitle's last known block
while retaining the audio/video time and skip anchors. Original/remux backward
seeks improve, and cold English/Finnish, delayed-first-cue and blank-gap tests
pass with the expected coarse video colors and frozen paused clock.

The overlap test prevents promotion. A long0–59.9s cue overlaps shorter10–12s
and30–32s cues. The candidate loses the long cue at15s and35s after using a newer
short cue as its replay point. Normal playback from the beginning preserves
the long cue after15s. At31s only the short cue appears: a raw presence-only
row says passed, but the review rejects that as complete-caption acceptance.

All17 native jobs close normally, with120 qualified pictures and retained
errors/warnings. This includes six initial comparisons, eight colored controls,
two overlap controls and one normal-playback reference. No exact mask, video
PTS, interaction latency, scanning-cost or whole-workflow acceptance is claimed.
The2,189-input freeze contains actual source copies, headers and producers;
it is not a translation-unit compile count. Only the Matroska segment unit is
replaced in each private plugin; other objects and build flags are shared.

Next, cue selection must account for all still-active spans, with appropriate
index coverage and measured scanning cost. Any layout change requires rebuilding
all affected C++ objects together. Track replacement, associated subtitles,
desktop/initial display and the full beta roadmap remain open. No new question
requires the sleeping user's input. No production Matroska edit, install,
commit/push, release/upload or shutdown occurred.

- [Checkpoint and artifact hashes](checkpoint.json)
- [Review, rejected conclusions and next actions](review.json)
- [Collected observations](matrix.json)
- [Packet/seek position analysis](subtitle-seek-analysis-r17/analysis.json)
- [Matched private plugin controls](native-mkv-seek-r19/comparison.json)
- [Cold/language/gap controls](native-mkv-edges-r20/comparison.json)
- [Overlap failures](native-mkv-overlap-r21/comparison.json)
- [Normal-playback reference](native-overlap-reference-r22/comparison.json)
