# Movable modal headers — 17 October 2026 guest

Open Network Stream, New Playlist, Conversion and Global Settings now share a
movable Dialog base and header pointer area. The earlier permanent center
anchor made dragging ineffective. A fresh open is still centered; a visible
dialog can move by its noninteractive header. Search and close controls retain
their own pointer handling. Coordinates stay bound and clamped to the owning
window, including after resize/scale changes, keeping the whole modal reachable.
These remain item popups owned by the main window; cross-window native floating
and desktop multi-monitor behavior are not accepted by this checkpoint.

The same maintained production-host component test fails the original Network
header movement case and passes **96 assertions** after repair, with no Qt
messages and completed engine destruction. It sends pointer events to each real
header in all four dialogs at 1/1.5/2 component scale, checks movement on both
axes, focus/modal guards, both bounds, parent shrink, clickable close controls,
focus return and centered reopen. Existing 12 directed modal transitions and
nine busy-operation guards also pass. Controllers are strict supplied contracts;
this is not a native VLC desktop pointer or Windows DPI test. See
[original result](component-original.json) and [component.json](component.json).

Both build systems register the new QML types. The one-job Autotools GUI/resource
build succeeds; all 779 frozen Qt source/registration/test inputs remain unchanged.
Meson registration syntax parses; a native Meson build was not run. A separate
ordinary runtime passes default redesigned and explicit traditional video/audio,
Direct3D 11/WASAPI/decoded frames, local adaptive HLS and normal quit, with zero
QML diagnostics. The baseline package and preceding runtimes are preserved.
See [ordinary-runtime.json](ordinary-runtime.json) and
[verification.json](verification.json). The four focused Network/New Playlist/
Conversion/Global Settings component suites also pass:
[component-regressions.json](component-regressions.json).

Two user-authorized local MKVs were sampled for three seconds at a 60-second
offset. Both use D3D11VA on the GTX 1080, decode audio/video, produce a frame and
quit normally with no QML diagnostics. [hardware-media.json](hardware-media.json)
retains 16/13 late-frame warnings: these short startup/seek/capture journeys do
not establish smooth playback or sustained performance. Media filenames, source
bytes and captured frames remain private under ignored build work.

Reproduce the maintained pointer/component checks with Qt/PySide6 6.8.3:

```powershell
python modules/gui/qt/tests/test_juxtopposed_dialogs_host.py --output-dir <fresh-output>
```

Run the four focused `test_juxtopposed_{network_dialog,new_playlist,conversion_dialog,global_settings}.py`
runners separately with fresh `--output-dir` paths. Frozen guest build/launch
scripts, original-source snapshot and raw logs are under
`build-windows/beta-popup-20261017`. An intermediate harness sent pointer events
outside the target window and failed its warning ledger; it is not accepted.
The recorded pair uses in-window pointer endpoints and an identical test script.

Native desktop pointer/focus, actual 100/150/200% Windows DPI, multiple monitors,
independent visual review and final clean beta/Windows 11 gates remain open.
Waveform/transcode/PCR diagnostics are a separate unresolved issue. No core
change was made here; the prior native 18/42/9/37 results retain their own tuple.
