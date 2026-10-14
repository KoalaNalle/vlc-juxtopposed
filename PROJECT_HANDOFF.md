# VLC Juxtopposed redesign — project handoff

Updated: **15 October 2026**. Read this file first when continuing from a new
Codex session on the home computer.

## Project and current checkpoint

| Item | Value |
| --- | --- |
| Fork | <https://github.com/KoalaNalle/vlc-juxtopposed> |
| Working branch | `juxtopposed-playback-bar` |
| Draft pull request | <https://github.com/KoalaNalle/vlc-juxtopposed/pull/1> |
| Latest implementation checkpoint | Branch tip; library/settings evidence linked below |
| Intended home checkout | `C:\modding\vlc-juxtopposed` |
| Initial platform | Windows desktop, x64 |
| Implementation | VLC's existing C++/Qt Quick/QML interface and native playback models |
| Immediate priority | Complete the first publishable beta against all confirmed Juxtopposed design surfaces |

The latest [General Settings checkpoint](doc/juxtopposed-references/native/general-settings-20261015/README.md)
opens the original General layout through Preferences/Ctrl+P, with native saved
language/theme/accent, startup and logging controls. The ordinary Windows build
and backend run pass (29 passed, 0 failed, one separately injected fault case
skipped); three native app runs quit normally and preserve the checked human VLC
configuration. Light/Purple and Unicode Errors-only logging restore after restart.
General is the only enabled category; the other twelve remain unfinished.
The new maintained General/shared-dialog suites and actual GNU-make remake oracle
pass locally; exact new-head remote CI must be checked separately. Backdrop blur,
Light volume artwork, native Reset, remaining categories and final licensed clean
packaging retain open gates. Continue the full beta goal after this checkpoint.

The subsequent [Light volume checkpoint](doc/juxtopposed-references/native/light-volume-20261015/README.md)
uses the exact original complete Light artwork. Its ordinary Qt build and native
pointer/keyboard/mute/wheel/theme-switch journey pass, including normal quit and
preserved checked human settings. Seven maintained component groups and the
existing playback/43 appearance checks pass. All four CI jobs on the preceding
General head `a8688e13fa` are green. Both Windows jobs on the subsequent Light
head `73ab60e6b9` pass all 22 commands, and both Ubuntu jobs pass. Full Light
fidelity, native System events, backdrop and the remaining beta gates stay open.

The [settings backdrop checkpoint](doc/juxtopposed-references/native/settings-backdrop-20261015/README.md)
adds native Home blur/shadow in Light/Dark, an honest original-tint fallback over
separate embedded video and a reproduced incremental QML packaging repair.
The same native shader fixture passes all 39 original assertions; maintained
General/host/software backdrop checks pass 12/24/23 with no Qt messages. Actual
GNU Make reproduces the old exit-0/stale-type failure and passes the repaired
metadata closure. Three native app checks quit normally and preserve the checked
human settings. Hardware-video blur, performance/DPI/full fidelity and the other
categories remain open; new-head CI must be checked separately. The native
window-size drift is being diagnosed without a guessed geometry correction.

Fetch the latest branch tip to receive this document as well as the code. A full Windows
x64 debug application has now been compiled and packaged on the home machine.
Basic native startup/plugin loading and Qt GUI loading pass. Initial real-video
and control checks work; comprehensive native acceptance remains pending.
The redesign is still an implementation in progress and the
PR remains a draft.

The user's active Goal now targets the first publishable, feature-complete beta;
the [beta plan](doc/juxtopposed-beta-plan.md) expands the earlier
[vertical slice plan](doc/juxtopposed-vertical-slice-plan.md). The user authorized
pushing each built and verified runnable checkpoint. The GUI
fixes below form the first checkpoint. Code Review reports the preceding remote
revision `561fd18388` passed the Windows UI workflow; both Windows checks on the
preceding pushed `7ff50d23fd` also passed. Both exact-head Windows UI checks on
the latest `8e257f915c` checkpoint passed all fourteen suites. That checkpoint
adds native playlist creation, reliable Home folder visits and honest unknown
durations. A later [audio-only folder desktop journey](doc/juxtopposed-references/home/audio-folder-desktop-20261014/README.md)
also passed in the existing test profile; fresh-user/full Home acceptance stays
open. Figma MCP access was restored on
14 October after the user connected a Professional Full seat. Page inventory,
high-fidelity Home/New Playlist/subtitle-timeline context and screenshots now
read successfully. Reuse the [saved MCP reference inventory](doc/juxtopposed-references/figma/20261014/README.md),
preserved exports and supplied video; browser permission remains a separate
unresolved setting and is not needed for these successful MCP reads.

## Active Goal progress on 14 October

The later [FLAC and SoXR regression checkpoint](doc/juxtopposed-references/native/audio-20261014/README.md)
repairs complete short/final FLAC frames, encoder finalization, demux EOF and
resampler draining/ownership. The Windows native gates pass: 11 source-compiled
SoXR lifecycle cases with actual SDK/native blocks, 7 actual loaded-packetizer
cases, and 9 staged FLAC encode/decode cases. Exact source/runtime identities,
build evidence, original/copy hashes and failed diagnostic history are retained.
The working package is preserved; these passes do not finish native conversion
dialog, subtitle timeline, Figma fidelity or final beta packaging.

The native GUI baseline and active full-slice plan were pushed as `7c54e2548b`.
Both Windows UI checks on that exact revision passed through Code Review.
The next checkpoint repairs paused-seek clock lateness and a first-picture race;
see [runtime evidence](doc/juxtopposed-references/runtime/paused-seek-verification.md).
The core rebuild, ES-out/input-clock regressions and packaged LibVLC color test
pass. The separate native AV1 redesign test updates paused frames, resumes and
stops without the reset loop. The original working package retains its old core;
the revised core is in `build-windows/slice-package`. Milestone 1 still has the
startup/shutdown and helper-process gates recorded in the evidence file.

The [coverage ledger](doc/juxtopposed-references/coverage-ledger.md) starts the
remaining reference audit. The complete walkthrough is sampled locally; exact
screen measurements, Figma nodes and advanced-surface confirmation remain open.

The next built checkpoint fixes system-wide Windows pipe-name collisions between
VLC and its preparser. A fresh traditional player now opens/seeks/resumes the
color fixture. A stricter video-only regression also exposes a lost output-clock
pause date on reset: before the repair resume skips ahead, afterward actual red
and blue frames arrive at the expected times without a late-frame burst. The
clock-start, ES-out, input-clock and concurrent-pipe tests pass. Details and the
tested core hash are in [the follow-up runtime record](doc/juxtopposed-references/runtime/windows-helper-and-resume.md).

The Qt rebuild also qualifies toolbar-dialog Basic controls and repairs the
search popup's construction-time anchor. All six component suites pass again.
The media-library native launch initializes and renders its shell, then reaches
a Windows Firewall prompt. Input checks await the user's dismissal; no Firewall
permissions were changed. Toolbar-editor and complete latest-core startup,
shutdown and AV1 UI acceptance remain open. The Goal remains active for all
milestones, and no completion or whole-app visual claim has been made.

The user chose saved manual Figma exports plus video when MCP quota remained
exhausted, and later authorized browser inspection. Browser attempts still
rejected `www.figma.com` because of a saved site block; do not circumvent that
block. MCP now succeeds independently with the restored Professional Full seat.
Conserve calls by reusing saved high-fidelity context, screenshots and assets.
Three exact-time library frames, provisional shell geometry and native mapping
are preserved in [library notes](doc/juxtopposed-references/library/reference-notes.md).
The generated chapter/subtitle/audio fixtures have been probed; their native
feature checks remain pending.

The shared navigation now has a compiled checkpoint: 46px rail / 204px expanded
sidebar, real history routes, active/scanning/unavailable states and keyboard
focus. Home exposes a native Add Folder action with worker-backed registration;
shared playlists include both audio and video. The final title/header and media
layouts remain open, as do original navigation-icon exports and native UI checks.
All seven component suites pass. The staged Qt DLL's generated resource lists
include both new QML types. See [navigation/library evidence](doc/juxtopposed-references/library/implementation-checkpoint.md).

