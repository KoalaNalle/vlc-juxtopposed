# Security trip — paused checkpoint

Recorded: 2026-10-17T15:57:48+00:00. User date/timezone: 2026-10-17, Europe/Helsinki.

> Another security trip. Log what was the last things you did and recieved. Make a security trip markdown file. We'll continue after that.

Development is paused at the user's request. This is a chronology and resume record, not a security diagnosis. No security detector explanation, alert payload, or automatic approval rejection was visible in the recoverable recent tool results. The cause of the reported trip is unknown.

## Last development actions and results

The following summarizes the recoverable conversation checkpoint and retained local artifacts; it is not a verbatim transcript of every earlier tool call.

1. Continued the Extensions settings page and its native test fixture. Add follows the user's confirmed decision: install the selected available catalog item, otherwise choose a local `.vlp` package; Find more loads the native catalog.
2. Fixed actual QML loading errors: unsupported Home/End handlers and a missing shared toggle help function. Added a real Qt component loading check. The component-loading R4 record reports two passing checks; it does not establish full native or visual acceptance.
3. Investigated the owned local package chooser. Official Qt 6.8.3 source showed that selectFile() leaves the filename field unchanged while that field has focus. Adjusted only the private test driver to clear its owned dialog focus, use a bounded repeating timer, select the Unicode package, and accept asynchronously.
4. Investigated a real VLC shutdown failure. Plain processes exited 0xC000041D; an owned-process fault capture retained the underlying 0xC0000005 access violation. Local symbols resolved the fault to CSDWin32EventHandler::nativeEventFilter, reading a destroyed GUI context during WM_NCHITTEST. The native QWindow could still be alive.
5. Fixed that Windows event-filter lifetime: the GUI handler owns the filter; borrowed GUI objects use guarded pointers and validity checks. Added private checks for live caption hit testing and a late owned-window message after MainCtx destruction.
6. Ran native window acceptance R7. The empty case passed all 16 checks and exited normally. The editing case completed 59 checks, with the final search-clear assertion failing; it also exited normally. Both cases passed three live caption hit tests and recorded the late hit-test probe. Neither required forced cleanup, and both recorded no selected QML diagnostics. The whole R7 workflow remains failed.
7. Before its final failure, the R7 editing case verified actual Unicode VLP installation, chooser cancellation, preference-save rollback/retry, stable package selection, sorted keyboard movement, status feedback, and local filtering. The final failure was traced to the private helper sending an empty key event after selecting text, instead of deleting it.
8. Changed the private helper to send Backspace for an empty replacement and added an explicit Escape-to-clear check. These R8 changes have been built, but have NOT been exercised in a new native UI run.
9. Prepared verify-ordinary-page-r1.py and syntax-compiled it without executing its test cases. Recorded its hash and executionPending=true in ordinary-page-runner-input.json.
10. Polled private build session 5358: terminal exit code 0. The build reported that the ordinary page and private fixture frozen inputs matched. The ordinary R5 build exit record is also 0.

## Last received context

- The preceding conversation checkpoint records a date/timezone update for 2026-10-17, Europe/Helsinki.
- It records a user-side Page context message: `<external_codex_apps_open_page>{"page_id":null}</external_codex_apps_open_page>`. This identifies no selected Page. No Page content was fetched and no Page was edited. Its timing does not establish a connection to the reported trip.
- The next substantive user instruction was the security-trip request quoted above.
- After that request, the assistant acknowledged the pause and promised this file, with the prepared tests left unstarted.
- A read-only status call immediately before conversation compaction returned output truncated by the available model context. Its contents were not treated as proof. The short checks below were repeated after compaction.

## Checks performed only to write this pause record

- Confirmed repository HEAD: `32cd62fc4d71b0b59d6a2e79c7ea72f3a6234112`; the frontend work remains uncommitted.
- Confirmed ui-build-r8-exit.txt = 0 and page-build-r5-exit.txt = 0.
- Confirmed the prepared ordinary runner exists. Neither window-acceptance-r8 nor ordinary-page-acceptance-r1 exists: both next runs are unstarted.
- Queried the matching project VLC, debugger, Python, and WSL process inventory; it returned no matching processes. No new build, UI run, media test, or debugger run was started after the pause request.
- Read the existing handoff, runtime identity record, R7 verification, prepared runner metadata, and working-tree status.
- The goal-status tool returned goal=null. There was no registered active goal to change; the file handoff is explicitly marked paused.
- Created this Markdown record and updated only the migration handoff metadata for the pause. No development source or binary was changed by this incident-recording step.

## Current source and runtime

