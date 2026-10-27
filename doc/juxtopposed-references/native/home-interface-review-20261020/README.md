# Interface review — 20 October 2026

Read-only review agent assessed checkpoint `1a174fc719`, its native Interface
captures, cached original design and current consumer paths. Root then retrieved
Figma high-fidelity design context plus screenshot for original Interface node
`3062:11379` in `zdNUYRVOH9lEtIxMYvh8DV`. This is an initial scoped review, not
32-surface, package, accessibility or Windows DPI acceptance.

## Findings and disposition

- **P2, geometry reproduction pending:** `widgets/native/qvlcframe.cpp` saves the
  current client rectangle even when maximized/fullscreen, then applies it before
  restoring the saved state. Qt Windows keeps pre-state normal geometry
  separately. Test normal → maximize/fullscreen → save/cold restore → normal.
  Native platform normal geometry is in native pixels; any repair must preserve
  the logical-coordinate format and correct original screen identity.
- **P2, popup wording:** the designed `0 s` tick needs a visible qualification
  when this row is enabled. Qt Windows uses its default duration for zero;
  Windows controls notification timing. Preserve the authored control and genuine
  OS notifications; do not promise exact disappearance time.
- **Recovery coverage:** saved geometry can extend beyond the available desktop
  even when screen identity/DPR still match. The earlier 19-pixel overflow is
  preserved in historical receipts. Reproduce a bounded case and decide recovery
  behavior before accepting the full geometry policy.

## Bounded visual assessment

The reviewer inspected `home-fullscreen-screen-20261019/stale-editor.png` and
`selected-fullscreen-ui.png` against the cached original Interface frame.
Section order, sidebar, control alignment, artwork, typography hierarchy and
accent broadly follow the reference. The focused screen field is fully visible.
Unavailable-settings feedback and an unmatched saved-screen label are operational
adaptations. No additional clear visual defect was identified in these states.
The black D3D region in the fullscreen grab does not prove video-frame rendering.
Light theme, full states, actual OS DPI and screen-reader behavior remain open.

## Verification direction

Use exact production consumers and generated media with isolated profiles.
Preserve paired before/after geometry results and current playback state. For
popup duration, Qt's `qt.qpa.trayicon.debug` logging in an owned process can
establish the duration received by its genuine Windows platform handler. Keep
that evidence separate from actual OS delivery/visibility and expiration.

The installed Code Review connector exposes existing PR/CI diagnostics, not a
local review submission action. The requested review agent performed the initial
local source review. Root additionally inspected existing draft
[PR1](https://github.com/KoalaNalle/vlc-juxtopposed/pull/1) through Code Review.
All four published-head checks pass at exact head
`1a174fc7195d681df4e0d2e658e03f7cbddc9b95`: playback-ui and
qml-dependency-remake on both returned runs. This does not test current local
geometry changes. No new CI run or review publication was requested.

Annotations for latest run39303613008 retain two lower-priority maintenance
items: Node20 action runtimes being forced to Node24 (checkout@v4,
setup-python@v5, upload-artifact@v4), and migration of ubuntu-latest to Ubuntu26.
Both match current `.github/workflows/qml-ui-checks.yml`. Reconcile supported
action versions and runner reproducibility after confirmed-action work; no CI
configuration was changed in this initial review.

## Popup candidate review after geometry activation

The existing review agent found no concrete blocker in the popup gate, native
player-name route or platform-log assertions. Actual decoded input/name signals
drive the existing tray; exact notification names and startup-duration arguments
are checked separately from OS expiration. Cold zero/max, Never, minimized-only
and tray-Off cases remain pending native execution.

Component12 passes. Generated1080 capture shows readable helper text and intact
authored ticks without overlap; narrow640 shows the helper readable, with the
slider/title above the captured viewport. A focused native/narrow capture is
still needed for the complete row. P3 visual refinement: the enlarged holder
centers the "Popup duration" title below the original slider alignment; align
against the original46px group after the confirmed behavior work if appropriate.
These observations do not accept the whole Interface surface or actual OS DPI.
