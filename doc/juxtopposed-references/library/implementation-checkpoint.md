# Navigation and library backend checkpoint

Recorded 14 October 2026. This advances milestones 2/3; their native and visual acceptance gates remain open. Original visual design and artwork: [Juxtopposed](https://www.youtube.com/@juxtopposed). References and inferred geometry are in [reference notes](reference-notes.md).

This records the `9f754b961c` navigation/backend checkpoint. The subsequent [header/library checkpoint](header-library-checkpoint.md) adds forward history, header, compact video/Songs adapters and Music categories with a newer staged Qt DLL. Its component/build passes remain separate from the pending desktop journey.

## Implemented and built

- A shared navigation component uses the inferred 46px compact rail and 204px expanded sidebar, bundled Inter, active-route feedback, keyboard navigation, preferences, scanning state and unavailable-library feedback. The app adapter routes to Videos, Music/Songs, Network and shared Playlists through VLC's history. It retains the native content and queue models.
- The existing title-bar cone provides an accessible Home action without duplicate Home history entries. The final redesigned title strip/breadcrumb header remains to be ported.
- Home's Windows redesign path exposes Add Folder through a native folder picker and `MediaLib::addFolder`. Registration runs on the media-library worker queue; cancelled selections do nothing, invalid/missing folders report failure, and asynchronous discovery failures reach the UI. Existing file/disc entry points remain available.
- The shared Playlists destination uses `PLAYLIST_TYPE_ALL` and existing playlist list/content adapters, representing both video and audio playlists. Its final referenced presentation/create flow remains open.
- New QML types are registered in Autotools and Meson. Generated native `qmldir` files contain both types. The incremental Qt plugin build exits 0 and is staged in the separate `build-windows/slice-package`. Tested Qt plugin SHA-256: `f29bcafaf4ff61df07bd44d58208c4bbc1dd61c36f0ffa08cff0a80d37d7faa2`.

Navigation currently uses VLC's existing glyph font. Those glyphs differ visibly from the designer's folder icons; original individual exports remain a fidelity gap. Video-based dimensions are provisional. No native whole-app visual score or milestone-completion claim is assigned.

[Expanded navigation component capture](navigation-expanded-component.png) is an isolated Qt render with supplied state, **not a capture of the Windows VLC app**. It establishes the rendered component only.

## Verification and limits

All seven component suites pass locally, including the new navigation suite. It checks route payloads, active-state updates without duplicate actions, mouse/keyboard activation, focus transfer, disabled library routes, unavailable/scanning feedback and compact/expanded layouts at 100/150/200% scale. Existing playback, seek, settings, filtered queue actions, queue geometry and overlay placement checks also pass. The navigation suite is added to the Windows workflow.

[native_library_slice.c](../../../test/libvlc/native_library_slice.c) is an optional diagnostic using the **actual packaged VLC backend**, generated local fixtures and fresh isolated userdata. It builds with the MinGW target and is registered as an explicit optional executable in both build systems. It is outside default tests because it requires fixture arguments.

The diagnostic exits 0 and proves:

1. Adding the generated folder indexes two videos and one audio file.
2. Searching for Waveform yields one audio result and no video result; an absent title yields none; clearing search restores three items.
3. Creating a mixed playlist and appending actual video/audio IDs retains their insertion order.
4. Recreating LibVLC against the isolated database retains two videos, one audio file and both playlist items.

It waits for real library events. Final logs and the exit record are under ignored `build-windows/native-library-final`; generated inputs are under `native-library-checks/fixtures`. The database path was confirmed under the test directory. No user library was modified.

This diagnostic does **not** call the Qt picker slot, operate the library UI, verify QML playlist selection or establish the complete add/open/return/resume journey. Native interaction still awaits the Windows Firewall prompt's user dismissal. The preceding working package remains recoverable. Continue the complete vertical-slice goal; these partial passes do not finish native milestone 2.

## Repeating the backend diagnostic

Generate the fixtures using the preserved recipe, copy only the two videos and WAV into a fixture folder, and explicitly build `test_libvlc_native_library_slice`. On Windows, run its actual `.libs` executable beside the built `libvlc.dll`, `libvlccore.dll` and required runtime libraries. Set `VLC_PLUGIN_PATH` to package plugins, `VLC_LIBEXEC_PATH` to the full `vlc-preparser.exe` path and `VLC_USERDATA_PATH` to fresh isolated userdata. Pass the fixture-folder path as the single argument. Enforce an external timeout; the diagnostic event wait is bounded at 45 seconds.

The test refuses databases with existing media, folders or playlists and retains generated data for inspection. Cold and warm logs use separate filenames. Meson registration has been inspected; configured native build/runtime evidence comes from Autotools.
