# Progress accessibility seeking

`seek_accessibility_journey.cpp` is an opt-in actual-player Qt regression. Keep
its translation unit outside ordinary `libqt_plugin` sources. Build it through
`sidebar_pins_journey.mk` with `QA_SOURCE` pointing at this file and a fresh
`QA_OUT`, as for the other private native UI journeys.

Use an isolated matching Windows runtime, native configuration and portable Qt
INI. Disable library/network discovery, privacy/setup prompts and timeline
thumbnails in that owned test configuration. Use real Direct3D11 video output,
software decoding and a verified synthetic, seekable 60-second local video.
Set precise seeking on. The test starts playback until a video picture and
player statistics are published, then pauses before its seek assertions. A
player statistics counter at cold paused startup is not its readiness oracle.

Supply `VLC_OWNED_UI_REPORT` for its JSON output, `VLC_OWNED_SEEK_MEDIA` for the
synthetic video, and `VLC_OWNED_UI_START_GATE` for an initially absent inspection
marker. Inspect the actual owned app window before creating that marker.

The fixture queries the actual production SliderBar with
`QAccessible::queryAccessibleInterface` and calls its value interface. Each
changed value must trigger exactly one native player seek, display a new
picture and reconcile the timeline with the controller position while keeping
the same paused input. It checks forward/backward values, normalized accessible
bounds, an identical value producing no seek, Shift+pointer seeking, resume,
ordinary playing-position updates producing no feedback seeks, another pause,
and a final accessible seek. The native timer callback copies an atomic request
count only; it does not call player APIs or write reports from that callback.

Require all 39 checks and normal process exit with matching source, fixture and
runtime identities. Preserve the failing old-build result separately. The
parent should leave a timed-out owned process running for inspection, without
an automatic kill/restart. Keep the private QtTest-linked DLL separate from the
ordinary GUI and replay direct accessibility value-setting in the ordinary
desktop app before acceptance.

Published player time can lag a paused seek. The fixture's time fields are
observations, not exact video PTS; a displayed-picture increment and controller
position do not replace independent coarse-frame/content validation in the
ordinary app. This journey does not qualify screen-reader speech, every seek
policy/format, caption clearance, display scaling or whole-beta readiness.
