# Deferred QML teardown callbacks — 17 October 2026 guest

Finite playback previously quit normally but logged seven QML TypeErrors when
queued `Qt.callLater` callbacks wrote to objects destroyed during teardown.
Six callbacks in MiniPlayer, MainDisplay and SideNavigationPane now check that
their Binding, Behavior, Transition or ListView target still exists. Live
objects retain the same deferred initialization and animation behavior.

The Qt GUI and affected embedded QML resources rebuilt with one job. All three
frozen source inputs still match after the build. A fresh ordinary runtime
combines the rebuilt GUI with the accepted live-clock core and startup launcher;
private fixture providers/probes are absent. Default redesigned and explicit
traditional finite playback each go from seven QML diagnostics to zero. Local
HLS also passes with zero diagnostics. Each run exits normally without forced
cleanup. Video frames, Direct3D 11 and WASAPI are observed for both finite runs.
See [verification.json](verification.json) and
[ordinary-runtime.json](ordinary-runtime.json).

The playback component runner also passes, including fourteen QML parses and
five width/scale variants. Two user-authorized local MKVs, sampled for three
seconds at a 60-second offset, decode video/audio through avcodec, produce a
frame, initialize Direct3D 11/WASAPI and quit normally without QML diagnostics.
See [local-media.json](local-media.json). The large source files were read in
place; filenames, media bytes and captured frames stay outside Git.

The baseline ordinary package and previous accepted runtime are preserved.
This incremental Windows 10 result accepts the reproduced delayed-callback
warnings for these journeys. It does not accept full-file playback, hardware
decoding, independent visual/DPI review, waveform/transcode diagnostics, popup
movement, Windows 11 or the complete clean beta package. The core did not change;
the earlier 18/42/9/37 regression results remain linked to their recorded tuple.

Reproduce from the E:-only guest environment with the maintained playback
component runner, a single-job `make -C win64/modules/gui/qt libqt_plugin.la`,
and a fresh portable ordinary runtime using the source/binary identities in
this receipt. Local frozen build/launch scripts and raw logs are under
`build-windows/beta-teardown-20261017`; do not substitute a stale GUI binary.
