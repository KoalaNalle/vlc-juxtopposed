# Network stream dialog checkpoint

14 October 2026. The confirmed [13:12 reference](0792-network-dialog.png) now has a compiled Open Network Stream dialog with real queue, playback and saved-playlist actions. Original design: [Juxtopposed](https://www.youtube.com/@juxtopposed).

## Working behavior

The redesigned Windows interface routes Open Network Stream and Ctrl+N through the new QML dialog. The traditional interface and other platforms retain their existing provider. The dialog accepts one absolute network URL per line, preserves order and duplicates, validates missing hosts/local paths and limits input to 512 URLs and 65,536 characters. Validation does not contact the sources. Unsupported protocols, authentication and playback failures follow VLC's normal input path.

Save appends the sources to the actual queue and starts the first new entry. An optional trimmed name creates a real media-library playlist on a worker before queueing. Repeated activation cannot duplicate a completed submission. Recoverable failures retain the fields for retry; partial-save rollback failures instead explain where to find the saved playlist and prevent implicit duplication. Close, Cancel and Escape are disabled during the short submitted save. Destruction joins library work before its native parents are released.

The popup supports multiline entry, keyboard saving, accessible fields/errors, focus return and short-window scrolling. Global application shortcuts remain disabled while it is open, even when another editor restores the raw shortcut flag. Dark geometry is provisional: 900 × 540 at nominal scale is inferred from an editorial video frame rather than measured Figma exports. Original fonts/assets/credits are preserved; the close glyph is drawn in code pending its original export.

## Verification

The [native QtTest record](network/native-tests.txt) reports **6 passed, 0 failed, 0 skipped**, including setup/cleanup. The test links the production NetworkStreamModel and actual VLC playlist/player/media-library implementation.

- Validation rejects malformed/local sources and explains an unavailable named-playlist library.
- Unnamed submission preserves three ordered URLs and a duplicate; a repeated submission does not append again.
- Named submission trims a Unicode title, saves three ordered entries and starts the first new queue entry after existing items. Actual HTTP playback produces 24 memory-audio buffers with RMS 0.282322.
- Reopening the actual media-library database retains the name, identity, order and repeated media entry.

The [native harness result](network/native-verification.json) records a successful HTTP GET and the independent fixture hash. The HTTP fixture uses loopback only and test userdata is isolated. The native media library retains its normal LAN discovery behavior; the whole process is not restricted to loopback. No Firewall permissions were changed. Initial diagnostic audio-output setup was corrected to select the memory-audio callback explicitly; the final complete test record is the accepted result.

All **ten Qt 6.8.3 component suites pass locally**. The new suite exercises the production dialog and shortcut component with supplied controller state: entry/Return, optional name, Save/Ctrl+Return, busy/error/retry/reset, unavailable-library focus, focus return, modal shortcut suppression and 100/150/200-percent short/scaled layouts. [Empty preview](network/network-dialog-empty.png), [short preview](network/network-dialog-short.png) and [component verification](network/network-dialog-verification.json) are component evidence, separate from the actual native backend and desktop journeys. Qt reports the existing single-sequence StandardKey shortcut warning in the test driver; it does not report a construction error.

The native test and Qt plugin build complete successfully in WSL Ubuntu 24.04 with MinGW x64 POSIX/MSVCRT and Qt 6.8.3. Both build systems include the model, metaobject, QML resource and native test. The plugin is staged into the separate slice package; the user's working package is retained.

| Artifact | SHA-256 |
| --- | --- |
| `slice-package/plugins/gui/libqt_plugin.dll` | `cc461e887372109dba4a2609f2407471f160625114afa675827d2bef27d4d097` |
| `core-tests/test_qt_network_stream.exe` | `8ba3584363240b159839338d11d5212bb66bfc8917b9e94c4100f8b15453341c` |

The preceding Qt plugin is retained as `libqt_plugin.dll.before-network-20261014`. Reproduce the native diagnostic with `test/libvlc/native_network_stream.py`, the slice package, native test executable and generated 30-second waveform WAV.

## Remaining gates

The previously blocked GUI and Firewall prompt were no longer visible during the morning inventory. A corrected fresh launcher exposed `Canvas is not a type`: this static Qt build omits Canvas. The close glyph now uses ordinary Rectangle items. The blank Home also exposed a read-only `Loader.implicitHeight` assignment in PageExt; binding writable `height` repairs the shared page/header construction. Both fixes are rebuilt into the final plugin above. The network and library component suites pass again.

The final plugin now passes actual Windows startup with rendered Home/header/Open File/Add Folder. Ctrl+N opens the production network dialog; invalid entry shows the native validation message and disables Save. Escape closes it and restores the prior Home focus. [Native window capture](network/native-invalid-address.png) and [desktop verification](network/native-desktop-verification.json) preserve these limited results. Reset/reopen also passed with the preceding Canvas repair. The GUI's library userdata is fresh, but portable configuration/Qt settings are retained; it is not a fully isolated UI profile. Actual GUI submission/playback/library persistence, other media journeys, menu variants and whole-app fidelity remain open. External HTTPS/authentication/protocol variants, injected allocation/database/rollback failures, playlist-source expansion, full appearance and original geometry/assets still require acceptance. The complete milestones 0–8 slice remains active.