A separate native backend diagnostic indexes the generated fixture folder,
filters/clears search, creates an ordered mixed playlist and verifies persistence
after restart, exiting 0. This proves the actual backend operations and does not
substitute for the blocked Qt picker/library journey. Its userdata is isolated
under `build-windows/native-library-final`; the user's library is unchanged.
The full milestones 0–8 Goal remains active and milestone 2 is incomplete.

The next [header/library checkpoint](doc/juxtopposed-references/library/header-library-checkpoint.md)
builds a shared back/forward/breadcrumb/count/sort/view/search header, compact
video/Songs rows and Music's Add/Songs/Albums/Artists/Genres entry view. The
adapters use VLC's existing models, selection, grouping, drag and playback paths.
Forward history retains stored view properties and skips stale network entries.
All eight component suites pass, ten changed QML files parse, and the native
QtTest history regression reports seven passes with zero failures. The newer Qt
DLL is staged separately; its hash, component captures and rollback filename are
recorded in the checkpoint. No native GUI pass is claimed for these adapters:
desktop input still awaits the firewall prompt, and title-strip/folder hierarchy,
original icons and the remaining library presentation remain open. Both exact-head
Windows workflows passed for the preceding `9f754b961c` checkpoint; check the new
pushed revision separately through Code Review. Continue the full 0–8 Goal.

The header/library checkpoint was pushed as `9f743a4ffb`; all eight component
suites passed in both exact-head Windows workflows through Code Review. The
next [settings checkpoint](doc/juxtopposed-references/settings/implementation-checkpoint.md)
replaces the interim list with Playback/Video/Audio/Subtitles tabs, live choices,
numeric speed/delays and a native ten-band equalizer. All nine component suites
pass locally. A native QtTest/LibVLC diagnostic verifies actual filtered PCM,
engine callbacks, consecutive band edits, disabled-filter presets and validation.
The Qt plugin builds and is staged separately; its hash and rollback filename
are in the checkpoint. The Music Add action's dialog-module import is corrected.
The saved references also confirm the full Global Settings categories and a
distinct first-run theme/accent step. Desktop input still awaits the firewall
prompt; missing video/subtitle/settings fields, original icons, themes and all
other full-plan gates remain active work. No whole-app score or completion is
claimed.

The following [video transform checkpoint](doc/juxtopposed-references/settings/video-transform-checkpoint.md)
adds the reference's None/Horizontal/Vertical selector and degree input. The same
native model observes current VLC filters and preserves other effects. Final
rendered-video tests pass 11 checks, including combined transforms, all seven
existing effects orientations, engine updates, validation and detach/reattach.
All nine component suites pass; numeric text clipping is repaired. The Windows
Qt build exits 0, and its DLL is staged separately with the preceding settings
DLL retained. Desktop interaction remains pending at the firewall prompt;
non-square/hardware variants and restart persistence retain open gates. Continue
the remaining subtitle/effects/preferences surfaces under the complete plan.

The [subtitle appearance checkpoint](doc/juxtopposed-references/settings/subtitle-appearance-checkpoint.md)
adds embedded formatting, font/colour/outline/background, offset and Relative/Pixels
controls. The rebuilt core invalidates cached subtitle cue formats safely, and
Freetype applies changes to the current paused cue while retaining playback-message
styling. Seven native output checks pass, including actual font-family/size/scale,
colours, outline, background, a measured 20px offset and unselected/output guards.
All nine UI suites, the core clock/pipe checks, packaged paused seeking and preceding
audio/video diagnostics pass on the updated runtime. Core/Freetype/Qt builds pass
and are staged together with each old DLL recoverable. The evidence records exact
hashes, component/native captures and separate GUI/format/persistence/visual gates.
Desktop input remains pending; presets/timeline, remaining effects,
preferences/themes and the complete integrated slice are still active work.

The [subtitle duration checkpoint](doc/juxtopposed-references/settings/subtitle-duration-checkpoint.md)
adds the referenced three-decimal field/slider using the real duration filter and
its current calculation mode. Eleven native rendered-output checks pass, including
disabled/multiplier/seconds/content timing, expiry, repeated paused seeks and filter
cycles at 4× speed, appearance with the filter active and inline/quoted-chain guards.
Core repairs address source/system timing, cloned region ownership, styled text
ranking, proxy removal and paused dejitter. All nine component suites and native
clock/pipe/equalizer/video/paused-seek regressions pass. Matching core/filter/Qt DLLs
are staged with preceding versions recoverable; exact hashes and before/after
evidence are recorded. GUI/other-format/overlap/hardware/persistence/visual gates
remain open. Continue all outstanding milestones 0–8.

## Scope and design credit

Implement Juxtopposed's VLC redesign inside VLC while preserving playback,
playlist behavior, keyboard/touch input, accessibility and useful existing
features. Start with the Windows dark interface, then expand screen by screen.

