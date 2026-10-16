# Native Windows Library acceptance

These private fixtures exercise the ordinary Windows x64 Core, filesystem and
WinMsg plugins. They do not add diagnostic exports to shipping binaries.
The configured VLC build must include the Qt tests and match the supplied package.
Python 3.11+, the configured cross compiler, Qt Test and the normal VLC build
dependencies are required; the runner generates silent owned media itself.

The opening scope has 42 cases covering real configuration/locked-save behavior,
legacy policies, extracted production instance decisions, the actual receiver,
Windows hidden attributes in parsing and directory playback, folder recursion, external-preparser root identity
and completion independent of optional folder artwork, and library preservation.
The same external child also handles successive On, Off and explicit override
requests without changing the caller's input options.
The real-process scope has nine cases: init/cleanup and seven data cases using
actual sender `vlc.exe` processes, a production receiving Core and WinMsg plugin,
different Unicode working directories, input options, playing/paused append,
replace and file-manager/master policies. Run it with all other same-version
one-instance VLC processes closed; the fixture refuses to send them test files.

The separate metadata scope has 18 cases, including init/cleanup: active and
queued cancellation, timeout, parse fallback, deletion with an active provider,
automatic queue/library local and network scopes, library timeout/restart,
external-child provider identity, queue timeout fallback and removal/recovery.
Owned providers use file traces containing their process IDs, explicit Windows
child environment inheritance and interruptible delays. Network-scope providers
use generated local artwork and make no external requests. Indexing assertions
wait for committed metadata rather than an early database placeholder.
Artwork assertions check queue fetch state/URI and byte-identical library-owned
album thumbnails after relocation, rather than accepting the title alone.
Lifecycle success also leaves an unparsed file eligible for later queue parsing
and preserves a duration update made while the provider is running.

A passing opening run does not accept metadata behavior. The metadata scope must
pass separately. Cooperative providers observe cancellation/deadlines; these
tests do not establish a hard deadline for third-party code that ignores VLC's
interrupt mechanism. Full provider/service coverage, final GUI fidelity and beta
packaging remain separate acceptance work.

The separate real-launcher regression exercises `WinMain`, saved Library ON/OFF
(including VLC's commented default-OFF representation), explicit CLI overrides,
native module loading and owned database creation. It uses the ordinary package
without adding private modules or modifying that package:

```powershell
python "$repo/modules/gui/qt/tests/native_library/startup.py" `
  --package "$package" --launcher "$rebuiltLauncher" --output-dir "$work/startup"
```

Omit `--launcher` to test the package's own executable. Every run requires a new
output directory. Its runtime has an empty portable directory, and each case has
owned config/userdata/log paths. A timeout retains its live PID as a failed result.
Fresh profiles follow the core default (Library OFF); saved ON and explicit
`--media-library` remain supported. This does not accept first-run GUI setup.

From a configured Linux/WSL build, set `repo`, `build` and `work` to absolute
paths for the checkout, its configured Windows build, and a fresh ignored output
directory. Retain the toolchain PATH used for the ordinary build. For example:

```sh
recipes="$repo/modules/gui/qt/tests/native_library"
python3 "$recipes/prepare.py" --output-dir "$work/prepared"
make -C "$build/modules/gui/qt" -f Makefile -f "$recipes/preferences.mk" \
  library-preferences-native PENDING="$work/prepared/input" \
  SHARED_STORE="$repo/modules/gui/qt" PROPOSAL_OUT="$work/native"
make -C "$build/modules" -f Makefile -f "$recipes/modules.mk" \
  library-private-fixture-modules PENDING="$repo/modules/gui/qt" \
  WINMSG_SOURCE="$work/prepared/input/tests/win_msg.c" \
  LIBRARY_MODULE_OUT="$work/native/modules"
make -C "$build/modules/gui/qt" -f Makefile -f "$recipes/process.mk" \
  library-process-native PROCESS_OUT="$work/native/process"
```

Run under Windows, using Windows paths for those same output locations:

```powershell
python "$repo/modules/gui/qt/tests/native_library/run.py" opening `
  --package "$package" --native-build "$work/native" `
  --module-build "$work/native/modules" --output-dir "$work/opening-run"
python "$repo/modules/gui/qt/tests/native_library/run.py" process `
  --package "$package" --native-build "$work/native/process" `
  --output-dir "$work/process-run"
python "$repo/modules/gui/qt/tests/native_library/run.py" metadata `
  --package "$package" --native-build "$work/native" `
  --module-build "$work/native/modules" --output-dir "$work/metadata-run"
```

Use the `metadata-lifecycle` scope for the eight-case lifecycle-only control pair.
Optional `--core`, `--medialibrary` and `--preparser` arguments stage matching
ordinary build outputs into the private runtime without changing the supplied
package. Every staged binary's identity is recorded.

The package is copied into an isolated runtime without its portable profile.
Fresh configuration/userdata, generated media, binary identities, xUnit output,
process exit and personal-config preservation receipts are kept under each new
output directory. Only the private opening runtime gets a fake local provider
and receiver probe. Never copy these modules into the ordinary package.
Timeouts remain failed/incomplete and retain the process ID for diagnosis.

The separate portable fault-branch driver is CI-registered:

```sh
python3 "$repo/modules/gui/qt/tests/test_library_ipc_errors.py" \
  --output-dir "$work/ipc-faults"
```

It compiles exact extracted production helper bodies with deterministic
dependency failures (16 receiver and eight sender cases). It supplements the
native and actual-process tests; it does not emulate or establish Windows IPC.
