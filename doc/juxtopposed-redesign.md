# Juxtopposed Windows redesign: playback controls and play queue

The original interface design is by **[Juxtopposed](https://www.youtube.com/@juxtopposed)**.
All visual design credit belongs to them. The source design is the
[VLC Redesign on Figma](https://www.figma.com/community/file/1691465999134370980/vlc-redesign),
shared under CC BY 4.0. This is an independent implementation of that design.

## Implemented

The Windows video/audio playback bar uses the original rectangular transport
buttons, a 28-pixel seek bar, time labels, cone volume control, separate audio
and subtitle entry points, playback settings and a queue toggle. At normal
widths it occupies one 54-pixel row. Below 860 logical pixels, the timeline
moves to a separate row. The icons and Inter font are packaged locally.

The integration uses VLC's playlist controller and its existing SliderBar,
including chapter snapping, keyboard seeking and tooltip handling. Track
buttons open VLC's existing track picker and focus the requested column.
Playback settings open a native menu with playback speed, aspect ratio, crop,
stereo mode and audio mix choices, plus access to VLC's effects and track
synchronization dialogs. Engine updates refresh the choices without applying
edits. Unavailable options disable, and keyboard focus scrolls into view.
This menu is an interim implementation, before the Figma settings sidebar.

The fullscreen action is available in the header, including pinned fullscreen
controls, and the existing fullscreen shortcut remains. Left/right arrows
navigate the playback buttons; timeline and volume arrows retain their own
actions. Seeking exposes the slider accessibility role and hides VLC's legacy
large hover handle behind the redesigned progress appearance.

Filtered queues map displayed rows to the underlying playlist for keyboard and
touch playback, removal, dragging and context-menu stream/save actions. Row
drops retain their source positions; footer and queue-toggle drops append to
the complete queue, including when no items match the filter. Current-item
scrolling and drag artwork map source rows back to the displayed list.

The Windows play queue has the original Queue, Bookmarks and Chapters tabs,
Inter typography, compact rows and original SVG action icons. Queue actions use
VLC's existing list, selection and playlist controller. Bookmarks use the
media-library bookmark model for adding, removing and seeking; adding stays
disabled until the current file has a resolved library ID and is seekable.
Chapters use the existing chapter model and native chapter selection. Without
the media library, the Bookmarks tab explains its availability and disables
its actions. The overlay follows the reference's 400-pixel width and margins,
supports resizing, and keeps the existing queue view when styles change.

Use `--no-qt-juxtopposed-controls` to return to the customizable VLC toolbar and
traditional queue appearance.
The option is also available in advanced Qt interface preferences. Linux,
macOS and the library mini player retain their existing controls.

## Validation and limits

The standalone playback component is rendered and exercised with Qt Quick.
The automated checks cover mouse and keyboard actions, seeks, engine value
updates, disabled next/previous buttons, long time labels, local asset loading,
and 640/1024/1920-pixel layouts with 100/150/200-percent UI scaling. Additional
checks execute the real seek-handle states, settings choice component and
queue callbacks against Qt's native models. They cover selection/reset updates,
filtered activation, removal, reordering, empty filters and append positions.
Queue checks render all three tabs, exercise actions, verify the retained view
across style changes, and check the production overlay anchors with Qt. QML
syntax is checked for every modified file. The GitHub workflow runs these
checks; native C++ context-menu and bookmark changes still need the full VLC
build and real media.

**This milestone is not a completed port of the entire Figma concept.** The
library sidebar, subtitle presets/timeline, final effects
sidebar, thumbnails, waveform, conversion flows and preferences remain future
work. The Windows binary now compiles and its Qt GUI loads. Initial native
video/control checks work, but paused seeking has a confirmed clock/reset fault
and full runtime acceptance remains open. Fullscreen, track switching, DVD
menus and pinned/floating controls still need native validation before release. See
the [home Windows build checklist](juxtopposed-windows-build-checklist.md).

The [vertical slice implementation plan](juxtopposed-vertical-slice-plan.md)
defines the next reference audit, a connected path through each design area,
implementation milestones and the proposed overnight work cycle. Detailed
home-machine build/runtime evidence is in [PROJECT_HANDOFF.md](../PROJECT_HANDOFF.md).

The Figma MCP account reached its tool-call limit. The queue tabs were inspected
and exported manually in Design Mode with the user's authorization. Original
references, measurements and asset provenance are preserved in the
[reference manifest](juxtopposed-references/queue/manifest.json).
A paid Figma plan is not required to continue developing VLC. Detailed layouts
and original assets can also be inspected and exported manually in Design Mode;
see [Figma's inspection guide](https://help.figma.com/hc/en-us/articles/22012921621015-Guide-to-inspecting).

The thicker timeline currently keeps VLC's chapter seeking behavior but uses
the simple Figma progress appearance rather than drawing chapter segments and
bookmark marks. The new bar has a fixed arrangement; custom toolbar profiles
apply to the legacy bar. The current visual implementation targets the dark
playback bar; a complete light-theme port remains future work.

The queue preserves the reference's 95-percent body fill. Its 50-pixel
background blur is not implemented. The Windows software offscreen renderer
flattens alpha when capturing previews, so native transparency and compositing
must be evaluated separately. Hover, selection, focus and current-item feedback
use VLC's existing theme; the neutral Figma frames do not specify those states.
Active repeat/shuffle buttons add an accent outline and tint, with descriptions
that distinguish repeat off/all/current and shuffle on/off. This feedback is an
implementation adaptation; it has no matching active-state Figma reference.

## Implementation and independent review loop

1. Preserve a named original reference, its Figma node ID, dimensions, font,
   colors, spacing and original assets before implementing a surface.
2. An implementation agent builds the presentation component and binds it to
   VLC's native models. Render fixed content at the reference dimensions.
3. A separate visual reviewer compares reference and render, using aligned
   overlays and measured geometry. A separate functional reviewer checks native
   actions, selection, keyboard/touch behavior, lifecycle and optional features.
4. Fix the ranked findings, regenerate the render and have the reviewers check
   the corrections. Integrate the shared resources and run the relevant checks.
5. Accept a canonical visual capture at 9.5/10 and assessed variants at 9/10,
   with no critical visual defects. Functional acceptance requires its own
   checks; a visual score alone does not establish production readiness.

The visual rubric weights layout 35%, typography 20%, surfaces 15%, artwork
15%, and state/responsive fidelity 15%. Record each assessed category and each
unassessed state explicitly. A score normalized over the assessed categories
must not be presented as a score for the entire interface. Native Windows
rendering and playback remain open gates until tested in the full application.

The independent canonical queue review passed all three 400 × 946 captures:

| Canonical state | Score over assessed 85% | Result |
| --- | --- | --- |
| Queue | 9.64/10 | Pass |
| Bookmarks | 9.61/10 | Pass |
| Chapters | 9.70/10 | Pass |

These are standalone Qt component captures. State/responsive fidelity is
excluded from the score, and background blur/native compositing remain open.
See the [three-tab preview](juxtopposed-references/queue/review/queue-final-visual-review-production-contact-sheet.png),
[independent visual review](juxtopposed-references/queue/review/queue-final-visual-review.json),
[bookmark backend review](juxtopposed-references/queue/review/bookmark-backend-review-final.json),
and [native adapter review](juxtopposed-references/queue/review/queue-native-adapter-review-final.json).
The review records the original/render hashes and normalized source Git blob
IDs. Small text raster/advance differences of 1–3 pixels remain.

## Local component checks

Install `PySide6-Essentials==6.8.3` in a development Python environment and run:

```
python modules/gui/qt/tests/test_juxtopposed_playback_bar.py --output-dir ui-previews
python modules/gui/qt/tests/test_native_seek_handle.py
python modules/gui/qt/tests/test_playback_settings_choices.py
python modules/gui/qt/tests/test_playlist_proxy_mapping.py
python modules/gui/qt/tests/test_juxtopposed_queue.py --output-dir ui-previews/queue
python modules/gui/qt/tests/test_juxtopposed_queue_overlay.py
```

This check renders Qt offscreen and produces PNG previews. It does not require
or replace the complete VLC build. Build VLC using the existing platform build
instructions in INSTALL and extras/package/win32; the new QML and resources
are registered in both the Autotools and Meson Qt module definitions.
