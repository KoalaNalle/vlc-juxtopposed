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
| Component Python | `tools/VlcUiTests/Scripts/python.exe`, Python 3.12 / PySide6 6.8.3 |
| Local Windows tools | Ignored root `tools/`: Git, Python312, VlcUiTests, Wheels and WinDbg |
| Active generated outputs | Ignored `build-windows/` and `ui-previews/` |
| Cold archives | `D:\Modding\vlc-juxtopposed\build-windows`, browsable through `build-windows/archive` |

Recheck paths/tools before use. The original setup and guest-machine migration
are historical; E:-only paths and guest resource limits are not home instructions.
Preserve the original distro, migration copies and working contrib cache. The existing
tools support the current native work. Ask before installing
anything beyond the user's expressly authorized environment setup.

The Windows tools moved within C: on 29 October. The old
`build-windows/homecoming-20261018/tools` path is a compatibility junction to
`tools/`, preserving frozen helpers. New helpers should use the root tools path.
The virtual environment's configuration, pip and PySide console launchers point to the new
location; no dependencies were installed. The active WSL virtual disk stays in
`build-windows/homecoming-20261018/wsl`; it was not moved with the Windows tools.

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

GNU make dry runs can still remake included makefiles and dependency records.
Never use `make -n -B` as a read-only diagnostic: it reset 1,131 Qt dependency
records on 28 October. Review included-file behavior before any make probe.
Dependency recovery can use the matching compiler directly with `-M -MP -MT`
and an explicit fresh `-MF` scratch destination, without compiling an object.
Validate the complete regenerated graph before separately approving installation.
Keep one live WSL namespace across a raw-metadata freeze and its guarded build:
device numbers can change between WSL invocations even when file contents match.

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
and original-state backup are
`build-windows/archive/cold-archive-20261029-r1/loose-files/configure-vlc-firewall.ps1`
and `build-windows/archive/firewall-setup-20261014/`. Those historical executable inventories
must be reviewed and refreshed before Apply or Undo; never run an old helper
blindly. Archive the journals/backups with cold build evidence before removing
build outputs, so project rules can be identified and safely cleaned up later.

## Component and native checks

The [Qt UI workflow](../../.github/workflows/qml-ui-checks.yml) lists maintained
component suites and their dependency pins. Use the existing Python environment.
For example, from the repository root in PowerShell, choose an unused output name:

```powershell
& .\tools\VlcUiTests\Scripts\python.exe `
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

`test_src_player_recording` is an opt-in real-input notification regression in
`test/src/player/recording.c`. Pass a generated short A/V file and an owned
recording output directory as its two arguments. Stage the matching Core/runtime
and set `VLC_TEST_PLUGIN_PATH` to its plugins. It checks recording notifications
at EOF, same/other-media succession, replacement, Stop, manual off and subsequent
recording; it does not qualify the recorded content. The 29 October
[before/after receipt](../juxtopposed-references/native/recording-repeat-webvtt-20261029/README.md)
preserves its compiler and runtime identities, controls and teardown diagnostics.

`test/libvlc/native_recorded_webvtt_pause.py` is a fixture-specific actual decoder
snapshot test, using the existing PySide Python environment. Its arguments are
the staged runtime, an existing empty output directory, the generated recorded
MP4 and a start offset (0, or 12 through 12.04 seconds). It uses the recording in
the [paused-start receipt](../juxtopposed-references/native/recorded-webvtt-paused-20261029/README.md),
including its embedded cue's MP4 edit. LibVLC time/seek arguments are microseconds
relative to the start offset. Frame markers must remain identical on selection;
the held-time tolerance is 50 ms. Set the staged plugin path/isolation as in the
frozen runner, adapting fresh outputs before use. This is not a general media
checker or evidence for all subtitle formats, physical GPUs or final packaging.

`test_src_player_subtitle_fps` is an opt-in actual Core/decoder snapshot driver
in `test/src/player/subtitle_fps.c`. Arguments are generated video, external
subtitle, existing empty output folder, FPS, mode (`reference`, `hold`, `inherited`
or `playing`, plus the startup modes below), target seconds and category delay in microseconds. Use a matching
staged runtime, isolated environment and `VLC_TEST_PLUGIN_PATH` as in the frozen
[inline WebVTT runner](../juxtopposed-references/native/inline-webvtt-fps-20261029/README.md).
Its JSON observations and pictures need independent reference/mask assertions;
an FPS getter or successful process alone does not prove retimed rendering.
The direct compiler build is qualified separately from the Makefile registration.

`startup-single` and `startup-batch` instead use the generated two-embedded-track
marker MP4, select Finnish and treat target seconds as the media start offset.
Pass `-` for the unused external-subtitle argument and 1,200,000 for delay. The
driver captures initial, delayed and reset pictures, then an explicit user seek
and a subsequent redraw. The batch models Default's native delay/FPS/per-track
reset sequence. Compare each process's actual initial marker; fractional startup
can retain a frame before the visible origin. A fixed elapsed clock alone is
insufficient. The [preset repair receipt](../juxtopposed-references/native/preset-retry-held-frame-20261029/README.md)
binds a before failure, unchanged-oracle after run and ordinary hardware result.

