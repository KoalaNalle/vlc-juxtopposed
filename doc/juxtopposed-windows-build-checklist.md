# Juxtopposed redesign: Windows build and playback checks

Visual design and original artwork are by **[Juxtopposed](https://www.youtube.com/@juxtopposed)**.
This fork implements their [VLC redesign](https://www.figma.com/community/file/1691465999134370980/vlc-redesign).

## Prepare the home checkout

- [ ] In `C:\modding\vlc-juxtopposed`, check out the PR's `juxtopposed-playback-bar` branch and record `git rev-parse HEAD`.
- [ ] Read the repository's [INSTALL](../INSTALL) and its linked [Windows compilation guide](https://wiki.videolan.org/Win32Compile) for the required toolchain. The script below needs a Unix-compatible build shell and a MinGW-w64 target toolchain; it is not a PowerShell or Visual Studio build command.
- [ ] Close other VLC instances before testing so opening a file does not hand playback to an installed copy.

## Build using the repository's Windows script

The options below come directly from [extras/package/win32/build.sh](../extras/package/win32/build.sh).
Run them from the repository root in the prepared build shell:

```sh
sh extras/package/win32/build.sh -h
sh extras/package/win32/build.sh -a x86_64 -p
```

The second command builds a debug Windows x64 version and tries prebuilt dependencies.
If prebuilt dependencies are unavailable, the script builds them from source.
It builds its tools and dependencies, runs `bootstrap`, configures VLC, and compiles the application.

- [ ] Keep the default Qt GUI enabled. The `-z` option disables it and cannot validate this redesign.
- [ ] Confirm the build finishes successfully and keep its configuration and error output if it fails.
- [ ] Launch the resulting build with its matching plugins and Qt resources, following the configured build/package layout. Confirm the executable belongs to this checkout.
- [ ] If using the Meson route, add `-m`; its build directory is `win64-meson` instead of `win64`. A UCRT build adds `-u` and requires a UCRT toolchain; it uses `win64-ucrt` or `win64-ucrt-meson`.

Installer generation is optional. The script's `-i n`, `-i r`, `-i u`, and `-i m` options add packaging work and prerequisites. A successful component preview or installer build alone does not establish correct playback behavior.

## Optional component check

The [Windows UI workflow](../.github/workflows/qml-ui-checks.yml) uses Python 3.12 and PySide6 Essentials 6.8.3:

```sh
python -m pip install PySide6-Essentials==6.8.3
python modules/gui/qt/tests/test_juxtopposed_playback_bar.py --output-dir ui-previews
python modules/gui/qt/tests/test_native_seek_handle.py
python modules/gui/qt/tests/test_playback_settings_choices.py
python modules/gui/qt/tests/test_playlist_proxy_mapping.py
```

- [ ] Confirm the component checks pass. These check Qt Quick rendering and interactions; they do not compile VLC or exercise real playback.

## Verify the built application with real media

Use a local video, an audio file, a video with multiple audio/subtitle tracks, a chaptered file, and an available network stream. Include a DVD menu if you use discs.

- [ ] Play/pause works with the mouse and keyboard; previous/next follow the queue and are disabled when unavailable.
- [ ] Click and drag the timeline, then seek with left/right keys and the wheel. Test chapter snapping and Shift-drag precise seeking. Confirm playback, the timeline, and displayed times stay synchronized.
- [ ] Move and focus the timeline: the redesigned appearance remains legible and the hover tooltip appears. Keep a drag held while moving outside the bar; the controls should remain usable until release.
- [ ] Change volume by click, drag, wheel, and keyboard; mute/unmute works and updates reflect engine changes.
- [ ] Open audio and subtitle controls, change the selected tracks, dismiss them with Escape, and check focus returns to the initiating button. Test missing audio/subtitle tracks as well.
- [ ] Open playback settings, show/hide the queue, and toggle duration/remaining time. Check streams with unknown duration and media longer than one hour.
- [ ] Change playback speed, aspect ratio, crop and audio modes in the settings menu. Confirm changes reach the engine, hotkey changes refresh the menu, unavailable choices disable, and effects/synchronization open the expected dialogs. Test keyboard scrolling in short windows.
- [ ] Filter a queue containing hidden rows before, between and after the matches. Play by keyboard and touch, remove with Delete and the context menu, reorder rows, stream/save selected items, and jump to the playing item. Confirm each action targets the visible selection. Drop files onto a row and the queue footer; footer/toggle drops should append to the complete queue. Repeat with zero filter matches.
- [ ] Enter and leave fullscreen, using both the visible action and VLC shortcuts. Repeat with pinned controls enabled and disabled, and with the Windows menu/title bar enabled and disabled.
- [ ] Let floating controls hide, then reveal them with the mouse and keyboard. Menus and held controls should stay accessible; after closing/releasing them, automatic hiding should resume.
- [ ] Tab and Shift-Tab through controls; activate buttons with Space/Enter. Test arrow navigation between the playback controls, timeline, header, and open queue.
- [ ] Resize to a narrow window and test Windows display scaling at 100%, 150%, and 200%. Confirm controls do not overlap or disappear unexpectedly and locally packaged icons/fonts load.
- [ ] Confirm audio playback and the library mini player still work. Test DVD navigation if available.
- [ ] Launch with `--no-qt-juxtopposed-controls` and confirm the traditional customizable toolbar still works.

## Record the result

Record the tested commit, Windows version, display scaling, Qt version, build route, and media types. For a failure, include exact reproduction steps, a screenshot if useful, and the application's relevant log output. Native compilation and these playback checks remain required before calling the Windows redesign release-ready.