**All credit for the original UI redesign and original artwork belongs to
[Juxtopposed](https://www.youtube.com/@juxtopposed).**

- [Original Figma community design](https://www.figma.com/community/file/1691465999134370980/vlc-redesign).
- [Design walkthrough video](https://youtu.be/ERFeOgqk324).
- Home-machine video reference supplied on 13 October 2026: [I Redesigned VLC After 25 Years.mkv](<C:/Users/Koala/Downloads/I Redesigned VLC After 25 Years/I Redesigned VLC After 25 Years.mkv>). The file is approximately 295 MiB and remains outside the repository. Its location has been checked; detailed frame/timestamp review is pending. Use it as context for interactions, motion and navigation alongside the measured Figma references, and as a local playback sample for native validation.
- [User's accessible Figma copy](https://www.figma.com/design/zdNUYRVOH9lEtIxMYvh8DV/VLC-Redesign--Community-).
- Artwork/design attribution: CC BY 4.0. Inter's SIL font license is bundled.
- VLC code retains its existing GPL licenses and VLC/VideoLAN author notices.

Credits are already prominent in the fork's README, including on its default
branch. Keep them in future documentation and distribution. Preserve original
SVG exports; use the native models for dynamic behavior. The new code is an
independent implementation of the designer's work.

## Completed implementation

| Area | Completed behavior |
| --- | --- |
| Playback bar | Rectangular transport buttons, 28px timeline, time labels, cone volume/mute, audio/subtitle entry points, settings and queue toggle. A second timeline row is used below 860 logical pixels. Original SVGs and Inter are packaged locally. |
| Native playback integration | Existing playlist/player controllers and native seeking, chapter snapping, tooltips, keyboard/wheel controls, engine updates, disabled actions and fullscreen access. Pinned fullscreen controls remain usable. |
| Playback settings | Working interim native menu for speed, aspect ratio, crop, stereo/audio mix, effects and track synchronization. This is not yet the final Figma settings sidebar. |
| Queue presentation | Original Queue, Bookmarks and Chapters tabs, compact rows, original icons, measured geometry and 400px canonical panel width. Player overlay margins, resizing and style-switch view retention are implemented. |
| Queue behavior | Native selection, filtering, sort/repeat/shuffle, file opening, removal, reordering and drops. Filtered indexes map to the underlying playlist; footer/toggle drops append to the full queue. |
| Bookmarks | Native media-library add/remove/seek, millisecond labels, playback/seekability/readiness gates, reset-aware selection and touch/accessibility activation. Missing media library is handled explicitly. |
| Bookmark backend | Appended numeric `timeMs` role, invokable actions, index/null guards, deduplicated removal, safe listener handling and rejection of obsolete async results. Media indexing and continuous-playback races were reviewed and corrected in source. |
| Chapters | Native chapter model and selection, numbered rows, full `HH:MM:SS` times, current-chapter feedback and separate accessible selection state. |
| Compatibility | `--no-qt-juxtopposed-controls` restores traditional toolbar/queue appearance. New appearance is Windows-gated; other platforms and the library mini player keep their existing controls. |
| Build integration | New QML/resources registered in both Autotools and Meson. Original SVG bytes are protected from Git newline conversion by `.gitattributes`. |

Active repeat/shuffle feedback and unspecified hover/selection/focus states
use implementation adaptations. Their exact Figma likeness has not been graded.

## Verification completed, and its limits

At implementation checkpoint `753acdc`, all **six Qt 6.8.3 component/regression
suites passed locally and in Windows GitHub Actions**. All 17 affected QML files
parsed successfully. [Successful CI run](https://github.com/KoalaNalle/vlc-juxtopposed/actions/runs/38389157833).

On **13 October 2026**, the home Windows x64 debug build completed. An
incremental native build and portable packaging both returned exit 0. The
packaged executable then passed a Windows dummy-interface startup/quit check
with a fresh plugin scan: **408 plugin DLLs, 600 registered modules, zero
plugin loading errors**. This confirms basic native startup; it does not
establish full Qt UI behavior or real-media playback acceptance. The subsequent
GUI and media smoke checks below cover a limited native subset.

### Native GUI fixes and smoke checks on 13 October 2026

The gray window was caused by QML dependency loading failures. Five redesigned
components now import `QtQuick.Controls.Basic`, matching the styled controls
actually registered in VLC's static Qt build. Reused `PlaybackSpeed.qml` now
qualifies `T.ButtonGroup` and its attached group; `ComboBoxExt.qml` explicitly
uses Basic `Popup` and `ScrollIndicator`. `ControlBar.qml` imports
`VLC.PlayerControls` for the native `PlayerControlbarModel.Miniplayer` enum.
The queued cached-renderer callback in `qt.cpp` copies its block-local values
instead of retaining references after those locals expire.

The Qt plugin was incrementally rebuilt and staged into the portable package,
with build exit 0. These fixes were tested as uncommitted changes on checkout
`561fd18388`. The original core build provenance remains recorded below.
The initially locked old Qt DLL was preserved by renaming before replacement;
remaining old copies are outside the runnable package in the ignored
`build-windows/previous-interface-builds` directory.

All **six component suites passed again on Windows** with Python 3.12.14 and
PySide6 Essentials 6.8.3. The existing settings suite now loads the production
speed and combo dependencies, covering grouped speed presets, engine updates,
the real combo popup/model rows and its styled indicator. In-memory rollback
checks reproduced the original missing ButtonGroup, Popup and ScrollIndicator
errors. Final local logs/previews: `build-windows/ui-verification/final`.

Actual Windows GUI checks verified:

- Home renders with the media library both disabled and enabled, including
  repeated starts using cached D3D11 renderer settings.
- The supplied 3840 × 2160 AV1 reference video decodes and displays. Keyboard
  and mouse play/pause and mouse mute/unmute update the native player and
  visible controls. A paused timeline seek from approximately 01:45 to 03:52
  updates its requested position/image, but triggers the core fault below.
- The redesigned queue opens with the actual media row. Queue, Bookmarks and
  Chapters tabs render. Bookmarks reports its empty state with Add enabled;
  bookmark creation/removal has not been exercised in this native pass. This
  sample has no chapters, so real chapter selection remains untested.
- The native playback settings menu and aspect-ratio dropdown render with
  real choices. Native speed/aspect/crop changes and focus/navigation acceptance
  remain to be exercised.

Native captures/metadata are saved locally in
`build-windows/native-ui-verification`; logs include
`native-gui-controls-fixed.log` and `native-media-gui-final.log`.
**The video itself contains the designer's prototype UI. That in-video UI is
reference content, not implemented app UI.** Assess only the actual window
chrome, playback bar, right queue and live settings popup in media captures.
No overall native visual score has been assigned.

This is still a debug test build. A paused seek triggered **73 clock/PCR
lateness errors with repeated clock/audio resets and rebuffering**, with no
return to PLAYING before the test was stopped. Stable paused seeking fails
this native pass even though its requested position/image update succeeds.
Independent log/source triage points to core paused-clock accounting: the
buffering reset in `src/input/es_out.c` sets the origin to the saved pause
time, while later PCR updates use current wall-clock time. This is a likely
cause, not a completed fix. Reproduce independently with traditional controls,
repair the clock path and retest before full playback acceptance.
The test media was stopped and VLC was left open on Home with its queue.
Startup also logs transient SearchBox null-parent bindings and
recursive layout warnings. A source audit additionally identified missing
control imports in the lazy Toolbar Editor path (`WindowDialog.qml`'s
DialogButtonBox and `ToolbarEditorDialog.qml`'s ItemDelegate); those were kept
outside this startup fix. Fullscreen/DPI variants, bookmark lifecycle races,
real chaptered media and the rest of the checklist remain open.

The tests cover playback/seek/settings actions, engine/model synchronization,
real Qt proxy mapping, drops, silent selection resets, bookmark action gates,
time formatting, duration updates without row-count changes, retained views,
production overlay anchors and component UI scaling.

Independent visual review of isolated 400 × 946 canonical components:

| Capture | Score over the assessed 85% of the rubric |
| --- | --- |
| Queue | 9.64/10 |
| Bookmarks | 9.61/10 |
| Chapters | 9.70/10 |

All three pass the 9.5 canonical target. Final captures are byte-identical to
the independently reviewed snapshots. Small text raster/advance differences
of 1–3px remain. These are component scores, not a score for the entire app.
The combined state/responsive category and full native behavior remain
unassessed. Source reviews at the implementation checkpoint found no remaining
actionable static finding; the later native pass above found runtime problems
and does not replace comprehensive acceptance.

Portable evidence is committed:

- [Three-tab preview](doc/juxtopposed-references/queue/review/queue-final-visual-review-production-contact-sheet.png).
- [Original references, geometry and asset hashes](doc/juxtopposed-references/queue/manifest.json).
- [Independent visual review](doc/juxtopposed-references/queue/review/queue-final-visual-review.json).
- [Bookmark backend review](doc/juxtopposed-references/queue/review/bookmark-backend-review-final.json).
- [Native adapter review](doc/juxtopposed-references/queue/review/queue-native-adapter-review-final.json).

## Start on the home computer

If the checkout already exists, run in **PowerShell**:

```powershell
Set-Location -LiteralPath 'C:\modding\vlc-juxtopposed'
git status --short
git fetch origin
git switch juxtopposed-playback-bar
git pull --ff-only
git rev-parse HEAD
```

Preserve any uncommitted home changes before switching/updating. If the branch
exists only on the remote, `git switch --track origin/juxtopposed-playback-bar`
is the explicit alternative. If Git reports divergence, inspect the commits
instead of resetting the checkout.

For a fresh checkout, use this fork and preserve Unix shell-script line endings:

```powershell
git clone -c core.autocrlf=false --branch juxtopposed-playback-bar https://github.com/KoalaNalle/vlc-juxtopposed.git C:\modding\vlc-juxtopposed
```

Open this folder as the new Codex project and read this handoff. Check that
`753acdc` is present in its history. Use the latest branch tip; the baseline
hash is for provenance, not an instruction to discard subsequent work.

Existing Windows clones may contain CRLF shell scripts. Check line endings
before compiling under a Unix shell. Changing `core.autocrlf` alone does not
rewrite an existing checkout; preserve local work before arranging an LF
checkout. Keep Git configuration changes scoped to this repository.

## Run the portable component checks

The CI uses Python 3.12 and `PySide6-Essentials==6.8.3`. This Python Qt runtime
is for tests; VLC's native build obtains its own Qt dependencies through its
build/contrib system.

Example **PowerShell**, with the environment and previews outside the source tree:

```powershell
Set-Location -LiteralPath 'C:\modding\vlc-juxtopposed'
py -3.12 -m venv C:\modding\vlc-juxtopposed-ui-env
$uiPython = 'C:\modding\vlc-juxtopposed-ui-env\Scripts\python.exe'
& $uiPython -m pip install PySide6-Essentials==6.8.3
& $uiPython modules/gui/qt/tests/test_juxtopposed_playback_bar.py --output-dir C:\modding\vlc-juxtopposed-ui-previews
& $uiPython modules/gui/qt/tests/test_native_seek_handle.py
& $uiPython modules/gui/qt/tests/test_playback_settings_choices.py
& $uiPython modules/gui/qt/tests/test_playlist_proxy_mapping.py
& $uiPython modules/gui/qt/tests/test_juxtopposed_queue.py --output-dir C:\modding\vlc-juxtopposed-ui-previews\queue
& $uiPython modules/gui/qt/tests/test_juxtopposed_queue_overlay.py
```

Reuse a suitable existing environment if available. Check each command's exit
status. Passing these suites establishes component behavior; it does not build
VLC's C++ code or exercise actual playback.

## Compile the Windows desktop application

### Choose and prepare the environment

Read the checkout's [Windows build guide](doc/BUILD-win32.md) and
[build script](extras/package/win32/build.sh). The
[upstream guide](https://github.com/videolan/vlc/blob/master/doc/BUILD-win32.md)
also documents the supported approaches. It recommends WSL; MSYS2 and Docker
are alternatives. The Windows target uses MinGW-w64 with GCC or LLVM, through
a Unix shell. MSVC/clang-cl are not supported by this guide.

The home Codex should first inspect the existing WSL/MSYS2/Docker setup and
toolchain, then follow the guide's prerequisites for that environment. Use the
fork/branch above as the source. Check the compiler and C++ compiler versions
and whether available prebuilt contribs match them. The guide includes older
example toolchain versions; select compatible dependencies deliberately.

### Home environment prepared on 13 October 2026

The home machine now has a working WSL 2 Ubuntu **24.04.5 LTS** environment.
AMD SVM virtualization was enabled in the ASUS TUF GAMING B550M-PLUS firmware.
The Windows build number is `26200.9550`; WSL runs kernel
`6.18.54.1-microsoft-standard-WSL2`. The host has 32 GB RAM and 16 logical CPUs;
Ubuntu currently sees about 15 GiB RAM.

Installed build prerequisites include native GCC/G++, Autotools, CMake
**3.28.3**, Python **3.12.3** and its build helpers, archive utilities,
`dos2unix` (needed by portable packaging), Java/Ant and MinGW-w64 x64
**GCC 13 POSIX**. VLC's extra-tools stage successfully built its required
Meson **1.12.0** and patched Ninja **1.13.2** locally. Qt remains the contrib
system's pinned **6.8.3** dependency.

The source checkout was normalized to LF where the committed file already
uses LF, and repository-local `core.autocrlf=false` is set. Git's cached
normalization metadata was refreshed; both staged and unstaged code diffs
were empty afterward. Files committed with CRLF were preserved.

A small Windows x64 executable compiled and ran under WSL. A second check
ran it from the Linux build directory and verified Windows file access to
that directory through a WSL UNC path. These are environment checks, not
VLC playback acceptance.

The first full build was started from source checkpoint `1b4948c322f46d106c0890c80dd868fa80a72b5e`,
with `JOBS=8`, `MESON_BUILD="-j 1"`, default debug/Qt/MSVCRT/Autotools settings,
and **without `-p`**, so contrib libraries use the same compiler as VLC.
Its state at this handoff update is **native compilation and portable
packaging verified**. The packaged executable now passes basic native startup
and Qt GUI loading. Initial video/control checks work; comprehensive media
and interaction acceptance remains pending.

The first contrib fetch stopped because both configured QtShaderTools
download locations returned 404. Qt 6.8.3 now lives in Qt's official archive;
the common Qt download URL in `contrib/src/main.mak` was corrected to use
that archive, retaining the same version and checksum manifests. The missing
QtShaderTools and QtSvg archives were recovered and passed their existing
SHA512 checks. Independent inspection also verified all 69 previously cached
archives with SHA512 manifests. The resumed build completed.

The original wrapper/supervisor were edited while Bash was reading them.
Compilation and packaging finished, but the changed file offsets caused
unmatched-quote errors at wrapper shutdown, so no completion status was
saved. A separate unchanged verification script subsequently ran the native
incremental build and `package-win-common`, both successfully. Its log is
`build-windows/final-package-verification.log`; `build-status.txt` now records
`state=succeeded`, `exit_code=0`. Future launches use frozen per-run copies
under `build-windows/runs/` to prevent this failure.

The startup check found four plugins importing a shared GCC exception
runtime that VLC's restricted Windows plugin loader would not find. Windows
GCC-only post-install fixes in `contrib/src/srt/rules.mak`,
`contrib/src/x265/rules.mak` and `contrib/src/gme/rules.mak` now normalize the
installed `.pc` metadata from `-lgcc_s` to `-lgcc_eh`, preserving static runtime
linkage and VLC's existing loader policy. The corresponding generated flags
were corrected and only the SRT access/output, x265 and GME plugins were
relinked. No Qt rebuild was needed. A fresh native startup scan then passed
with no missing plugin dependencies. Logs: `native-startup-verified.log` and
`final-package-verification.log` in the local build artifacts directory.

Current local locations (the helpers and logs are ignored build artifacts):

- Linux intermediates: `/root/vlc-juxtopposed-build`, with VLC in its `win64` subdirectory.
- Setup helper/log: `C:\Modding\vlc-juxtopposed\build-windows\setup-ubuntu.sh` and `setup-ubuntu.log`.
- Build helper/log: `C:\Modding\vlc-juxtopposed\build-windows\build-windows-x64.sh` and `build-windows-x64.log`.
- Supervised run: `build-windows\build-supervisor.sh`, launched by a hidden Windows `wsl.exe` process; `launcher.json` records the launcher PID/start time and `build-status.txt` records running/succeeded/failed and the exit code. Cross-check the process when status says running, since an interrupted host cannot always write a final status.
- Verified portable output: `C:\Modding\vlc-juxtopposed\build-windows\package`, containing `vlc.exe`, matching core DLLs and 408 plugins. Qt GUI loading is fixed; this is a debug test package with full runtime acceptance pending.
- GUI rebuild helper/log/status: `build-windows/rebuild-native-gui.sh`, `native-gui-rebuild.log`, `gui-build-status.txt`. It stages the rebuilt Qt DLL separately; close the portable app before replacing the package's active DLL. Never edit a running helper.
- Convenient local launcher: `build-windows/Launch VLC Juxtopposed.cmd`. It opens the portable executable, disables single-instance handoff and keeps 4K video from automatically expanding the window beyond the screen.
- Isolated test settings: `build-windows/package/portable`. VLC detects this directory and keeps its config/cache/media-library state separate from installed VLC. Keep legacy VLC installed while validating this fork.

Do not start a second build while another is running; build-directory locks
reject duplicate launches. To start a future build in a visible terminal
using frozen helper snapshots, run from PowerShell:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File C:\Modding\vlc-juxtopposed\build-windows\start-visible-build.ps1
```

The helper deliberately clears exported `CC`/`CXX` before invoking VLC's
script. Linux helper tools require native compilers; VLC subsequently chooses
the installed Windows cross-compilers itself. An initial attempt exported
the cross-compilers too early and failed in the Linux Ninja build. Its
configuration was preserved under `extras/tools/ninja/vlc_build-cross-compiler-attempt`
in the Linux output tree, the wrapper was corrected, and the subsequent
helper build passed. Both attempts are retained in the appended build log.

### Example WSL build with a separate output directory

After prerequisites and LF source files are ready, run these commands in
**Bash inside WSL**, using the Windows checkout mounted at `/mnt/c/modding`:

```bash
mkdir -p /mnt/c/modding/vlc-juxtopposed-build
cd /mnt/c/modding/vlc-juxtopposed-build

# Inspect available options; this script returns exit 1 even for its help text.
bash ../vlc-juxtopposed/extras/package/win32/build.sh -h

# Build debug Windows x64, attempt prebuilt contribs, and assemble a runnable folder.
set -o pipefail
bash ../vlc-juxtopposed/extras/package/win32/build.sh \
  -a x86_64 -p -o "$PWD/package" 2>&1 | tee build-windows-x64.log
```

These are preparation/build instructions, not a previously successful native
build recipe on this home machine. Resolve environment or source errors from
the actual logs. The script builds tools/contribs, bootstraps, configures and
compiles VLC. `-o` supplies an absolute output folder and invokes packaging of
the executable, DLLs and plugins for testing.

- Keep the Qt GUI enabled. `-z` builds without it and cannot validate this UI.
- `-p` attempts prebuilt contribs. Download failure falls back to source builds;
  compiler/ABI incompatibility may require choosing a matching toolchain or
  rebuilding contribs without `-p`.
- Start with the default debug build. Installer generation is a later step.
- The default x64 build directory under this output root is `win64`.
- `-m` selects Meson (`win64-meson`); `-u` selects UCRT and requires a compatible
  toolchain. Keep the first attempt on one route rather than mixing outputs.
- Record the tested commit, toolchain, Qt/contrib versions, build command and
  first meaningful error. Preserve the full build log.

If WSL filesystem performance becomes an issue, a Linux-filesystem checkout
and build can be considered. Keep its commit synchronized with the authoritative
Windows checkout and record which source tree produced each binary.

### Launch and validate the resulting binary

The verified home package is currently
`C:\Modding\vlc-juxtopposed\build-windows\package`. The local launcher
`C:\Modding\vlc-juxtopposed\build-windows\Launch VLC Juxtopposed.cmd` opens it
alongside legacy VLC using the isolated portable settings directory. Continue
the remaining native acceptance checks. The paths below belong to the
generic example build, not this verified local package.

After the example build succeeds, the requested portable folder is
`C:\modding\vlc-juxtopposed-build\package`. Confirm the actual output exists and
contains `vlc.exe` with its matching DLLs/plugins. Close other VLC instances,
then launch that executable from **PowerShell**:

```powershell
Set-Location -LiteralPath 'C:\modding\vlc-juxtopposed-build\package'
& .\vlc.exe
```

Confirm this build uses the redesigned Qt interface. Test the opt-out in a
separate run after closing the first instance:

```powershell
& .\vlc.exe --no-qt-juxtopposed-controls
```

Use the full [Windows build/playback checklist](doc/juxtopposed-windows-build-checklist.md).
The first native acceptance pass must cover:

1. Real video/audio playback, seeking, volume, tracks, settings and fullscreen
   with both pinned and floating controls.
2. All three queue tabs, native sort/search, filtering, selection, removal,
   reordering, drag/drop, keyboard/touch input and accessible activation.
3. Bookmark readiness, delayed first-play indexing, rapid media switches,
   continuous A-to-B playback, stop/restart, removal and selection resets.
   Test with the media library enabled and with `--no-media-library`.
4. Real chapter selection/current state, unknown durations and long time labels.
5. Queue docking/detaching, narrow/short windows, 100/150/200% Windows scaling,
   native compositing and the legacy opt-out.

Capture screenshots from the actual VLC Windows window and record media,
Windows/Qt versions, display scale and reproduction steps. Native build and
runtime acceptance remain open until those checks are actually performed.

## Advanced reference and audio export checkpoint

The [advanced audit](doc/juxtopposed-references/advanced/reference-notes.md)
preserves thirteen original video frames with hashes. Home, music waveform,
subtitle scope/presets/timeline, New Playlist, Open Network Stream and profile
menus are confirmed. The former conversion attribution near 13:10 was actually
Open Network Stream; conversion dialog geometry remains unresolved. Exact UI
geometry, assets and unshown states still need references; milestone 0 remains
in progress.

The [audio export checkpoint](doc/juxtopposed-references/advanced/audio-drain-checkpoint.md)
repairs the copy packetizer's lost final packet and the transcode resampler's
undrained tail. The same native diagnostic fails against the preceding plugins
and passes after repair: complete 30-second PCM at 8/44.1 kHz, byte-identical
source-rate PCM, valid WAV and 300 actual waveform peaks. The copy-module
ownership contract and clock/ES/pipe/equalizer/video/packaged-seek regressions
pass. Both plugins build and are staged in the separate slice package with
rollback copies; the working home package is preserved. The asynchronous model,
player waveform GUI and conversion journey are still incomplete.

The following [waveform checkpoint](doc/juxtopposed-references/advanced/waveform-checkpoint.md)
builds a single-worker native decode, cancellation/generation guards, bounded
file-identity cache and rounded peak bars on VLC's existing slider. Eight native
QtTest checks pass: full audio, silence/four-sample files, errors, source changes,
cache invalidation, shutdown and independence from normal/paused primary audio.
All nine component suites pass, including peak colours/height/seek/scaling and
fallback. The native Qt plugin builds and is staged with its preceding version
recoverable. This working path supports local single-track audio; cover/list
layout, track/codec variants and desktop/visual acceptance remain open.

The [network dialog checkpoint](doc/juxtopposed-references/advanced/network-dialog-checkpoint.md)
builds multiline validation, actual ordered queue/playback and optional named
playlists. Six native checks pass, including real HTTP audio and database restart
persistence; all ten component suites pass, including modal shortcut suppression,
failure/retry and short/scaled layouts. The new Qt plugin is staged separately
with its preceding version recoverable. External protocol/failure variants and
native desktop/visual acceptance remain open.

The old GUI and Firewall prompt were no longer visible in the morning inventory.
A new diagnostic launcher used an incorrect option/helper path and was corrected.
The visible launch exposed `Canvas is not a type` in the new dialog's close glyph;
the static Qt build omits Canvas. The glyph now uses ordinary Rectangle items.
PageExt's read-only Loader.implicitHeight assignment also left Home blank; a
writable height binding repairs page construction. The final rebuilt plugin now
renders Home/header/Open File/Add Folder and opens the actual dialog via Ctrl+N.
Invalid entry, disabled Save, dismissal and reset/reopen pass on Windows. Complete
GUI submission/playback and library/media journeys still require acceptance.
No security permission choices were made. Next: complete the native journeys and
port music cover/list and subtitle presets, with the full Home/conversion/preferences
inventory. Keep the full 0–8 Goal active and draft PR/visual gates open.

The following [native desktop checkpoint](doc/juxtopposed-references/library/desktop-checkpoint.md)
repairs Add Folder's uninitialized QWidget parent and protects editable QML focus
from global application shortcuts. The actual picker now registers the fixture
folder; two videos/one audio appear and remain after normal shutdown/restart.
Native Videos search/clear and actual video activation/rendering pass. All ten
component suites pass, including real Paste suppression/restoration. The final
Qt plugin is staged with rollback available. Playback bars were absent from the
video snapshots; visibility/input/geometry/compositor diagnosis and the complete
return/resume journey remain open. No full native journey or visual score is claimed.

A stronger [Home audit](doc/juxtopposed-references/home/reference-notes.md) preserves
whole-grid/list/lower-section/pin frames and confirms Continue Watching, Listen
Again, Frequent Folders/Playlists/Artists. Native visit counts, complete artist
aggregation and recent-audio history are required rather than relabelled favorites.
Music-folder presentation/backend, Home/dashboard/activity and subtitle presets
have separately prepared source/integration patches under ignored build-windows;
their registered sources/native builds are not yet integrated into this checkpoint.

The subsequent [native controls checks](doc/juxtopposed-references/runtime/player-controls-verification.md)
verify the same built Qt plugin's ordinary floating controls: auto-hide,
keyboard/pointer restoration, held red-to-blue seeking, Back with the retained
mini-player position, player switching and actual resume from six to eight
seconds. Fullscreen entry, restored controls and exit also pass. No compositor
order change was needed. Native narrow/DPI, settings/chapter/bookmark journeys
and visual acceptance remain open. The music-folder/cover source is now integrated;
its native build/testing is in progress. Home/presets remain separate pending ports.

The [music checkpoint](doc/juxtopposed-references/advanced/music-player-checkpoint.md)
now builds and runs the actual local source-folder/cover/waveform screen. Seven
native folder checks and six native timer checks pass; all eleven component
suites pass. Desktop search/clear/queue fallback, actual waveform seeking,
resume/Back retention and audio-to-video replacement pass within the recorded
scope. Initial paused audio now displays zero time and its real duration after
repairing an uninitialized timer point and missing early length notification.
The final Qt plugin is staged separately with rollback. Missing-artwork contrast
and overlapping legacy title are repaired. Actual album-art blur, native narrow/DPI,
other codecs/tracks, full settings/lyrics journeys and visual acceptance remain
open. Integrate the prepared five-section Home next; subtitle preset peer-review
repairs and New Playlist integration remain pending. Continue the full 0–8 Goal.

The [five-section Home checkpoint](doc/juxtopposed-references/home/implementation-checkpoint.md)
now connects actual video/audio history, validated folder/playlist visits and
complete native artist activity. Seven real store/snapshot checks and all twelve
local component suites pass; the native Qt plugin builds. The initial Home build
renders real history/artist rows, but a Windows firewall prompt blocks desktop
interaction and needs manual user dismissal. No security choice was made. The
final bounded-preview plugin is saved separately; full Home routes/visits/restart,
live Clear History/reset and visual acceptance remain pending. Music's clean-tree
test repair passes both exact-head eleven-suite Windows workflows. Subtitle
presets, New Playlist, cue timeline and conversion continue as separate work;
the full milestones 0–8 Goal remains active.

The [subtitle preset checkpoint](doc/juxtopposed-references/advanced/subtitle-preset-checkpoint.md)
now builds with persistent bounded presets, actual ordered track restoration,
appearance/duration snapshots and per-media associations. Ten native preset
checks, eight actual normal-lock PlayerController checks, the existing eleven
appearance regressions and all thirteen component suites pass. Native confirmation
now submits writes once, preventing the reproduced input-control FIFO retry storm.
Secondary delays now inherit the category until overridden and publish actual
effective values after unlocking; stale GUI roles cannot retarget changes.
The final plugin is preserved in a separate `build-windows/slice-presets-package`;
native menu/save/restart and visual acceptance remain pending. Both exact-head
  twelve-suite Home CI runs passed. Both firewall prompts were manually dismissed.
  The [Home desktop checks](doc/juxtopposed-references/home/desktop-checkpoint.md)
  reproduced and repaired an inactive history model's `ml: null` assertion, then
  passed actual video/audio See All, search/clear, alphabetic sorting, view switching
  and audio seek/resume/Back/retained-player return. The final plugin hash is
  `364bfe40d5af74435b08421a6fe1032fa8338158c8f208cda3652e1b3a500abe`.
  The actual pinned subtitle settings change a held caption from green to red;
  native footer-menu Save/apply/restart remains pending separately from the service
  tests. Current test preferences are portable private copies. No security choice
  was automated. Prepared independent repairs cover New Playlist unsafe paths and
  thumbnail previews, audio-only Home folder bootstrap, dark missing-art and cue
  seeking. Integrate/build them sequentially, then conversion and the full 0–8 Goal.

## Remaining design work and proposed order

The [vertical slice implementation plan](doc/juxtopposed-vertical-slice-plan.md)
now defines the proposed breadth-first coverage ledger, connected user journeys,
milestones and overnight continuation approach. Its first execution target is
reference inventory, stable runtime and a shared shell/Home slice. The full
milestones 0–8 Goal is active; no separate follow-up schedule has been created.

1. **Reference inventory and stable runtime.** Catalogue every original screen,
   panel, dialog and reusable control. Reproduce and repair paused-seek clock
   behavior, preserve the current working GUI baseline and verify affected paths.
2. **Shared shell and Home.** Port common navigation, header and Home around real
   native library data. Deliver add/open/return/resume as the first connected slice.
3. **Library and discovery.** Port confirmed video, music and playlist layouts,
   search/filter, artwork/thumbnails and source-opening routes.
4. **Player and queue completion.** Finish tracks, timeline chapter/bookmark marks,
   fullscreen and bookmark/chapter native acceptance. Assess transparency and the
   queue's unimplemented 50px reference blur using the actual compositor.
5. **Settings and effects.** Port the final panels while preserving native choices,
   effects and synchronization behavior from the interim controls.
6. **Advanced media surfaces.** Implement one real confirmed path through subtitle
   presets/timeline, waveform and conversion/export, with required backend support.
7. **Preferences and appearance.** Complete persistent settings, dialogs and light
   appearance across all implemented surfaces.
8. **Integrated and visual acceptance.** Run the full native journeys, review real
   Windows captures and close the coverage ledger. Assess responsive/DPI states
   separately before accepting the portable build for broader use.

Confirm the reference for each new surface before implementing its appearance.
Do not treat the whole Figma concept or the current Windows app as finished.

## Agreed implementation/review workflow

The user wants implementation agents and separate visual/functional evaluators.
Use them for suitable independent subtasks. Give file ownership to implementers
and keep evaluations independent of the implementer's self-score.

1. Preserve the original reference/node ID, dimensions, fonts, colors and assets.
2. Implement a small surface and bind it to existing VLC models.
3. Render fixed reference content, then compare aligned captures and overlays.
4. Independently audit native actions/lifecycle and run meaningful regressions.
5. Fix ranked defects and have the evaluators review the corrections.
6. Integrate resources in both build systems, test, commit and update this file.

Target canonical visual likeness **at least 9.5/10** and assessed variants
**at least 9/10**, with no critical defects. Rubric: layout 35%, typography 20%,
surfaces 15%, artwork 15%, state/responsive fidelity 15%. Explicitly exclude
unassessed categories when normalizing a partial score. Functional acceptance
and full native build/playback acceptance are separate gates.

Figma MCP hit its account call limit earlier. Manual Design Mode inspection and
original asset exports were authorized and successfully used. MCP reads now
work after the user's Professional Full-seat connection; this does not accept
the current app's visual fidelity. Follow the applicable Figma skills, reuse
saved references, and obtain sufficient high-fidelity context before new UI
edits. Keep original SVGs unchanged and retain attribution.

Windows development firewall setup is now complete for the explicitly named
test programs and package paths: Private TCP/UDP incoming traffic is limited to
the local subnet; Public/Domain incoming traffic is blocked. Existing automatic
query rules for those exact programs were disabled to avoid conflicting rules.
All firewall profiles and checked Defender protections remain enabled. The
setup/Undo script and host-specific audit files remain ignored under
`build-windows`; new executable paths are not automatically covered. A fresh
Home native helper run after setup again passed all 11 cases. The complete
Home worker/picker desktop journey remains a separate acceptance gate.

## Latest native playlist and Home checkpoint

The [New Playlist checkpoint](doc/juxtopposed-references/advanced/new-playlist-checkpoint.md)
now has a production modal, native image/media pickers, ordered mixed/repeated
files, optional copied artwork and native library routing. The final native
suite passes 10 cases, including real external files with unknown duration;
all 14 component/source suites pass. Actual desktop creation, reorder, saved
contents and Home cover/content persistence after restart were observed.
The rebuilt desktop now displays `--:--` for unknown row and all-unknown playlist
durations. Actual drop, active-primary-player preservation and full error/DPI/
reference acceptance remain open. Synthetic ML model additions compiled, but
the existing Windows stub initializer prevents that separate model test from
running; it is not a passing check.

The [Home folder-bootstrap checkpoint](doc/juxtopposed-references/home/folder-bootstrap-checkpoint.md)
adds explicit audio-only Add Folder visit registration and preserves accepted
activity after actual settings write/clear failures. All 11 native Home cases
pass with real indexing/events/restart and separate request/store helpers.
The complete worker/picker/Home journey remains a desktop gate. The built Qt
DLL SHA-256 is `88452491563814608b6d4790752aa4ad9875204780e35e95d5886ab69aed99c6`.
These are incremental checkpoints; the [beta plan](doc/juxtopposed-beta-plan.md)
still has open functional, visual and final-package gates.

## Conversion and real-media verification in progress

The [shared-modal native proof](doc/juxtopposed-references/native/shared-modal-20261014/README.md)
records the same frozen harness failing the unguarded modal key route and
passing all six cases with production guards. The
[desktop conversion proof](doc/juxtopposed-references/native/conversion-desktop-20261014/README.md)
records actual Windows pickers, a completed FLAC conversion, Completed-state
keys/Done, normal exit and an independent full 1,323,000-frame mono PCM match.
The new Qt checkpoint contains those sources and proof bundles.
Encoding-state keys, full queue/drop/lifecycle, other conversion profiles,
original Figma form and final-package acceptance remain open.

The user authorized read-only testing with `D:\Media` and its subfolders.
Private inventory, paths, metadata and actual-media observations stay ignored
under `build-windows`. Eight selected originals cover embedded ASS/text/PGS,
multiple audio tracks, AVC/HEVC/legacy AVI and MP3/FLAC. The corrected native
smoke passes all six video cases, including track switching and actual decoded
video/PCM. MP3 and FLAC fail its precise paused `get_time` seek-completion gate;
the unchanged failed oracle and separate clock diagnostics are retained. Actual
desktop multi-audio/subtitle selection, HEVC/ASS rendering and clearing, and
FLAC paused-seek display/resume were observed. Separate native traces confirm
accepted audio seeks and actual target clocks after resume; paused getters
retain the older output clock without a new audio timing point. Their strict
paused-clock gate remains failed. Caption redraw still needs packet-timing
investigation. Original size/mtime was
preserved; no full-original hash or all-format/full-file acceptance is claimed.

The first Home route guard rebuild allowed Videos-to-Home and Back/Forward,
but a later playlist journey still failed and the closed log retained five
`initialIndex` property errors. Generic nested property forwarding and real
view restoration require further repair and a fresh desktop check. The first
guard build is not a passing complete-navigation checkpoint.

The later [leaf-property repair](doc/juxtopposed-references/native/navigation-leaf-20261014/README.md)
passes eleven paired component gates and nine actual QCore History results.
The [mounted Windows journey](doc/juxtopposed-references/native/navigation-desktop-20261014/README.md)
now opens Home/Videos/Playlists and real empty/mixed playlist details through
keyboard navigation. Back restores the second overview selection; Enter reopens
its same three ordered rows. The flushed log has no QML property/loader errors,
and normal quit returns exit 0. All seventeen local component/source suites
pass. The fresh Qt DLL is `03d93c91c056442d7392866eec4e39ca6f12f0fb745529fadb8a50b8adddd797`.
Videos pointer selection, broader Music/Network history, rapid same-route
generations, clean-profile/restart and native design fidelity remain open.

The [literal conversion-path repair](doc/juxtopposed-references/native/conversion-literal-path-20261014/README.md)
preserves percent characters despite inherited file-sink date formatting.
The same frozen native fixture reproduces 10 passes/1 failure before the
one-line fix and passes all eleven afterward. Both encode and validation sinks
keep literal paths; original conversion/cancel/race/primary/shutdown assertions
remain intact. The preceding desktop exact-PCM proof is retained separately.

The current subtitle service remains a bounded selected-local-UTF-8-SRT
snapshot adaptation. Its actual native twelve-case suite passes, including the
NORMAL-lock exact 4,352,555us request. The native bookmark/chapter suite passes
nine cases with real local ML and database restart, and the subtitle-delay
controller suite passes eight cases after waiting for actual output readiness.
These do not accept authoritative embedded-cue/renderer expiry, full native
panels, queued-worker lifetime or every subtitle format. The core cue bridge
proposal is still ignored and unbuilt; do not advertise it as implemented.

Visible tests now use monitor 2 at the user's request. No VLC test GUI remains
open after the recorded normal quit. The copied Qt test settings contained
personal recent-file history; that section was removed only from the new test
package, preserving its geometry and other settings. Seed future test settings
selectively, and keep all preferences/library/userdata out of public artifacts.
The installed VLC and original working package are preserved. Pending broader
history and playlist Play repairs live in ignored proposals and have not yet
changed this built source checkpoint. The full beta Goal remains active.

## Later Music/Network, playlist and Windows quit repair

The preceding Qt checkpoint was pushed as `909958b4a7eb23463cd7370ec135b88e568b93e3`.
Both associated GitHub Windows jobs passed all seventeen suites; the draft PR
records their complete Code Review diagnostics and separate native scopes.

The reviewed eight-file Music/Network restoration and two-file playlist Play
repairs are now integrated locally. Their supplied Qt gates pass twenty-seven
and thirteen cases respectively. The unchanged playlist oracle reproduces
exactly seven original failures: empty-card view changes and duplicate requests.
Its new never-cleared diagnostic ledger also covers startup, creation and
explicit QML-object/engine teardown. All nineteen local component/source suites
pass, including both newly registered CI launchers. These are component results.

The fresh incremental Qt DLL is
`05662582c5fac80f43c7b3e256034a53483eafabfc1aa650cec31d25479563a5`.
The build regenerated ignored `modules/gui/qt/Makefile.in`; original pre-build
and post-generation manifests retain that sole generated-file transition.
An initial staging rejection and unsuccessful launch attempts remain setup
history, with no accepted native result. Corrected staging and explicit
PowerShell launch started the actual test GUI as PID 29628.

Actual monitor-2 tests establish empty-card Play preserving the library and
paused chapter primary; one filled-card Play adds the three ordered playlist
entries once to the existing one-item queue. The four-row queue preserves the
repeated video, and those three short entries play through to normal return.
Albums opens the second real album's single waveform track; Home/Back visibly
restores that overview selection. Immediate Return from the History-button
focus did not reopen it, so completed refocus/activation remains unaccepted.
Artists opens the second genuine artist and returns to that same identity,
song count and album after Home/Back. Genres shows its actual empty state;
populated genre routes and broader model/view replacement remain open.

Network Browse reached standard folders/drives and started interface discovery,
which is separate from the disabled ML discovery flag. Windows then opened a
firewall prompt for this new executable path. The human allowed it. A later
administrator run applied and independently verified 36 owned rules for twelve
exact programs, including this path: Private TCP/UDP restricted to LocalSubnet,
Public/Domain inbound blocked, broad prompt-generated rules disabled. Firewall
profile and Defender settings were preserved. No permission handoff is pending.

The original Qt056 native run exposed two Network defects. A later list/grid
selection looked retained, but Home/Back reset it to row zero. Its flushed log
also records BrowseDeviceView calling forceActiveFocus on an undefined
currentItem. Ctrl+Q closed the window and the process reported exit status zero,
but the owned wait timed out; CIM still lists one thread and the GUI DLL remains
locked. An exact-PID taskkill reports no running instance. Exit status alone is
not proof of completed process termination. Preserve this shutdown failure.
Original captures and exact limits remain ignored in
`build-windows/music-playlist-desktop-v2-20261014`; Network/user-path captures,
preferences, databases and the full native log are excluded from public proof.

Root repaired MainViewLoader's cursor retention through view replacement and
changed BrowseDeviceView to use the existing grid/list setCurrentItemFocus
method. The loader test fixture previously restored its own cursor from the
selection, masking the actual grid's default-zero behavior; that convenience
was removed. The same stricter eleven-case test now fails before the source
repair and passes afterward. All nineteen current Windows component suites
pass again. The new incremental Qt DLL is
`ca793b4c45c54ada9ec6c0d00b7ef35ab07391541ec17034bc163eb6ccf84a68`,
with 1072 source identities frozen in
`build-windows/navigation-native-fixed-source-freeze-20261014.json`.

Staging initially rejected the retained process entry. A premature cache/launch
attempt started no app; a later guarded DLL replacement also stopped at the
Windows file lock. Preserve those setup attempts. The original plugin root is
unchanged. The same reviewed development executable now loads a separate
`plugins-navigation-native-fixed-20261014` root, with actual startup log evidence
of that directory. That native GUI was PID 30824, on monitor 2, with an
isolated generated-fixture profile under
`build-windows/navigation-native-fixed-desktop-20261014`.

The rebuilt pointer path opens the drive and authorized Media folder without
the prior QML error so far. Fourth-row grid selection survives list conversion;
a later seventh-row selection remains selected after returning to grid. However,
the next Home/Back returns to the parent drive instead of the Media subtree.
This history journey remains failed acceptance. Native NavigationHistory skips
frames whose NetworkTreeItem input pointer is no longer in its media tree;
whether asynchronous refresh invalidated this particular child is still an
inference requiring native reproduction. A later settled journey returned to
the same subtree and fourth selection after grid/list and Home/Back. Keep the
first failure and broader native tree lifetime gate open. Ctrl+Q still timed
out with an unsignaled process handle despite reported exit status zero.

The [new integrated checkpoint proof](doc/juxtopposed-references/native/navigation-play-shutdown-20261014/README.md)
preserves those failures and the strict paired 27-case Music/Network, 13-case
playlist and 11-case cursor checks. The original cursor fixture restored its
own index and masked the production default-zero bug; the stricter helper now
fails before the production repair and passes afterward. Nineteen exact local
component/source suites pass; the workflow now registers both new launchers.

A separate actual GUI playback/automatic-quit check reproduced the retained
process without Network browsing or Computer Use observation/input. Read-only
noninvasive debugging mapped its remaining thread into winpthreads TLS-key
deletion during the C++ static key destructor. A real Windows static-DLL fixture
reproduces the reader-lock/process-detach deadlock. Its original exit case times
out despite exit status zero; normal key operations and explicit FreeLibrary
pass. The proposed final-process-only wrapper passes all three cases, then an
actual Qt prototype fully terminates after playback. No forced cleanup counts
as acceptance, and the original failing processes/results are preserved.

The wrapper is now registered in both Qt build systems; Windows module link
flags are kept out of Meson test targets. Ordinary key deletion and explicit
unload use the real implementation. Two failed Automake setup attempts are
retained. The successful ordinary Autotools build produces Qt
`7c7f99054ee0017faf5cd2dd538f7775f3b45c4cf0505a173a54f733e13fddaf`.
Its final post-generation manifest freezes 1,073 Qt-directory files. Relative
to v2, Makefile.am was corrected before the final build and Makefile.in was
regenerated; no exact final pre-build manifest or clean beta build is claimed.
Meson registration is source-reviewed only.

The exact registered wrapper passes its three native DLL cases. Actual GUI
playback and automatic quit terminates in 10.828 seconds with exit zero.
Monitor-2 pointer input then opens the authorized subtree, keeps its fourth
selection through grid-to-list and Home/Back, and normal Ctrl+Q fully terminates
PID 26676: signaled owned wait, exit zero, no later process entry and released
Qt DLL file lock. Its flushed log has no QML property/call/reference errors.
The fresh app initially restored a 306x353 window before being maximized on
monitor 2; that geometry behavior remains open. Generated-profile tests do not
establish first-run/clean-profile acceptance. The 132 provenance pairs include
only generated-fixture captures; Network captures, full native logs, personal
media/preferences/databases remain private. The earlier Qt056 observation
booleans confusing exit status with termination are explicitly corrected in
this new proof, with the original identity and failing wait preserved.

One new Figma MCP design-context call cached original Global Settings node
`999:5047` at 1048x865, its screenshot and all twenty unchanged SVG assets.
Three assets have exact existing repository matches. No Global Settings UI has
been implemented from this new reference yet. Reuse VLC's `ColorSchemeModel`
for System/Day/Night; Figma's Dark/Light/System order differs from its native
indices. Full thirteen-category preferences, appearance, embedded cues,
renderer expiry, native focus/DPI/fidelity and final clean packaging remain
required beta work. Keep the draft PR and full beta Goal open; this scoped
incremental checkpoint does not accept the complete beta or final package.

The source/proof checkpoint was pushed as
`bf2c2e1a09192d7c57e8af2138810abf494ef3ae`. Both clean GitHub Windows jobs then
failed at the new Music/Network worker's startup. The downloaded artifact shows
an exact LF-only multiline Loader search failing against CRLF checkout bytes.
This was a fixture portability failure, not an accepted negative baseline.
The [test-only follow-up](doc/juxtopposed-references/native/navigation-play-shutdown-20261014/ci-newlines/README.md)
normalizes only the decoded parsing view while retaining unchanged copied QML
bytes/hashes, and exposes subprocess stderr in failing launcher logs. The old
worker reproduces the error against CRLF. The updated strict 27-case oracle
reproduces all original failures and all repaired passes for both LF and CRLF;
all nineteen suites pass again locally. Only the two Python helper files differ
from the native build source freeze; no compiled production source changed.
The preceding 132 published provenance pairs also match their committed Git
blobs exactly. Verify the follow-up head's CI through Code Review before
claiming GitHub acceptance; keep the first failed jobs/results intact.

## Code map

| Responsibility | Main locations |
| --- | --- |
| Playback presentation/adapters | `modules/gui/qt/player/qml/Juxtopposed*.qml`, `ControlBar.qml`, `SliderBar.qml`, `TopBar.qml`, `Player.qml` |
| Interim settings | `modules/gui/qt/player/qml/PlaybackSettingsMenu.qml` |
| Queue presentation | `modules/gui/qt/playlist/qml/JuxtopposedQueuePanel.qml`, `JuxtopposedQueueRow.qml` |
| Queue/chapter adapter | `modules/gui/qt/playlist/qml/PlaylistPane.qml`, `PlaylistDelegate.qml` |
| Optional bookmark adapter/backend | `modules/gui/qt/playlist/qml/JuxtopposedBookmarkList.qml`, `modules/gui/qt/medialibrary/mlbookmarkmodel.hpp/.cpp` |
| Context-menu source mapping/sort | `modules/gui/qt/menus/qml_menu_wrapper.cpp` |
| Window/library queue placement | `modules/gui/qt/player/qml/Player.qml`, `modules/gui/qt/maininterface/qml/MainDisplay.qml`, `PlaylistDetachedWindow.qml` |
| Windows feature flag | `modules/gui/qt/maininterface/mainctx.hpp/.cpp`, Qt module option registration in `modules/gui/qt/qt.cpp` |
| Artwork/font/credits | `modules/gui/qt/pixmaps/juxtopposed/`, `.gitattributes` |
| Resource/module registration | `modules/gui/qt/assets.qrc`, `Makefile.am`, `meson.build` |
| Tests and CI | `modules/gui/qt/tests/test_juxtopposed*.py`, `test_native_seek_handle.py`, `test_playback_settings_choices.py`, `test_playlist_proxy_mapping.py`, `.github/workflows/qml-ui-checks.yml` |

More context: [implementation notes](doc/juxtopposed-redesign.md) and
[home build checklist](doc/juxtopposed-windows-build-checklist.md).

## Suggested first message to the new Codex session

> Continue the Windows VLC Juxtopposed redesign in this repository. Read
> PROJECT_HANDOFF.md, doc/juxtopposed-vertical-slice-plan.md,
> doc/juxtopposed-redesign.md and the Windows build checklist.
> Inspect the branch, current commit, local changes and available build tools.
> Preserve the existing GUI fixes and reuse the configured WSL Windows build.
> Start with milestones 0 through 2: catalogue every reference surface, repair
> and verify the paused-seek runtime fault, then implement the shared shell/Home
> slice using real native library data. Run relevant component/native checks,
> capture the actual Windows UI and keep the coverage ledger current.
> Preserve Juxtopposed's design/artwork credits and existing VLC behavior.
> Keep native acceptance distinct from component scores, retain the draft PR
> until its native gates pass, and update this handoff with results and the
> next milestone. Goal/schedule activation remains a separate explicit request.
