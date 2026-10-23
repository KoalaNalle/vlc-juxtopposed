# Subtitle frame-step resume repair, 23 October 2026

The ordinary build now retains external captions after two genuine frame steps
and playback resume in all seven maintained cases. Internal paused refreshes no
longer emit unsolicited Next frame status, while real requests still return two
successes and advance the actual held picture by one each. This repair is
accepted locally within the tested scope; the full beta remains active.

Four matched controls distinguish the failure: with the unchanged Core, both
paused holds retain captions and both stepped runs lose them after resume,
with and without WASAPI. Stepping discarded subtitle packets, and resume then
flushed already-demuxed cues. The fix retains those packets and skips subtitle
flush only for the existing retained-video frame-step-resume path. Actual seeks
continue to clear subtitle decoders, and audio resynchronization remains.
The decoder separately tracks the internal flush picture so its completion is
not reported as a user request. This changes no public structure or callback ABI.

All seven combined private cases and the same seven ordinary cases pass with
the maintained assertions intact: SubRip/Matroska, ASS/Matroska and mov_text/MP4,
with and without audio, plus a two-track external MKS source. All17 earlier
startup/interval regressions also pass. All35 native children exit normally;
306 paired control/selection/step snapshots verify. Native diagnostics and
compiler warnings remain recorded.

Two actual desktop sessions open external SRT/MKS through the subtitle file
dialog, verify sole automatic selection, make two genuine keyboard steps,
resume with the cue visible, and seek forward/backward to the expected cues.
MKS also checks Finnish selection and English restoration. An additional
deliberate step after the MKS seek checks visibly shows Next frame; a 250-ms
follow-up capture catches that transient feedback. Both sessions quit normally
without checked GUI errors. Desktop observations are separate from native
pixel-based frame checks and do not establish absolute seek PTS.

Current runtime:
`build-windows/beta-subtitle-frame-resume-20261023/ui-external.srt-r4/runtime`.
Tuple: Core0ef9dde2 / GUId0ff14c6 / MKVf46d8d15 / WASAPI4e4e.
Ordinary Core recompiles matching decoder/ES-output production sources with no
private objects or test hooks. The2,372 actual frozen input copies verify.
The earlier decoder-only candidate remains rejected in its immutable receipt;
this combined repair supersedes that private-only direction.

Long stepping across cue boundaries, previous-frame transitions, broader
subtitle policies, full caption layout and original preview-hover attribution
remain open. Continue all13 categories/32 surfaces, design/accessibility/DPI,
consolidated real-media checks, physical/platform limits, independent review
and clean packaging. No install, commit/push, release/upload or shutdown.

- [Paired results](comparison.json)
- [Actual desktop observations](ui-observations-r5.json)
- [Scope and limits](review.json)
- [Ordinary broader regressions](regressions-ordinary-r4/processes.json)
- [Artifact hashes](checkpoint.json)
