# External subtitle selection repair, 23 October 2026

Opening an external subtitle while paused now selects its preferred track as
the sole subtitle, displays its cue and retains the held picture in the tested
ordinary build. This repair is accepted locally; the full beta remains active.

The explicit request uses VLC's existing exclusive user-selection route.
Subtitle source identity is established before opening its demuxer, and that
source's PCR/reset/display-target controls no longer drive the main media clock.
This also fixes the held-picture/time loss exposed by a two-track MKS file.
Generic associated sources keep their prior behavior.

The maintained regression uses the same generated fixtures, worker and options
before and after: 15 behavior failures and two load-only passes on the earlier
ordinary Core, then all17 passes on ordinary Core `ea14300c`. The earlier17
startup/interval regressions also pass. All51 native children exit normally.
Three actual desktop sessions open SubRip, ASS and two-track MKS through the
subtitle file dialog, verify automatic/sole selection and retained picture/
elapsed text, resume, pause and close normally without checked GUI errors.
The first SubRip accessible seek's normalized value reconciles slightly on
track refresh; exact GUI slider-value conservation is not claimed.
A transient Next frame OSD on some ASS/MKS subtitle refreshes is a reproduced
feedback defect and remains open; no step key was sent and the picture stayed.

Runtime: `build-windows/beta-external-subtitle-acceptance-20261023/ui-external.srt-r3/runtime`.
Tuple: Coreea14300c / GUId0ff14c6 / MKVf46d8d15 / WASAPI4e4e.
Ordinary Core recompiles matching production sources and header dependents;
no private replacement objects or GUI hooks are used. The earlier candidate
failures remain in their immutable investigation receipt. Frozen source input
copies and all exported artifact hashes are verified before publication.

This qualifies paused runtime addition, not all subtitle paths. Playing,
pending/nonseekable/recording, repeated additions/removal/restart, multi-select,
cold captions, defaults/language/sync, exact layout, physical audibility,
Windows10, DPI, independent review and clean packaging retain separate gates.
Native missing-file/dummy-window diagnostics and compiler warnings are retained.
No install, commit/push, release/upload or shutdown occurred.

- [Paired results, checks and markers](comparison.json)
- [Reviewed desktop observations](ui-observations-r4.json)
- [Source scope and remaining work](review.json)
- [Ordinary broader regressions](regressions-ordinary-r3/processes.json)
- [Artifact hashes](checkpoint.json)
