# Home development and validation

Use [STATUS](STATUS.md) for the selected runtime and [BETA](BETA.md) for acceptance.
This guide describes the established home setup; it does not request a fresh
installation, rebuild or desktop test merely by being read.

## Environment

| Item | Recorded home location/version |
| --- | --- |
| Authoritative source | `C:\Modding\vlc-juxtopposed`; active branch and fallback in STATUS |
| WSL build distro | `VLC-Juxtopposed-Home-20261018` |
| Linux build root | `/root/vlc-juxtopposed-build`; configured VLC in `win64` |
| Source as seen by WSL | `/mnt/c/Modding/vlc-juxtopposed` |
| Native build lock | `/root/vlc-juxtopposed-build/.vlc-build.lock` |
| Target | Windows x64, MinGW-w64 GCC 13 POSIX, pinned Qt 6.8.3 contrib |
| Component Python | `build-windows/homecoming-20261018/tools/VlcUiTests/Scripts/python.exe`, Python 3.12 / PySide6 6.8.3 |
| Active generated outputs | Ignored `build-windows/` and `ui-previews/` |
| Cold archives | `D:\Modding\vlc-juxtopposed`; retain existing verified archive/junction layout |

Recheck paths/tools before use. The original setup and guest-machine migration
are historical; E:-only paths and guest resource limits are not home instructions.
Preserve the original distro, migration copies and working contrib cache. The existing
tools support the current native work. Ask before installing
anything beyond the user's expressly authorized environment setup.

## Before a build

1. Inspect Git and preserve unrelated work. Check the recorded process/session
   before restarting an interrupted observer; a timeout can leave its child alive.
2. Determine exactly which production targets and dependents changed. Reuse the
   configured tree and compatible dependencies. Start with two workers and one
   lock-owning writer; avoid full rebuilds without a concrete dependency reason.
3. Create a fresh run/output directory. Review any reused helper's paths, flags,
   source expectations, output names and versioned source inventory. Freeze a
   copy before launch; editing a running helper previously invalidated its ending.
4. Record source/fixture/binary identities. Inventories must include the actual
   test dependencies and current document paths. A manifest only proves the
   files it enumerates; preserve old receipts unchanged when updating an inventory.

The successful incremental FPS build is documented by
[its frozen build producer](../juxtopposed-references/native/home-subtitle-fps-20261024/producers/build-ordinary-r3.sh)
and [dependency records](../juxtopposed-references/native/home-subtitle-fps-20261024/ordinary-dependencies/).
It builds the regular Core and ordinary Qt object list. Its old-Core precondition
and one-use outputs make it an example to adapt, not a command to rerun unchanged.

After a large upstream update, inspect imported object dependency files before
trusting an incremental build. The 27 October integration found old `.o`/`.lo`
files whose `.Plo` files contained only `# dummy`; changed public headers therefore
did not rebuild them. An obsolete `video_format_IsSameChroma` import broke linking.
Under the build lock, the owned stale objects were quarantined with verified
hashes, then rebuilt. Fresh objects had real dependency lists. Preserve this
quarantine and the tested fallback; do not reuse those stale objects or repeat
the old quarantine helper against a later build without a fresh inventory.

For a genuinely fresh environment, inspect [INSTALL](../../INSTALL) and the
repository's [Windows build script](../../extras/package/win32/build.sh). It
needs a Unix-compatible shell and the Windows cross toolchain; its help describes
the available Autotools/Meson/UCRT/packaging routes. The historical help command
returns 1 even when displaying help successfully. Preserve required local recipe/
dependency patches and the Qt GUI; `-z` disables the GUI. Clear exported cross
`CC`/`CXX` when building Linux host tools. Do not switch build routes or fetch
unmatched prebuilt contribs to repair a working incremental setup by guesswork.

## Staging and desktop testing

- Stage a fresh isolated ordinary runtime with matching executable, Core, GUI,
  codecs/audio modules, resources and plugin cache. A compiled DLL alone is not
  a runnable package. Keep private QtTest modules/hooks out of the distributable.
- Some consolidated runtimes share hardlinks. **Unlink the destination before
  copying a replacement DLL.** Never modify a loaded DLL or an older accepted
  package indirectly. Record identities before and after a run.
- Use an isolated portable profile for tests. Keep installed legacy VLC available
  for comparisons; do not uninstall it or change user associations/settings.
  Verify the executable and instance-handoff policy target the intended test app.
