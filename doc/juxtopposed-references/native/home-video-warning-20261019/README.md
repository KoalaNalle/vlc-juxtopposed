# Initial video-window warning — 19 October 2026

[Ordinary R7 replay](ordinary-video-r7.json) establishes the tested scope of
`main warning: window size missing`. Core emits it when the initial display
width or height is zero, then calls `vout_display_GetDefaultDisplaySize` before
opening the display (`src/video_output/display.c`, `vout_display_New`). No Core
or video-provider production change was made for this review.

The exact ordinary GUI is `9892032f4456…`, with the protected Core/audio/library
and repaired FLAC tuple recorded in the JSON. A generated128×128 H264 clip has
red/green/blue/yellow quadrants. Direct Computer Use observes its actual
Direct3D11 video in paused startup, playing, paused fullscreen and windowed
return. Orientation, square aspect and side bars agree with the
[decoded fixture reference](generated-reference-frame.png). The visible paused
clock remains00:00:12 through fullscreen return; this is not exact native-clock
or pixel-perfect proof.

The native log retains one initial size warning and later reports1920×1032,
1920×1080 and the return size. MP4 edit/sample/composition table diagnostics are
also retained. There are no native errors or Qt warning/critical/fatal messages;
the codec's context-reinitialization debug line is recorded separately. Normal
Ctrl+Q exit, unchanged generated media, protected binaries and source geometry
pass. No raw Computer Use screenshot is saved.

This supports accepting the initial fallback in this tested ordinary H264/
Direct3D11 case. It does not establish every provider/codec/GPU, actual OS DPI,
hardware decoding or complete final package. An attempted corner drag did not
resize the restored window and adds no smaller-window evidence.

The [earlier black-clip probe](black-probe.json) exits normally but provides no
rendered-image acceptance: its decoded reference frame is itself black. The
color source is derived from an existing owned quadrant fixture using the
already installed ffmpeg encoder. Its source/output/reference hashes accompany
the receipt. No dependencies, security settings or user media were changed.
