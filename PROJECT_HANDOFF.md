# VLC Juxtopposed redesign — project handoff

Updated: **13 October 2026**. Read this file first when continuing from a new
Codex session on the home computer.

## Project and current checkpoint

| Item | Value |
| --- | --- |
| Fork | <https://github.com/KoalaNalle/vlc-juxtopposed> |
| Working branch | `juxtopposed-playback-bar` |
| Draft pull request | <https://github.com/KoalaNalle/vlc-juxtopposed/pull/1> |
| Latest implementation checkpoint | `753acdc251c7d94a5228ba1ed0a356ba80bfaac7` |
| Intended home checkout | `C:\modding\vlc-juxtopposed` |
| Initial platform | Windows desktop, x64 |
| Implementation | VLC's existing C++/Qt Quick/QML interface and native playback models |
| Immediate priority | Compile the complete Windows application and verify it with real media |

This handoff is added after the implementation checkpoint above. Fetch the
latest branch tip to receive this document as well as the code. The redesign
is an implementation in progress; no full VLC Windows binary has been compiled
or run by this project session yet. The PR remains a draft.

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
The combined state/responsive category, native compositing and real-media
behavior remain unassessed. Source reviews found no remaining actionable
static finding; they do not replace native compilation/runtime testing.

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
Its state at this handoff update is **compiling contrib libraries**; a
complete VLC executable and native runtime acceptance remain pending.

The first contrib fetch stopped because both configured QtShaderTools
download locations returned 404. Qt 6.8.3 now lives in Qt's official archive;
the common Qt download URL in `contrib/src/main.mak` was corrected to use
that archive, retaining the same version and checksum manifests. The missing
QtShaderTools and QtSvg archives were recovered and passed their existing
SHA512 checks. Independent inspection also verified all 69 previously cached
archives with SHA512 manifests. The resumed build is compiling dependencies.

Current local locations (the helpers and logs are ignored build artifacts):

- Linux intermediates: `/root/vlc-juxtopposed-build`, with VLC in its `win64` subdirectory.
- Setup helper/log: `C:\Modding\vlc-juxtopposed\build-windows\setup-ubuntu.sh` and `setup-ubuntu.log`.
- Build helper/log: `C:\Modding\vlc-juxtopposed\build-windows\build-windows-x64.sh` and `build-windows-x64.log`.
- Supervised run: `build-windows\build-supervisor.sh`, launched by a hidden Windows `wsl.exe` process; `launcher.json` records the launcher PID/start time and `build-status.txt` records running/succeeded/failed and the exit code. Cross-check the process when status says running, since an interrupted host cannot always write a final status.
- Requested portable output: `C:\Modding\vlc-juxtopposed\build-windows\package` (only usable after packaging succeeds).

Do not start a second build while the current one is running; build-directory
locks reject duplicate launches. After a
stopped build has been inspected, the prepared helper can resume it from
PowerShell:

```powershell
wsl -d Ubuntu-24.04 -u root -- bash /mnt/c/Modding/vlc-juxtopposed/build-windows/build-windows-x64.sh
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

## Remaining design work and proposed order

1. **Full Windows build and runtime fixes.** Resolve compile, resource-loading
   and native interaction issues first. Rerun affected component checks.
2. **Production visual review.** Compare real Windows captures with the saved
   references; assess interaction states and responsive/DPI variants separately.
3. **Finish playback/queue details.** The queue's Figma 50px background blur is
   unimplemented; native transparency remains unverified. Chapter segments and
   bookmark marks on the redesigned timeline are also pending.
4. **Final settings/effects sidebar.** Port the Figma layout while preserving
   the working settings actions already provided by the interim menu.
5. **Media-library views and navigation.** Sidebar, grids/lists, artwork,
   thumbnails and related flows still need their design port.
6. **Other concept surfaces.** Subtitle presets/timeline, waveform, conversion
   flows, preferences and a complete light theme remain future work.

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

Figma MCP hit its account call limit in the previous session. Manual Design
Mode inspection and original asset exports were authorized and successfully
used. Reuse committed references first; a paid Figma plan is not required to
continue. If MCP is available, use the applicable Figma skills. For new screens,
obtain sufficient original reference material rather than guessing from sparse
metadata. Keep original SVGs unchanged and retain attribution.

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
> PROJECT_HANDOFF.md, doc/juxtopposed-redesign.md and the Windows build checklist.
> Inspect the branch, current commit, local changes and available build tools.
> First help me prepare and compile the full Windows x64 Qt-enabled application,
> fix any build/runtime issues, run the six component suites, and verify real
> playback and all three queue tabs. Capture the actual Windows UI for independent
> visual comparison. Preserve Juxtopposed's design/artwork credits and the existing
> VLC behavior. Use separate implementation, visual-review and functional-review
> agents where useful, with clear file ownership. Keep native acceptance distinct
> from component scores, retain the draft PR until its native gates pass, and
> update this handoff with results and the next milestone.