- Check current permission to use the desktop. Use the Computer Use skill for
  native input, fresh window observations and normal closure of owned test apps.
  Security/privacy choices follow the current user's express authorization and
  applicable tool policy. Preserve the agreed network scope and enabled Firewall/
  Defender protections. If a tool requires a human action, record it and continue
  independent work.
- Preserve raw private logs locally. Publish generated fixtures, scoped reports,
  hashes and useful sanitized excerpts/captures. Record child exit results;
  a launcher exiting 0 does not prove its VLC child passed or exited normally.
- Bind results to the exact source/runtime/fixture tuple. Distinguish deliberate
  negative controls and fixture warnings from accepted ordinary-flow behavior.

### Firewall rule journal

For authorized development rules, keep exact executable paths, Private-network
TCP/UDP access restricted to LocalSubnet, Public/Domain inbound blocks and blocked
edge traversal. Keep the firewall and Defender enabled. Record every created,
changed or removed rule's exact name, purpose, program, before/after settings,
verification and removal/restoration steps in an ignored firewall ledger before
changing it. Preserve original prompt-rule states when replacing their behavior.

The read-only home inventory is under
`build-windows/firewall-ledger-20261027/`; it creates no rules. The earlier helper
and original-state backup are `build-windows/configure-vlc-firewall.ps1` and
`build-windows/firewall-setup-20261014/`. Those historical executable inventories
must be reviewed and refreshed before Apply or Undo; never run an old helper
blindly. Archive the journals/backups with cold build evidence before removing
build outputs, so project rules can be identified and safely cleaned up later.

## Component and native checks

The [Qt UI workflow](../../.github/workflows/qml-ui-checks.yml) lists maintained
component suites and their dependency pins. Use the existing Python environment.
For example, from the repository root in PowerShell, choose an unused output name:

```powershell
& .\build-windows\homecoming-20261018\tools\VlcUiTests\Scripts\python.exe `
  modules/gui/qt/tests/test_juxtopposed_settings.py `
  --output-dir build-windows/new-settings-check
```

This settings suite uses offscreen/software Qt internally. Supplied-model tests
check presentation and bindings; they do not establish native playback, physical
DPI, screen-reader usability or final visual fidelity. Other maintained entry
points include `test_juxtopposed_playback_bar.py`, `test_native_seek_handle.py`,
`test_playback_settings_choices.py`, `test_playlist_proxy_mapping.py`,
`test_juxtopposed_queue.py` and `test_juxtopposed_queue_overlay.py` under
`modules/gui/qt/tests`. Select checks relevant to the actual change.

Native controller/media/PCM tests use their maintained harnesses and generated
fixtures. For the current FPS case, the test requires `VLC_FPS_SNAPSHOTS` and the
matching 60-second marker fixture; its default skip does not qualify rendered FPS.
Dummy subtitle blending needs supported `--no-hw-dec`; do not restore obsolete
decoder flags from old attempts. Source identities must prove genuine external
tracks were selected even if the main media already contains embedded subtitles.

`test/libvlc/native_mp4_subtitle_seek.py` exercises the public Windows LibVLC API
with the generated 960x540 marker MP4 and a supplied external SRT. Positional
arguments are runtime folder, existing empty output folder, media path, SRT path,
rate, audio backend (`wasapi` or `dummy`), seek seconds and subtitle selection
(`eng`, `fin`, `both` or `none`). The media needs the generated English/Finnish
embedded tracks and eleven-bit frame marker. It checks delayed/Reset held frames,
captions or their intended absence, resume and normal release. Explicit placement
keeps dual-caption image bands repeatable. See the frozen
[fragment checkpoint](../juxtopposed-references/native/home-fragmented-mp4-20261026/README.md)
for fixture generation and producer examples; it does not qualify initial paused
caption display or every fragment/index layout.

`test/src/player/timer_cache.c` compiles the production timer implementation
directly to check video timestamps before frame-rate metadata becomes available.
From the configured Windows test build directory, add
`-f <repo>/test/player-timer-cache-native.mk` and use the target
`owned-player-timer-cache` with a fresh absolute `TIMER_CACHE_OUT` directory.

`test/modules/stream_out/record_start.c` compiles the production recording module
to test its buffered start-time calculation. From the configured test build
directory, add `-f <repo>/test/record-start-native.mk`, set a fresh absolute
`RECORD_START_OUT` and build `owned-record-start`. It checks sparse subtitles,
audio/video alignment and subtitle-only fallback. It does not record media;
the separate [recording receipt](../juxtopposed-references/native/home-recording-start-20261027/README.md)
contains actual native/ordinary output decoding and stream-timing comparisons.