`test_modules_demux_ttml_fps` in `test/modules/demux/ttml_fps.c` is an opt-in
source-compiled actual TTML demux/parser/serializer check. Pass
`test/modules/demux/fixtures/ttml-fps` as its argument and use the isolated runtime
environment from its [receipt](../juxtopposed-references/native/ttml-fps-nested-clock-20261029/README.md).
It loads the real XML reader and checks containing-cue seeks, rates/Reset,
invalid controls, EOF revival, parser roundtrip and mixed clock/frame offsets.
Unused module-descriptor callbacks are failing stubs. Actual decoder output is
verified by the separate native snapshot matrix. The expanded
[time-parameter receipt](../juxtopposed-references/native/ttml-time-parameters-20261029/README.md)
qualifies media-time frame/multiplier/subframe/tick parsing and malformed-value
cleanup. Its source tests have 538 checks. Native boundary probes check actual
video clocks: a nominal seek can round forward to a 25-FPS frame. Preserve the
failed nominal-time calibration separately. Direct-build evidence is distinct
from Makefile registration; other TTML time bases/profiles remain separate.

`test/modules/demux/avformat_mux_queue.c` includes the production queue helper
and uses real Core FIFOs/frame ownership. The regular `make check` entry is
`test_modules_demux_avformat_mux_queue`; its 41 cases cover all 24 A/V/S/S ending
orders, live flush/gaps, exact release and failure cleanup. The home cross build
has a separately recorded focused compile/run with 5,772 checks. That result
does not claim the whole standard test target or FFmpeg/container paths passed;
use the [real writer receipt](../juxtopposed-references/native/recording-tail-drain-20261028/README.md)
and its container follow-up for those distinct scopes.

The pending notice helper adds ten cases (51/6,062 total) with separate queue-only
evidence. In the current merged-FFmpeg configuration, rebuild `libavcodec_plugin.la`,
not the disabled separate avformat plugin. Preflight must use `MERGE_FFMPEG` so
the included `avformat.c` is inventoried. The [current notice receipt](../juxtopposed-references/native/ffmpeg-output-notices-20261029/README.md)
records the normal 13-unit build, actual dependencies and shared-artifact restore.
`test/libvlc/native_avformat_notice.py` is a bounded-runner worker using the
public error/state callbacks. Supply the isolated runtime, generated media,
fresh case directory and success/header-error/writer-error/cancel mode. Writer
failure requires a parent closing the stdout sink; cancellation requires a paced,
open sink and its actual-output marker. Trusted media-item sout options are
required: LibVLC's player does not inherit the global sout string. The parent
must verify child exits, callbacks, logs, preservation and decoded content;
worker exit alone does not qualify a case. Frozen producers are examples, not
commands to rerun into existing outputs. Ordinary Details remains separate.

First-change FPS checks in `test_player_subtitle_delay.cpp` are opt-in:
run the selected function or data row alone in a fresh process with
`VLC_TEST_FIRST_FPS_FRESH=1`. Earlier FPS edits in the same input can mask the
initial-rate case. `VLC_TEST_PRIMARY_SUBTITLE` selects an alternate external
filename beside the generated video. The explicit `VLC_TEST_FPS_FORMAT_ONLY=1`
mode avoids requiring Freetype appearance/preset support for timing checks of
renderer-owned ASS styles; it does not qualify those style controls. The
nonseekable and overlapping fixtures use their separate opt-in variables and
must follow their fixture-specific setup rather than a seek-based shortcut.
`VLC_TEST_PLAYBACK_RATE=0.25` starts the native fixture at quarter speed. The
first-live-change checks allow 700 ms of media-time demux progress, scaled to
the actual playback rate; they retain independent frame/clock bounds. Delayed
cases require a nonempty reference caption before evaluating the FPS edit.
`VLC_TEST_FPS_PROMPT_RESPONSE=1` instead uses the original 700 ms wall-time
settling wait, including at quarter speed, to catch deferred command handling.
Snapshot and callback overhead is additional; this is not an exact latency
measurement. Run each selected case in its own fresh process.

For `firstDefaultFpsChangeRetimesPlayingCaption`, optional `VLC_FPS_RECORDINGS`
must name an existing empty output directory. Recording begins before the first
FPS edit and remains active past the fixture's next 20-second video keyframe.
The ten-second wait assumes normal playback speed. The test checks native state,
caption, clock and nonempty output; separately decode the complete recording and
verify its subtitle text/timestamps/durations. A nonempty file is insufficient.
Video starts after the FPS edit in this fixture, so it cannot establish encoded
AV continuity across that edit. See the
[recording follow-up](../juxtopposed-references/native/home-subtitle-refill-20261028/README.md).

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

Old top-level archive shortcuts are grouped under `build-windows/archive`, a
junction to the HDD archive. Individual compatibility paths remain only where
maintained tests or the deferred comparison require them; internal links in
active build trees retain their own purpose. Do not follow archive junctions
when measuring C: usage. WSL cannot reliably traverse cross-volume junctions;
use the corresponding `/mnt/d/Modding/vlc-juxtopposed/build-windows/...` path
when reading cold artifacts from Linux, and refresh helper inventories first.

Folder dates and summed file sizes do not establish whether storage is cold.
The old homecoming directory still contains the working WSL virtual disk, and
`build-windows/package` remains the configured Windows packaging destination.
Both stay physically on C:. Historical test runtimes share many NTFS hard links;
count distinct file identities and retained links before estimating reclaimed
space, preserve that sharing on D:, and verify checksums before removing originals.
The maintenance started 29 October and completed 30 October. Records are under
`build-windows/storage-maintenance-20261029-r1/`, with a full mirror at the same
name on D:. All 941,286 regular file entries and their link mappings passed
verification before removal of the selected C: originals; the three protected
current/clean/fallback runtimes passed before/after binary checks. The archive
preserves hard links (174.8 GB of distinct data across about 2 TB of logical file
sizes). Do not expand that sharing through a naive copy of whole old collections.
The cutover reclaimed about 170 GB; consult STATUS and recheck current free space.
These maintenance helpers and receipts are one-use historical evidence. This
operation did not run GUI tests or advance the beta acceptance gates.

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