- Authorized working tree: `E:\Modding\vlc-juxtopposed`.
- The chat environment still advertises an F: checkout. All commands for this checkpoint explicitly used the E: working tree and the E: migration environment. F: was not inspected.
- Ordinary candidate runtime: `E:\Modding\vlc-juxtopposed\build-windows\beta-extensions-20261017\page-runtime-r4`.
- Ordinary build: page-native-r5; frozen input record page-inputs-r5.json contains 1,910 entries. This is an input inventory, not a claim that all entries are tracked source files.
- Private build: page-native-ui-r8; ui-inputs-r8.json contains five frozen inputs. Its test hooks are private, not part of the ordinary GUI binary.
- Ordinary GUI remained unchanged by the R7 private fixture. No complete acceptance claim is made for the current Extensions page or the beta.

Ordinary candidate SHA-256 identities:

| Binary | SHA-256 |
| --- | --- |
| libvlccore.dll | `bf29403169b936e2156bcfcd0f03a38fbbaad0d9b54fd0e7d9e843e4bdb5bc33` |
| plugins/gui/libqt_plugin.dll | `17d870ec6dbb978ce3aa1a11f1dd9974425a21b996018cec15e67b3837c81eb1` |
| plugins/misc/libaddonsfsstorage_plugin.dll | `1f72cd18278294baccf78d9b26f6e83c7e37544f1d4df46020460d5eef982ef0` |
| plugins/misc/libaddonsvorepository_plugin.dll | `e480f590309f612320abfd614c0cc3af32604f6c122c4af70a675b97eff5d18c` |

The main uncommitted work covers the Extensions model/controller/page, its original SVG assets and build registration, the startup metadata consumer, stable selection/status behavior, a small native add-on metadata publication correction, and the Windows caption event-filter lifetime fix. Private test sources are also uncommitted. Existing unrelated untracked media/audio reference files were preserved.

## Retained evidence

All relative entries in this section are under `E:\Modding\vlc-juxtopposed\build-windows\beta-extensions-20261017`.

- `window-acceptance-r7/verification.json`: latest completed native workflow; overall passed=false.
- `window-acceptance-r7/empty/late-window-event.json` and `window-acceptance-r7/edit/late-window-event.json`: owned late-message records.
- `window-fault-capture-r6/edit/native-fault.dmp` and `native-fault-stack.txt`: earlier original fault evidence; retained, not overwritten.
- `shutdown-baseline.json`: shutdown defect diagnosis and candidate fix context.
- `ui-build-r8.log`, `ui-build-r8-exit.txt`, `ui-inputs-r8.json`: latest completed private build.
- `page-build-r5.log`, `page-build-r5-exit.txt`, `page-inputs-r5.json`: ordinary build and frozen inventory.
- `page-runtime-r4.json`: current ordinary candidate identities.
- `component-loading-r4/verification.json`: Qt component loading results.
- `ordinary-page-runner-input.json`: prepared runner identity; execution pending.

Earlier failed runs and debugger evidence remain retained. A debugger process exiting successfully is not proof that its VLC child shut down normally. The old forced-cleanup runs remain rejected evidence, not accepted tests.

## Recent outside reads and actions

- Read official Qt 6.8.3 QFileDialog source as text: https://raw.githubusercontent.com/qt/qtbase/v6.8.3/src/widgets/dialogs/qfiledialog.cpp . An earlier attempt to open the corresponding code.qt.io page returned an access/internal error. No downloaded web code was executed.
- Used local Windows debugger tools and MinGW symbols on the owned VLC fault dump; symbol cache was directed to E:\Apps\Cache\symbols. The private fixture used the installed Windows dbghelp library for its owned-process dump capture.
- Used cached design references. No new Figma write or Page operation was performed in this recent sequence.
- No Git push, merge, publication, external message, BIOS/partition change, or software installation was performed in the recent sequence summarized here.
- The diagnosed VLC shutdown bug and the earlier host GPU/WSL crash are distinct observations. Neither is established as the cause of this user-reported security trip.

## Resume only after the user continues

1. Read this file and E:\Codex\migration-20261016\ACTIVE-BETA-WORK.json. Use the E: checkout and dot-source E:\Codex\migration-20261016\scripts\Common.ps1 for shell work. Recheck source/frozen-input identities and that no job is already running.
2. Run the prepared native Extensions journey with ordinary package page-runtime-r4, private GUI page-native-ui-r8/.libs/libqt_plugin.dll, and a fresh output directory window-acceptance-r8. It has not run yet. Require the full five-case workflow and normal shutdown, not just the partial R7 passes.
3. If that passes, run the prepared ordinary five-case smoke runner verify-ordinary-page-r1.py into its fresh ordinary-page-acceptance-r1 directory. Keep Downloads filenames, raw media logs, and frames private.
4. Recheck the changed native add-on core with the maintained native regression suite, then finish actual catalog behavior and the remaining beta validation. Do not treat successful builds as feature acceptance.

