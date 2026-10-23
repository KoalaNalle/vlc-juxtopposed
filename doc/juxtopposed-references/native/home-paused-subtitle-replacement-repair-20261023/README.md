# Paused subtitle replacement repair, 23 October 2026

Changing a selected subtitle while paused now displays the new cue and preserves
the held video picture in the tested SubRip/Matroska, ASS/Matroska and mov_text/MP4
fixtures, with WASAPI and with audio disabled. This selected repair is accepted
locally on ordinary Core `ef3e47fd`; the full beta remains active.

The player refills the new subtitle at the displayed-video timestamp, converting
its time origin for the demux seek. The decoder retains the exact paused picture
boundary after packet preroll. The second part is necessary for reordered MP4
video: the maintained regression rejected the earlier player-only candidate
despite its restored captions. That failure is retained here.

The same portable regression reproduces missing captions in six unchanged
controls. The combined private candidate then passes six cases; the ordinary
build independently passes all six, with 90 paused snapshots. All17 earlier
startup and subtitle-interval regressions pass. There are40 normal native exits
across controls/candidates/regressions and345 replacement snapshots. Three
ordinary desktop sessions show English-to-Finnish changes with a held marker
pattern and timeline, and close normally with no checked GUI errors. SubRip also
shows English restoration, explicit external loading and resume/Pause.

External files load, but automatic selection still retains the previous track;
explicitly choosing the new track works. Cold initial cues, absolute seek PTS,
stale WASAPI reported time, policy/step/concurrent-seek stress, complete caption
layout, Windows10, DPI and physical audibility retain separate gates. Desktop
observations are not pixel metrics. Native diagnostics and compiler warnings
are preserved. No install, commit/push, release/upload or shutdown occurred.

Current runtime:
`build-windows/beta-subtitle-replacement-repair-20261023/ui-frame-marker.mkv-r3/runtime`.
Tuple: GUId0ff14c6 / Coreef3e47fd / MKVf46d8d15 / WASAPI4e4e.
The earlier fade, accessible seeking and Matroska interval repairs retain their
tested scopes. All2,290 frozen source inputs were copied and verified.

- [Per-case checks and actual frame markers](comparison.json)
- [Scope and remaining beta work](review.json)
- [Reviewed visible app observations](ui-observations-r4.json)
- [Ordinary regression results](regressions-ordinary-r3/processes.json)
- [Artifact hashes](checkpoint.json)
