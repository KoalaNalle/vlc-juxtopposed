# Paused subtitle interval repair, 23 October 2026

The selected **precise Matroska subtitle-seeking repair is accepted locally**.
The complete desktop subtitle workflow and full beta remain open.

The ordinary build keeps an index of subtitle block spans so a seek can replay
every still-active cue, including older long cues overlapped by shorter ones.
It scans block headers instead of loading video/audio payloads during indexing.
Audio/video anchors, fast-seek policy and normal full-payload playback remain.

Eight ordinary caption scenarios pass and eight unfixed controls fail. Coverage
includes English/Finnish, cold/delayed cues, blank gaps, overlapping SubRip/ASS,
backward/repeated seeks, stable paused clocks, resume and normal teardown. Full
caption masks are compared with normal-playback references where configured.
All sixteen C++ units were rebuilt under the ordinary rules with matching
headers. Core000f1159, GUI716862a1 and WASAPI4e4e remain; Matroska is nowf46d8d15.
The new runtime is `build-windows/beta-tracks-subtitles-20261023/ordinary-ui-r37/runtime`.

On the30.66MB gap fixture, header indexing reduces first-seek process reads from
27.40MiB to2.56MiB. Long active captions still require replay from their start.
These counters include probe/cache activity and are not storage/latency claims.
Normal Stop during a pending index seek passes with private and ordinary DLLs.
Disabled-subtitle and existing fast-seek behavior are preserved.

The visible ordinary player shows both captions in fullscreen, keeps the long
cue after the short one ends and on backward seek, holds pause through menu
open/Escape, resumes/Pause and quits0. **The small window still covers the bottom
caption with its controls**, and the fullscreen gradient darkens that line.
Direct accessibility progress value-setting also did not commit a seek. These
are recorded next tasks, not a full desktop acceptance. Initial/external/newly
selected subtitle workflows and all remaining beta categories stay open.

This receipt retains64 normal native jobs plus one ordinary GUI exit, failed
unfixed controls, two inconclusive initial cancellation attempts, exact producers,
build failures/corrections and frozen input manifests. Actual source snapshots
remain in the ignored build area; the final one contains2,210 inputs. That is
not a compile count. No commit/push, install, release/upload or shutdown occurred.

- [Checkpoint and artifact hashes](checkpoint.json)
- [Review, limits and next actions](review.json)
- [Native observations](matrix.json)
- [Process cost observations](cost-observations.json)
- [Ordinary build audit](ordinary-mkv-build-audit-r36c.json)
- [Ordinary native controls](native-subtitle-ordinary-r36c/comparison.json)
- [Desktop observations](desktop-subtitle-observations-r39.json)
- [Reusable test documentation](production-sources/test/libvlc/native_paused_subtitle_seek.md)
