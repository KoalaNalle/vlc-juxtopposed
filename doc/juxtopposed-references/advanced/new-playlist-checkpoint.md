# New Playlist checkpoint

14 October 2026. Initial evidence recorded at 10:06:18 UTC; updated with the final native suite, Home bootstrap suite and eight observed Windows captures. The confirmed [13:28 New Playlist reference](0808-new-playlist.png) now has a compiled native media-library save path and passing native and component checks. Root observed the modal, title/image/media pickers, ordered repeated save and copied-cover restart journey. Original design: [Juxtopposed](https://www.youtube.com/@juxtopposed). The remaining desktop states and exact design acceptance are still pending.

Evidence is stored in [new-playlist-verification-20261014T100618Z](new-playlist-verification-20261014T100618Z/manifest.json). The original nine-pass evidence remains unchanged; the final ten-pass and separate Home eleven-pass evidence have distinct names. Preserved results and sanitized verification contain no private userdata, settings, media-library database or decoder logs. Component images use supplied controller state and generated example media names; the separately labelled JPEGs are actual Windows captures with generated fixtures.

## Confirmed reference and provisional details

The preserved video frame confirms the New Playlist title and close action, Playlist Details with a title field and square thumbnail, Select Files with a dashed drop area, and Cancel/Save actions. Its SHA-256 is `ed120d4d766376bd4fe907140b4fb18f233c5c33caafb4cba079a91815c3e672`; the original reference video SHA-256 is `70b0ccfa8276acd8fbdf876b96233ebde13aab922acb4f618ef7c31c422aff08`.

The initial compiled 670 × 554 logical dialog remains provisional. A later successful MCP read now confirms original node `6259:14954` is **713 × 589**, with a 112-pixel thumbnail, 186-pixel drop area and 100 × 32 footer actions; the [saved original reference](../figma/20261014/README.md) is distinct from this checkpoint's actual app captures. Aligning the compiled layout remains open. The Remove action, selected-file list with reorder/remove controls, error/partial-save states and smaller-window scrolling are functional extensions whose exact original geometry is unconfirmed. Original example artwork is not copied into the app. No whole-app visual or Figma fidelity score is claimed.

## Implemented scope

The production modal uses `NewPlaylistModel`, owned by DialogsProvider, and native file/image pickers. It accepts ordered absolute local media files, including intentional repetitions and mixed audio/video; users can reorder or remove entries and can save a named empty playlist. A nonempty trimmed title is required. Pointer/drop/keyboard actions, busy protection, recoverable errors, partial-save routing and focus return are present. The main window guards playback shortcuts while the modal is visible, and the Media menu reaches New Playlist. Successful creation navigates to the actual typed native playlist ID.

`new_playlist_data.*` resolves media through the real VLC media library, creates one native playlist, and performs a single ordered append. The path has no queue or primary-player API. Imported media may remain after an earlier resolution/create failure, which is stated in the error. Append or thumbnail-save failure deletes the incomplete playlist; if deletion also fails, the retained native ID is exposed, editing/new-save creation is disabled and View playlist offers that existing playlist. The worker copies its request, and its owner joins submitted work before the media library is destroyed.

The configured native playlist interface exposes an artwork getter without a setter. Optional images are decoded/scaled and saved atomically as an app-owned PNG under `VLC_USERDATA_DIR/juxtopposed-playlist-art/<playlist ID>.png`; the native title, contents and ordering remain native library data. Existing playlist covers and Home playlist cards read that override. DialogsProvider emits a completed-save update after the artwork commit so existing cover models and Home can refresh. A new playlist without artwork removes an obsolete same-ID override left by a database reset.

Decoded Windows UNC/device and relative paths, including authority-free encoded backslash paths and drive-relative `C:filename`, are rejected before filesystem access. Unsafe nonempty thumbnail URLs are also rejected before publishing the preview property: the previous accepted thumbnail and an error remain visible. Empty thumbnail selection is the valid Remove action.

## Native verification

The preserved [initial native QtTest results](new-playlist-verification-20261014T100618Z/native-results.txt) report **9 passed, 0 failed, 0 skipped, 0 blacklisted, 5,250 ms**, including setup and cleanup. [Initial runner verification](new-playlist-verification-20261014T100618Z/native-verification.json) records exit 0, isolated userdata and fixture/test/core hashes.

The separate [final native QtTest results](new-playlist-verification-20261014T100618Z/native-final-results.txt) report **10 passed, 0 failed, 0 skipped, 0 blacklisted, 3,607 ms**. [Final runner verification](new-playlist-verification-20261014T100618Z/native-final-verification.json) records exit 0 and test/core/fixture hashes. This run adds the real unindexed-media and playlist-duration case below; it does not convert the injected failure case into a naturally observed failure.

| Case | Actual evidence |
| --- | --- |
| `validationAndSelectionOrder` | Production model validation, ordered repeated local selection/reorder/remove, unsafe thumbnail publication guard and empty-name/thumbnail rules |
| `actualMixedSaveWithoutPlayback` | Real indexed audio/video, Unicode trimmed title, single native playlist, four ordered entries with a repeated video, counts, copied PNG readable after deleting its original, busy/duplicate-save guards |
| `actualExternalMediaUnknownDurationWithoutDiscovery` | Real external media without folder discovery remains native `UNKNOWN` with duration −1; individual and all-unknown playlist durations format as `--:--`. An empty playlist remains valid zero, a mixed known/unknown playlist retains its genuine known subtotal, signed −650 ms synchronization stays valid, and audio/video/root counts do not change |
| `actualThumbnailWriteFailureRollsBack` | An actual file obstructs the artwork directory; thumbnail writing fails and the newly created native playlist is deleted |
| `actualEmptyPlaylist` | Real named native playlist with zero media |
| `persistenceAfterRestart` | Fresh libVLC instance reads the same native title, counts, repeated item ordering and copied PNG |
| `submittedSaveCompletesBeforeModelDestruction` | Destroying the production model joins its submitted worker, and the native playlist exists afterward |

`rollbackAndPartialFailure` is a **separate injected-backend case**. It verifies resolution/create/append/delete failure decisions and retained partial IDs; it does not claim those failures were naturally observed in the real native library. The actual thumbnail-directory obstruction above is a distinct real write failure. Disk exhaustion during the final atomic PNG commit and a naturally occurring native delete failure are not established.

Both native fixtures check that a fresh primary queue remains empty with current index −1. They do **not** establish preservation of an already playing primary input, its clock, track selection or nonempty queue. The source path avoids those APIs, but the active-playback desktop journey remains a separate gate. Native restart reads the artwork file; the separately observed desktop journey below also renders the copied cover after restart.

## Observed Windows journey

Root observed the actual New Playlist modal, entered a title, selected a real image through the native picker, selected two media files, then selected the video again. The resulting audio/video/video selection was reordered with the audio moved up, saved to a real native playlist, and opened in its native contents route. Quit/warm restart retained the copied cover on Home; activating that Home card returned to the same audio/video/video order. These are actual application actions, separate from supplied-controller component checks.

| Capture | Observed state and build scope |
| --- | --- |
| [01 thumbnail preview](new-playlist-verification-20261014T100618Z/01-thumbnail-preview.jpg) | Named native modal with the selected generated image preview; Qt plugin `b943a87f…97579b` |
| [02 reordered repeated files](new-playlist-verification-20261014T100618Z/02-reordered-repeated-files.jpg) | Selected `alpha-one.flac`, `home-video.mp4`, `home-video.mp4` in that order, with real reorder/remove controls; same initial Qt build |
| [03 saved native order](new-playlist-verification-20261014T100618Z/03-saved-native-order.jpg) | Native saved playlist route retains all three ordered entries; the then-unrepaired rows display −1 ms |
| [04 copied thumbnail](new-playlist-verification-20261014T100618Z/04-native-copied-thumbnail.jpg) | Native playlist card uses the copied image; external unindexed entries remain `3 Other`, and its then-unrepaired all-unknown total displays zero |
| [05 restarted Home cover](new-playlist-verification-20261014T100618Z/05-restarted-home-thumbnail.jpg) | Actual warm Home renders the copied cover under Frequently Visited Playlists |
| [06 restarted Home route](new-playlist-verification-20261014T100618Z/06-restarted-home-route-order.jpg) | Activating the Home card opens the saved alpha/video/video order; this initial Qt build still shows the former −1 ms formatting |
| [07 repaired row durations](new-playlist-verification-20261014T100618Z/07-repaired-unknown-row-durations.jpg) | Actual rebuilt Qt plugin `88452491…ed99c6` shows `--:--` for all three unknown-duration entries |
| [08 repaired playlist total](new-playlist-verification-20261014T100618Z/08-repaired-unknown-playlist-total.jpg) | Same rebuilt Qt plugin shows the all-unknown total as `--:--`, preserving copied cover and native `3 Other` metadata |

Captures 01–06 preserve the actual initial result, including the duration defect; 07–08 provide the later repair evidence. They do not invent audio/video classification or tags for unindexed external entries: native `UNKNOWN`/Other metadata remains honest until genuine discovery supplies more information. These captures establish the listed Windows states, not an overall RHI/fidelity score, light/scaled variants or complete desktop acceptance. The JPEGs were inspected for visible private paths/data; only generated fixtures and app chrome are shown, and their bytes are preserved.

## Separate Home bootstrap helper verification

The [Home native results](new-playlist-verification-20261014T100618Z/home-bootstrap-native-results.txt) report **11 passed, 0 failed, 0 skipped, 0 blacklisted, 4,228 ms**, exit 0. [Sanitized Home runner verification](new-playlist-verification-20261014T100618Z/home-bootstrap-native-verification.json) records matching adjacent/runtime libVLC/core hashes, the original `slice-home-reviewed-package` plugins/preparser, fresh isolated storage, and an owned hidden process completing in 4.328 seconds within its 150-second bound.

This suite uses production visit-store/snapshot/request helpers with real native media-library roots, events, audio-only Unicode/#/% discovery, mixed-folder querying, warm restart, banned/deleted state, reset before indexing events, and actual blocked-store/clear failures. It does **not** exercise the full `MLHomeActivity` QObject/runner wiring, Add Folder picker, or audio-only first-folder Home → play → restart journey. Those app-level gates remain separate from the observed playlist Home cover/route journey.

## Component and build evidence

[New Playlist component verification](new-playlist-verification-20261014T100618Z/component-verification.json) passed against the actual production QML with a **supplied controller** and Qt's offscreen software renderer, with zero QML messages. It covers required name/valid empty playlist, picker requests, ordered repeated mixed rows, drop dispatch/rejection, reorder/remove, thumbnail removal, busy close guard, retained edits/retry, partial-save duplicate guard and route, success/focus, Ctrl+Enter, unavailable library, and short/high-scale focus visibility. A supplied 128-row selection renders 13 virtualized rows. The partial-save and failure displays in this suite are supplied states, not native save failures.

The four tested width/height/scale cases are 1050 × 760 at 1×, 640 × 420 at 1×, 960 × 600 at 1.5×, and 1280 × 720 at 2×. [Selected-state preview](new-playlist-verification-20261014T100618Z/new-playlist-selected-component.png), [empty-state preview](new-playlist-verification-20261014T100618Z/new-playlist-empty-component.png), and [short-window preview](new-playlist-verification-20261014T100618Z/new-playlist-short-component.png) are component-only images. They do not prove native picker, drag/drop, save, restart or RHI rendering.

The root's broader run reports [all 14 Python component/source suites passing](new-playlist-verification-20261014T100618Z/component-suites-verification.json). That includes the New Playlist regression alongside navigation, Home, music, network, queue, library, settings, subtitle presets and existing playback/seek mapping checks. Their exit statuses are distinct from native QtTest or desktop acceptance.

The separate `test_ml_model.exe` target compiled, but its existing Windows stub `initTestCase` fails before the added synthetic duration slots can run. Those slots are not passing checks. The actual ten-case native playlist suite independently verifies the production duration mappings through real media-library queries.

The initial native Qt plugin build passed after the stale `dialogs/qml.deps` dependency file was repaired. `build-windows/staged-new-playlist-libqt_plugin.dll` has SHA-256 `b943a87f8c255661b27d3ff56d6dc3636ffeb9f871fc4a6100a289829c97579b` and identifies desktop captures 01–06. Root subsequently integrated/built the Home bootstrap and unknown-duration repairs; `build-windows/staged-home-bootstrap-libqt_plugin.dll` has SHA-256 `88452491563814608b6d4790752aa4ad9875204780e35e95d5886ab69aed99c6` and identifies captures 07–08. Both hashes were checked from the actual staged files. Neither build accepts the final beta package or release.

## Remaining desktop gates

Root still needs actual desktop drag/drop, empty creation, cancel/Escape/focus, error/busy/partial-save interactions and a save while the primary player is already playing. Full Home bootstrap/visit/reset/opt-out journeys remain pending as described above. Native RHI/fidelity acceptance, scaled/light-window presentation and exact reference geometry remain unaccepted. This checkpoint does not complete the full milestones 0–8 slice or the feature-complete beta.

The manifest records the eight completed desktop observations separately while keeping complete desktop, visual, full-slice and beta acceptance false. Later evidence must retain the exact build and scope rather than extending these passes to untested states.
