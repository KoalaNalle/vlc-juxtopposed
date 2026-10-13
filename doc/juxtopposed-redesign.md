# Juxtopposed Windows redesign: playback controls and functional integration

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

Use `--no-qt-juxtopposed-controls` to return to the customizable VLC toolbar.
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
QML syntax is checked for every modified file. The GitHub workflow runs these
checks; the new native C++ context-menu mapping still needs the full VLC build.

**This milestone is not a completed port of the entire Figma concept.** The
library sidebar, redesigned queue rows, subtitle presets/timeline, final effects
sidebar, thumbnails, waveform, conversion flows and preferences remain future
work. The Qt components have been tested independently; the full VLC
Windows binary still needs compilation and testing with real media, fullscreen,
track switching, DVD menus and pinned/floating controls before release. See
the [home Windows build checklist](juxtopposed-windows-build-checklist.md).

The Figma MCP account reached its tool-call limit while inspecting the next
screens. No new sidebar appearance was inferred from sparse metadata. A paid
Figma plan is not required to continue developing VLC. Detailed layouts and
original assets can also be inspected and exported manually in Design Mode;
see [Figma's inspection guide](https://help.figma.com/hc/en-us/articles/22012921621015-Guide-to-inspecting).

The thicker timeline currently keeps VLC's chapter seeking behavior but uses
the simple Figma progress appearance rather than drawing chapter segments and
bookmark marks. The new bar has a fixed arrangement; custom toolbar profiles
apply to the legacy bar. The current visual implementation targets the dark
playback bar; a complete light-theme port remains future work.

## Local component checks

Install `PySide6-Essentials==6.8.3` in a development Python environment and run:

```
python modules/gui/qt/tests/test_juxtopposed_playback_bar.py --output-dir ui-previews
python modules/gui/qt/tests/test_native_seek_handle.py
python modules/gui/qt/tests/test_playback_settings_choices.py
python modules/gui/qt/tests/test_playlist_proxy_mapping.py
```

This check renders Qt offscreen and produces PNG previews. It does not require
or replace the complete VLC build. Build VLC using the existing platform build
instructions in INSTALL and extras/package/win32; the new QML and resources
are registered in both the Autotools and Meson Qt module definitions.
