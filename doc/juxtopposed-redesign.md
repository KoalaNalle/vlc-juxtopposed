# Juxtopposed Windows redesign: first implementation milestone

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
Playback settings open VLC's existing effects dialog. The fullscreen action
is available in the header and the existing fullscreen shortcut remains.

Use `--no-qt-juxtopposed-controls` to return to the customizable VLC toolbar.
The option is also available in advanced Qt interface preferences. Linux,
macOS and the library mini player retain their existing controls.

## Validation and limits

The standalone playback component is rendered and exercised with Qt Quick.
The automated check covers mouse and keyboard actions, seeks, engine value
updates, disabled next/previous buttons, long time labels, local asset loading,
and 640/1024/1920-pixel layouts with 100/150/200-percent UI scaling. QML syntax
is checked for every modified file. The GitHub workflow runs these checks.

**This milestone is not a completed port of the entire Figma concept.** The
library sidebar, redesigned queue rows, subtitle presets/timeline, new effects
sidebar, thumbnails, waveform, conversion flows and preferences remain future
work. The playback component has been tested independently; the full VLC
Windows binary still needs compilation and testing with real media, fullscreen,
track switching, DVD menus and pinned/floating controls before release.

The thicker timeline currently keeps VLC's chapter seeking behavior but uses
the simple Figma progress appearance rather than drawing chapter segments and
bookmark marks. The new bar has a fixed arrangement; custom toolbar profiles
apply to the legacy bar. The current visual implementation targets the dark
playback bar; a complete light-theme port remains future work.

## Local component checks

Install `PySide6-Essentials==6.8.3` in a development Python environment and run:

```
python modules/gui/qt/tests/test_juxtopposed_playback_bar.py --output-dir ui-previews
```

This check renders Qt offscreen and produces PNG previews. It does not require
or replace the complete VLC build. Build VLC using the existing platform build
instructions in INSTALL and extras/package/win32; the new QML and resources
are registered in both the Autotools and Meson Qt module definitions.
