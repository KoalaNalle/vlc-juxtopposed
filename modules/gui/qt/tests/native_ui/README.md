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
