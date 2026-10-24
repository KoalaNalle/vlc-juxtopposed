# Paused video startup repair, 23 October 2026

A selected video track can now deliver its first picture while playback stays
paused, even when audio fills the initial buffer before any video packet arrives.
The decoder uses its first-picture state independently of user frame-step counts.
Audio clocks, subtitle demux/selection, and the GUI are unchanged by this repair.

The timestamp-preserving remux succeeds with identical A/V payloads and PTS;
the earlier21ms shift was not a sufficient explanation. A separate diagnostic
Core shows the original file pausing with no video packet queued, while the
matching remux receives video before that pause. The diagnostic DLL is not the
ordinary acceptance Core.

Ordinary Core000f1159 with GUI716862a1 passes nine native startup controls,
including real WASAPI, dummy/no audio, disabled subtitles, plain/remuxed video,
audio-only, a single-picture file, and delayed video. Four old video failures
remain recorded. The same corrected probe also checks selected video tracks.
Existing real-controller audio7, bookmark/chapter9, and paused video6 checks pass.
The visible app opens with its video surface, remains paused during track-menu
open/Escape, resumes/Pause, and quits normally. Desktop subtitle text remains
absent in these observations; that workflow is not accepted.

The earlier paused subtitle failures remain: backward seeking and switching or
loading a subtitle while paused miss cues; resume supplies the external cue in
native probes. Post-start subtitle pixel-presence patterns match the earlier
controls. The old buffer warning remains, as do documented native teardown and
build/parser/cache warnings. This is selected startup acceptance, not a clean-log,
all-format, Windows10, physical-output, design/DPI or full-beta acceptance.

R10's frame-count candidate, R11's uncompiled malformed generated source, and
R12's incorrect no-vout audio-only assumption are retained and explained in the
review. R13 corrects that assumption using selected tracks. No scripts or old
receipts were rewritten to turn a failure into a pass. Only snapshots available
while paused qualify; files delivered after resume remain excluded.

- [Checkpoint and artifact hashes](checkpoint.json)
- [Review, corrections and remaining work](review.json)
- [Ordinary nine-control comparison](native-startup-ordinary-r14/comparison.json)
- [Corrected paired probe](native-startup-edges-r13/comparison.json)
- [Original paired edge cases](native-startup-edges-r12/comparison.json)
- [Private decoder trace](native-core-trace-r9c/processes.json)
- [Timestamp controls](timestamp-control-inputs-r8b.json)
- [Ordinary UI observations](ui-observations.json)
- [Native audio replay](native-audio-r15/verification.json)
- [Native bookmark replay](native-bookmarks-r15/verification.json)
- [Native video replay](native-video-r15/process.json)
- [Current source inputs](inputs-r14.json)

The source freeze holds2147 actual copies, including headers and producers.
Historical receipts retain their own binary scopes. Full13-category/32-surface
beta work continues; no installs, commit/push, release/upload or shutdown.
Nonurgent questions stay for morning; none is required for the next trace.