`modules/gui/qt/tests/test_pointing_tooltip_text.py` takes `--repo-root`, a fresh
`--output-dir` and `--mode fixed`. It loads the actual tooltip QML and compares
its shaped text layout with an independent literal Qt Text item. The baseline
mode requires the exact expected old formatting failures. Style/MainCtx are
supplied dependencies; ordinary popup placement and OS-DPI remain separate.
The support library is linked statically with the configured compiler's existing
stack protection enabled. Stage the executable beside the runtime DLLs.

Its sixteen cases cover four rates, paused/playing dates and listener presence.
They check timestamp retention and withholding frame-number callbacks until a
known rate arrives. This is a source-level component check with an opaque local
source token, separate from actual decoder/track/output integration. Do not put
the test executable into an ordinary package. The
[selection/clock checkpoint](../juxtopposed-references/native/home-subtitle-selection-clock-20261027/README.md)
retains failing previous-source cases, passing fixed-source cases and independent
ordinary first-frame selection/Reset exports.

The opt-in Qt sidecars `tests/native_library/music-queries.mk` and
`tests/native_library/ml-play-request.mk` build real media-library query and
production model/service command-lifetime checks. From the configured Qt build
directory add the selected sidecar after `-f Makefile`, then use
`music-queries-native MUSIC_QUERIES_OUT=<fresh absolute folder>` or
`ml-play-request-native ML_REQUEST_OUT=<fresh absolute folder>`.
Copy the real executable from that output's `.libs/` directory, not the small
libtool launcher in its parent. Stage it beside the identified runtime DLLs and
regenerate the plugin cache after replacing the library plugin.

Both need an isolated empty `VLC_USERDATA_PATH` and `VLC_MUSIC_FIXTURE_DIR`
pointing to the generated five-tagged-audio/one-video fixture. The query check
also supports `VLC_MUSIC_OLD_TRACK_CRITERION=1` as the intentional preceding
mapping control; its expected failure is separate from a positive run.
The [music receipt](../juxtopposed-references/native/home-music-library-20261027/README.md)
retains exact producers, fixture generation, Qt reports and ordinary evidence.
These executables and profiles do not belong in the ordinary package.

Run meaningful affected checks once; repeat or broaden for a changed dependency,
new failure or unresolved coverage question. Final-package validation is a separate
consolidated run on the actual ordinary candidate, not the sum of all old passes.

The maintained `test_qt_local_music_folder.exe` target covers native local metadata,
folder browsing/history, queue independence/playback and worker teardown. Copy its
actual Windows PE from the Qt build directory's `.libs/`, rather than the small
libtool launcher in the parent directory. The music component test mirrors
`assets.qrc` aliases into its ignored output directory; direct physical filenames
can hide a resource-name mismatch. Keep that mirror outside temporary cleanup
because Windows FontLoader can retain a font handle until process exit. The
[source-header receipt](../juxtopposed-references/native/home-music-source-header-20261027/README.md)
binds these checks to the tested ordinary GUI and records their distinct scopes.

## Storage and source map

Keep active source and build work on C: and respect the owner's under-80% SSD
usage target. Reuse immutable unchanged source copies instead of duplicating full
trees for every receipt. Copy cold artifacts to the HDD, verify the copies before
reclaiming SSD space, and preserve link targets and needed active files. Never
clean unknown folders as part of a routine test or documentation change.

| Area | Source |
| --- | --- |
| Qt settings/state models | `modules/gui/qt/maininterface/` |
| Player adapters, subtitle/effects services | `modules/gui/qt/player/` |
| Shared QML controls and dialogs | `modules/gui/qt/widgets/qml/`, `modules/gui/qt/dialogs/` |
| Playlist/library adapters | `modules/gui/qt/playlist/`, `modules/gui/qt/medialibrary/` |
| Core playback, input and clock behavior | `src/player/`, `src/input/` |
| Regression/component harnesses | `modules/gui/qt/tests/`, relevant maintained native tests |
| Windows build/dependencies | `extras/package/win32/`, `contrib/src/` |
| Original artwork/fonts | `modules/gui/qt/pixmaps/juxtopposed/`; retain attribution/licenses |

The selected receipts in [STATUS](STATUS.md) identify current local helpers and
their limits. Windows 10 comparison,
physical devices and complete final-package checks stay separate from this setup.
