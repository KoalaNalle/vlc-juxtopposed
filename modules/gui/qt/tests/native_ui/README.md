# Owned native settings-window journey

This opt-in test links the ordinary GUI objects and QML resources with one
additional test translation unit and QtTest. It exercises the real MainCtx,
compiled page, native preferences store and Advanced dialog. All pointer/key
events target windows owned by that same test program. It does not automate
another desktop app, use a supplied controller or add hooks to ordinary VLC.

Read the configured Qt Makefile first. With the ordinary GUI already built,
add `network_journey.mk` after that Makefile and set `QA_OUT` to a fresh private
output directory. `QA_SOURCE` defaults to the maintained `network_journey.cpp`.
Run `owned-native-ui` with one worker. Never install the resulting GUI DLL or
add this test TU to the ordinary module sources. Preserve the private output
unchanged while a runner hard-links its DLL.

Run `run_network_journey.py --package <owned-ordinary-runtime> --qa-gui
<private-build>/.libs/libqt_plugin.dll --output-dir <fresh-owned-output>` under
Windows. All three paths must be on the same volume. Use the migration kit's
E:-only environment on the travel computer. The runner hard-links immutable
binaries and creates an empty portable directory and isolated E: user data.
Its synthetic proxy credentials are test data, never personal credentials.

Two normal processes cover 31 editing/failure/retry checks and 11 restart
checks. These include repeated pointer-target/window assertions; they are not
42 distinct product features. A real locked preferences file rejects a UI edit,
leaves native values unchanged and permits retry. Native buffering, separately
masked password persistence, restart and the actual QWidget Advanced route are
exercised. Captures come from the owned QQuickWindow.

The fixture requests normal application quit and records process exits,
private/ordinary binary identities, current fixture source hash and individual
checks. Timeout/forced cleanup is a failure. Broader settings interactions,
playing/paused independence, themes, 150/200% Windows DPI, monitor movement and
independent visual scoring remain separate gates.

The Extensions variant uses `extensions_journey.mk` and
`run_extensions_journey.py`. Its default five processes cover the empty original
page, owned Unicode VLP installation/removal and failures/retry, stable selection,
keyboard Home/End, navigation, native extension menus and startup preference
restart and command-line override. A real locked catalog separately exercises
the UI save-retry action without repeating file mutations. Counts come from the report; failed or timed-out runs are rejected.
The optional `live` mode only fetches the production catalog metadata. It never
installs third-party packages. The fixture waits for the actual settings host;
its private CRT configuration reports assertions to stderr instead of hiding a
startup failure behind a modal assertion dialog. Ordinary VLC is unaffected.

Before rebuilding QML, run `test_juxtopposed_extensions_settings.py --output-dir
<fresh-output>` with the Qt/PySide6 test runtime. It loads the actual source with
Qt's semantic property validation: the AOT compiler alone did not reject an
unsupported Keys signal handler. This guard supplies no native consumer proof.

The Home variants use `home_journey.mk` / `run_home_journey.py` for the
owned audio bootstrap/privacy journey and `home_content_journey.mk` /
`run_home_content_journey.py` for genuine native content in all five sections.
The latter requires `--fixtures <E-folder>` containing `first.flac`,
`second.flac` and `owned-clip.mp4`. Use two 60-second tagged audio files with
one shared album/artist (Travel Artist Ää / Owned Album, tracks 1 and 2) and a
60-second video. The runner copies only these generated files into its owned
Unicode scan root; it does not inject models or edit the media database.
The retained guest proof records the actual FFmpeg recipe and fixture hashes.

The accepted content journey uses `--keep-window-size`, which disables native
video auto-resize. Default auto-resize and small-window geometry need separate
acceptance. Real pointer dragging moves the floating video preview before
activating controls it covers. The File menu's New Playlist action, actual
file picker, Save, five See All routes, search, view changes, playlist/artist
cards and Continue Watching are exercised. Seeking and pausing use the native
player as fixture preparation; route activation uses actual owned-window input.
The runner sets global resume to Never to check the Home item's explicit resume
request. Populate/reopen must both pass with normal exits, no forced cleanup,
no captured fault and no selected QML/assertion diagnostics. The two processes
must share one owned profile and use matching Core/ordinary/private-GUI inputs.