Continue with one build or native/media test at a time and low build concurrency. Keep project data, downloads, caches, Linux disks, and large tools on E:. The user allowed only small necessary host software on C:; no large project data belongs there. F: remains outside the authorized work path.

Beta remains limited to bringing Jux's confirmed redesign to life with low bug rates. Vanilla parity belongs to V1.0; proposed additional extensions belong to later planning.

## Working-tree snapshot at pause

This was captured before adding this incident file; it is retained to distinguish prior work from the pause record.

```text
 M doc/juxtopposed-beta-plan.md
 M modules/gui/qt/Makefile.am
 M modules/gui/qt/assets.qrc
 M modules/gui/qt/dialogs/dialogs/qml/JuxtopposedDialogsHost.qml
 M modules/gui/qt/dialogs/dialogs/qml/JuxtopposedGlobalSettings.qml
 M modules/gui/qt/dialogs/plugins/plugins.cpp
 M modules/gui/qt/dialogs/plugins/plugins.hpp
 M modules/gui/qt/maininterface/mainctx.cpp
 M modules/gui/qt/maininterface/mainctx.hpp
 M modules/gui/qt/maininterface/mainctx_win32.cpp
 M modules/gui/qt/meson.build
 M modules/gui/qt/network/addonsmodel.cpp
 M modules/gui/qt/network/addonsmodel.hpp
 M modules/gui/qt/tests/native_ui/README.md
 M src/misc/addons.c
?? doc/juxtopposed-references/native/guest-extensions-page-checkpoint-20261017/
?? modules/gui/qt/dialogs/dialogs/qml/JuxtopposedExtensionsSettings.qml
?? modules/gui/qt/maininterface/extensions_preferences_model.cpp
?? modules/gui/qt/maininterface/extensions_preferences_model.hpp
?? modules/gui/qt/pixmaps/juxtopposed/extensions-settings-08f95.svg
?? modules/gui/qt/pixmaps/juxtopposed/extensions-settings-1b0f7.svg
?? modules/gui/qt/pixmaps/juxtopposed/extensions-settings-1bf8a.svg
?? modules/gui/qt/pixmaps/juxtopposed/extensions-settings-36bfa.svg
?? modules/gui/qt/pixmaps/juxtopposed/extensions-settings-3ffd5.svg
?? modules/gui/qt/pixmaps/juxtopposed/extensions-settings-432a4.svg
?? modules/gui/qt/pixmaps/juxtopposed/extensions-settings-66cd9.svg
?? modules/gui/qt/pixmaps/juxtopposed/extensions-settings-6db55.svg
?? modules/gui/qt/pixmaps/juxtopposed/extensions-settings-7d179.svg
?? modules/gui/qt/pixmaps/juxtopposed/extensions-settings-80d5a.svg
?? modules/gui/qt/pixmaps/juxtopposed/extensions-settings-97863.svg
?? modules/gui/qt/pixmaps/juxtopposed/extensions-settings-9c2da.svg
?? modules/gui/qt/pixmaps/juxtopposed/extensions-settings-ad68b.svg
?? modules/gui/qt/pixmaps/juxtopposed/extensions-settings-cf6e1.svg
?? modules/gui/qt/pixmaps/juxtopposed/extensions-settings-ec3f6.svg
?? modules/gui/qt/pixmaps/juxtopposed/extensions-settings-f1a4a.svg
?? modules/gui/qt/pixmaps/juxtopposed/extensions-settings-f7e21.svg
?? modules/gui/qt/pixmaps/placeholder/noart_albumCover_dark.svg
?? modules/gui/qt/tests/native_ui/extensions_journey.cpp
?? modules/gui/qt/tests/native_ui/extensions_journey.mk
?? modules/gui/qt/tests/native_ui/run_extensions_journey.py
?? modules/gui/qt/tests/test_juxtopposed_extensions_settings.py
?? test/libvlc/native_soxr_reference.py
?? test/modules/audio_filter/soxr_lifecycle_trace.c
?? test/modules/audio_filter/soxr_native_reference.c
?? test/modules/audio_filter/soxr_pcm_trace.c
```

## Resumed after explicit user instruction

2026-10-17T16:15:47+00:00: The user requested “Continue from the security trip checkpoint. Keep everything on E:”. Frozen inputs and ordinary binary hashes matched before resuming. R8 exposed a private fixture window-type assumption; R9 retained its compile failure; corrected R10 passes all five native journeys (190 checks). The metadata-only production catalog run passes 17 checks; native add-ons49/948 and ordinary playback5 pass. All work and outputs stayed in the authorized E: project/tool environment. See [the current Extensions checkpoint](doc/juxtopposed-references/native/guest-extensions-page-20261017/README.md) for scope and remaining gates. This continuation does not establish the cause of the reported security trip.
