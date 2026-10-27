# Caption bottom fade repair, 23 October 2026

The redesigned player no longer draws VLC's old bottom fade above its opaque
playback bar. The former fade darkened captions even when they were clear of
the controls. A single visibility guard scopes the change to the redesign.
Legacy controls keep their existing gradient; video bounds, saved subtitle
offsets, authored ASS positions and the top fade stay unchanged.

The unchanged ordinary build reproduces the dark lower line at 31 seconds.
The repaired ordinary build shows both overlapping SubRip and ASS captions
brightly in fullscreen. Backward SubRip seeking, paused menu open/Escape and
resume/Pause work. The legacy comparison retains its old fade. Four ordinary
GUI sessions exit normally with no checked runtime errors or byte drift.
These are reviewed desktop observations, not automated image metrics.

Read-only Figma context and screenshots for Player 247:445 and hidden controls
3328:11554 confirm media behind floating controls. Original design/artwork is
by Juxtopposed. Reference screenshots are not application assets. The design
does not settle caption clearance when controls cover a line in a small window;
that preference is saved for morning, with no subtitle-position change here.

Current runtime: `build-windows/beta-caption-layout-20261023/ui-after-r1/runtime`.
Tuple: GUId0ff14c6 / Core000f1159 / MKVf46d8d15 / WASAPI4e4e.
The before/after freezes each contain 2,242 actual input copies, not compiled
translation-unit counts. Only Player.qml differs between the two freezes.
Existing native subtitle seeking and accessible progress repair receipts retain
their original scopes. Full caption layout, accessibility/DPI, remaining track
flows, Windows 10, independent review and the full beta remain open.

- [Checkpoint and artifact hashes](checkpoint.json)
- [Reviewed desktop observations](desktop-observations.json)
- [Scope and remaining work](review.json)
- [Saved morning question](morning-question.md)
