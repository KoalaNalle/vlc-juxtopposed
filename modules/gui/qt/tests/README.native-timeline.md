# Native PlayerController timeline checks

`test_qt_player_timeline` links the production controller and uses the real
NORMAL-lock main-playlist player. The existing test environment supplies only
the interface context and unrelated discovery/library seams. Playback, adaptive
HLS demux, timer updates, capability changes and queued Qt notifications are real.
Audio output is dummy; the fixtures contain generated silence.

Generate fixtures into a **new** directory using FFmpeg, then build the native
test with the configured matching VLC/Qt toolchain:

```sh
bash modules/gui/qt/tests/native_timeline_fixtures.sh NEW_FIXTURE_DIRECTORY
make -C CONFIGURED_BUILD/modules/gui/qt -j1 test_qt_player_timeline.exe
```

The fixture generator also copies its small Python loopback server into the
fixture directory. The server listens only on `127.0.0.1`, serves the generated
segment allowlist and exits normally on the owning test's stop command or stdin
EOF. A live playlist update uses atomic file replacement. No public service or
personal media is required.

Copy `.libs/test_qt_player_timeline.exe` to an owned Windows build directory.
From an environment whose temporary/cache/profile paths are owned test paths,
run the Windows driver with the matching ordinary package:

```powershell
python modules/gui/qt/tests/run_native_timeline.py `
    --package MATCHING_PACKAGE --native-build OWNED_TEST_BUILD `
    --fixtures GENERATED_FIXTURES --output-dir NEW_ACCEPTANCE_OUTPUT
```

Use `--core REBUILT_CORE_DLL` to compare a repaired core with the supplied
package while keeping the test executable identical. Both are copied into the
owned runtime; the supplied package remains unchanged.

The driver copies the package's libraries/plugins into a fresh portable runtime,
regenerates its plugin cache and records executable/library/fixture identities,
Qt test reports and confirmed process termination. The supplied package is not
modified. A timeout is a failure with a retained PID, never a successful quit.

The two native journeys cover finite elapsed time and paused seek-to-zero;
live range-relative time, pause/resume and seek; playlist/window replacement;
and clearing the live state and clock after media removal. The live journey
queues native seek and pause together under one NORMAL player lock so pause
cannot stop the input required to complete seek buffering. Direct signal
observers re-lock the NORMAL player to catch notifications made under its lock.
Live position seeks leave raw stream time unknown until an actual timer point
arrives, including a pause immediately after seeking; their window position
remains available for presentation. A relative seek offset cannot become a cue
timestamp through pause interpolation.
The four reported Qt cases include initialization and cleanup. Missing fixture
configuration skips a direct test launch, which is not acceptance; the Windows
driver requires four passes, zero skips and normal exit.

`test_qt_player_timer_point` separately covers absolute stream timestamps,
unknown/unseekable/invalid ranges, overflow and core interpolation. The QML
component test checks the LIVE indicator, disabled duration toggle and preserved
finite controls. These checks do not establish complete native GUI, accessibility,
DPI, public-service or beta-package acceptance.
