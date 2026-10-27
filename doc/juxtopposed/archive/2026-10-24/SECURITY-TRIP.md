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

## Subsequent settings quality checkpoint

2026-10-17T18:31:44.964619+00:00: Continued under the user's E:-only instruction, one heavy task at a time. Fixed observed Light placeholder contrast and unnamed native settings buttons. Playing/paused/restart1652/1653/14, Extensions5/190 and ordinary playback5 pass normally; scoped evidence is in [settings quality](doc/juxtopposed-references/native/guest-beta-settings-quality-20261017/README.md). Earlier native18/42/9/37 and component results are separately scoped there. Failed private assumptions/typo and genuine UI failures remain rejected evidence. No security-trip cause is established and the full beta remains incomplete.

## Subsequent Home content/resume checkpoint

18 October 2026: Continued under the user's E:-only instruction, one heavy task
at a time. Genuine all-five-section Home journeys reproduced a lost video resume
point on two plain old-core runs. A current-input timer snapshot before detach
repairs it; two fresh288/137 journeys, prior audio/privacy234, Library69,
Privacy17/database18, queued history40/52, decoded history6, five upstream player
programs, subtitles38 and ordinary playback5 pass normally. E:-full during runtime
copy and missing private Windows test modules were resolved as setup failures;
rejected reports and original incident evidence remain retained. See the
[scoped proof](doc/juxtopposed-references/native/guest-home-content-resume-20261018/README.md).
No security-trip cause or full-beta acceptance is established.

## Subsequent default-resize Home checkpoint

18 October 2026: Continued on E:, one heavy task at a time. An explicit native
negative reproduced a 16-pixel off-screen floating preview at 320×360. The width
clamp passes native288/137/112 and ordinary playback 5 with normal exits and no
selected diagnostics or captured fault. Generated-file restoration and eight
rapid asynchronous native-ID requests pass; native rescanning reindexes the
restored video, so old identity/history preservation is not accepted. Original
binaries and rejected fixture assumptions are retained. See the
[scoped proof](doc/juxtopposed-references/native/guest-home-default-resize-20261018/README.md).
No security-trip cause or full-beta acceptance is established.

## Subsequent ordered-pin checkpoint

18 October 2026: Continued on E:, one heavy task at a time. Native ordered pins pass
203/70/68/72, cold storage22, component 19 and strict ordinary playback5 on the recorded
Core0cdebe/GUIa7af1c/MLe3ec5e tuple. The original unnamed-signal warning was missed by
an earlier narrow log filter; the actual signal fix and stricter replay are accepted.
Earlier partial/failed runs remain rejected. All owned jobs ended normally in the
accepted run; generated files and ordinary binaries were preserved. The original
security-trip cause remains unknown; no new detector payload or automatic approval
rejection was received. See the [scoped proof](doc/juxtopposed-references/native/guest-sidebar-pins-20261018/README.md).
Full beta/fidelity/package gates and source-browser empty SVG warning triage remain open.

## Subsequent network artwork checkpoint

18 October 2026: Continued on E:, one heavy task at a time. Last substantive work:
read the saved pin checkpoint; traced empty artwork calls with a private-only
colorizer diagnostic; received six marked QML errors at NetworkGridItem.qml:58;
then reproduced four original warnings with the unchanged GUI and stricter runner.
The diagnostic's first link failed due to a missing private make prerequisite;
its corrected R2 link succeeded. This was a test-harness failure, not a detector response.
Guarded the real artwork binding and rebuilt the ordinary/private modules sequentially.
Received normal native203/70/68/72 and ordinary5 results with zero Qt warning lines;
input inventories, generated media and prior runtime preservation all pass.
No new security detector payload, automatic approval rejection or crash was received.
The original security-trip cause remains unknown. The temporary diagnostic is absent
from both accepted modules; the test journey is absent from the ordinary module.
All owned jobs are terminal. See the [scoped proof](doc/juxtopposed-references/native/guest-network-artwork-20261018/README.md).
Full beta/fidelity/package gates and original sidebar geometry/Home/Recent remain open.

## Subsequent sidebar shell/Home/Recent checkpoint

18 October 2026: Continued entirely on E:, one heavy task at a time. Last substantive
work: used the final budgeted Figma correction at 6261:15094, retained its screenshot/
code and downloaded tool-provided originals. The export incorrectly maps Videos/Music
to a gear. Existing cached Player data supplied the correct inactive Home outline;
no extra MCP calls were made. A user question for two extra focused icon reads remains
pending. Implemented217/50 geometry, Home/Recent routes, native paged media/stream
history, recent playlist visits, native history leaves and password-free locations.

Received native R1 normal exit/204 checks with a failed private QML-id evaluation;
R2 corrected that driver, passed 282 fresh-profile checks including decoded HTTP reopen,
then rejected its cold 96-check expectation after the preceding explicit history clear.
R3 checks the real empty-to-normal-stop-to-populated transition and fixes screenshot-
observed footer/tab styling. R4 adds actual app expand/collapse and explicitly records
Library-OFF Recent as unaccepted. Final native 298/178/101/106 (683), component pin21/
navigation and ordinary5 pass with normal exits and all Qt warnings zero. Frozen
inputs, generated media and prior runtime preservation pass. The ordinary GUI contains
no private journey/HTTP-test/temporary diagnostic marker. Rejected artifacts remain local.

No new security detector payload, automatic approval rejection or crash was received;
the original security-trip cause remains unknown. All owned jobs are terminal. See
[scoped proof](doc/juxtopposed-references/native/guest-sidebar-shell-20261018/README.md).
Full beta is incomplete. Next: expose existing history-only data to Recent with Library
OFF, resolve two original icons within the user's reference budget decision, and
continue the full redesign/fidelity/platform/clean-package gates.

## Subsequent Recent with Library OFF checkpoint

18 October 2026: Continued on E:, one heavy task at a time. Last work connected
Recent to the existing lazy history-only backend, registered minimal reader types
without a browsing frontend, guarded the model's initial source and drained reads
on shutdown. The first thread runner omitted new discovery metadata (32); explicit
regeneration passed 34. Initial launcher errors started no playback. Private R1 was
interrupted 130 after static review; R2's actual Recent click asserted after 14 valid
startup/no-database checks. R3 passed six processes but exposed deferred database
initialization being mistaken for empty history under a real owned file lock.
R4 checks the status-returning read before publishing its frontend and passes the
actual failure/Retry path. A stale ordinary verifier path was corrected before its
first playback case; rejected outputs remain private.

Received final native 24/10/298/178/102/106/82 (800), thread 34 and ordinary 5 results:
all normal exits, no forced cleanup, all Qt warnings zero. Exact source inputs,
retained 458 Core/eight store, generated media, previous runtime and the five original
untracked files are preserved. No new Figma call, detector payload or automatic
approval rejection was received; original security-trip cause remains unknown.
All owned jobs are terminal. Full beta/fidelity/platform/clean-package gates remain
open. See [scoped proof](doc/juxtopposed-references/native/guest-recent-history-only-20261018/README.md).

## Subsequent shared Home/player strip checkpoint

18 October 2026: Continued on E:, one heavy task at a time. Last substantive work:
reused cached full Home/Player design contexts, downloaded their tool-provided
original SVG exports to E: and compared nine runtime slots/six common Home assets.
No new Figma MCP read was made; the two-icon budget question is still pending.
Integrated the shared title strip/native menu/window controls and existing typed
navigation. Private R1 rejected duplicate QML visibility before any native run;
R2 exposed raw title versus native fallback semantics; R3's traditional readiness
helper incorrectly required redesigned objects. R4 reproduced a retained stale
name after actual decoded source switching. R5 stores PlayerController's name
before notifying QML and verifies actual paused Home/source returns.

Received final native13 processes/2,012 checks, component12 and ordinary5: all
native/ordinary exits normal, no forced cleanup and all Qt warnings zero. Native
resource-byte/geometry/keyboard/window actions and live title changes pass.
Earlier component harness failures included an access violation in a dynamic
PySide attached metaobject, not a machine crash; an inherited 2px button-padding
error was corrected. The proof writer initially assumed the wrong cached Home
asset filename; the exact existing names were verified before it succeeded.
Production1,935/private6/component13 and retained Core458/store8/thread4/original5
inputs match; prior thread34 was not rerun. The ordinary GUI is63aa786f2b6d;
Core0cdebe/MLe3ec5e remain unchanged. Previous runtime/media are preserved; private
QA/HTTP-test/title-asset and diagnostic markers are absent from ordinary output.

No new security detector payload, automatic approval rejection or Windows crash
was received. The original security-trip cause remains unknown. All owned jobs
are terminal. Full beta, native drag/double-click/OS-DPI/Windows11, broader themes/
accessibility/fonts, short-height drawer, full screen fidelity and clean licensed
packaging remain open. See [scoped proof](doc/juxtopposed-references/native/guest-shared-strip-20261018/README.md).
No push, publication or designer outreach occurred.

## In-progress window/navigation follow-up — not accepted

18 October 2026: Work remains on E:, one heavy workload at a time. Reproduced
keyboard focus clipped below a short player drawer (Playlists184..216 in186px).
A five-line viewport adjustment passes native16 processes/3,150 checks, including
actual system drag/double-click, decoded audio/video volume/mute invariants and
prior sidebar/history regression. Independent mouse-release checks pass.

The ordinary candidate GUIb3233ec9df0b/Core0cdebe/MLe3ec5e is NOT accepted: three
file cases pass, then local HLS exits0xc0000005 in AUDIOSES.DLL10.0.19041.5794+1358f
at07:53:57 local, WER21009420-6c1f-445e-bad5-94b70cabcac5. This is an application
crash, distinct from the earlier Windows blue screen. Its original temporary
Windows dump is gone; event evidence is retained on E:. Debugger and normal
capture-enabled replays exit normally; the audio fault remains unexplained.

A private diagnostic interface/helper exists only under the owned E: test tree.
It reads only its launching VLC process and writes dumps to E:, without changing
Windows settings. Initial static-export SDK mode failed to load; dynamic mode
was corrected and installation/removal verified. No AV was reproduced so far.
Forced stderr logging exposes a late QObject timer-thread warning after the GUI
thread exits. Fatal-warning diagnostics are deliberately separate from acceptance:
R1 missed statusC0000602 and timed out90s; the owned target/debugger were killed.
R2 captures the stack but its quoted dump path was malformed. Symbol mapping
locates QObjectPrivate/ QObject destruction during DLL process detach. R3 corrects
the dump path for object-lifetime analysis. Earlier log-only zero-warning results
do not establish absence of these late stderr/debug-output warnings.

All processes before R3 are terminal. No new security-detector payload or
automatic approval rejection was received. Original security-trip cause remains
unknown. The current audio crash and timer warning must be resolved or explicitly
qualified before final candidate acceptance. No publication or push occurred.
See build-windows/beta-window-navigation-20261018/WORK-CHECKPOINT.md for live state.

## Departure checkpoint — 18 October 2026

The user requested ramp-down and a return-home report, then explicitly authorized
committing and pushing source. No new heavy build was started after that request.
WSL shut down normally; the current disk was verified closed for manual copying.
The Qt cache SDK one-object rebuild succeeded (204 other archive members unchanged).
Two extra Figma reads were explicitly approved, used for71:857/71:851 and cached
on E:. Exact original SVGs are bound but unverified in a rebuilt native GUI.
New cache tests/strong late-warning capture are saved but uncompiled/unreplayed.
The small departure component check passes21. Original AUDIOSES AV remains open;
the controlled fatal Qt warning dump does not establish its cause. See RETURN-HOME.md.

### Departure push automatic-review rejection

The combined Git add/commit/push command was rejected before execution. The
reviewer stated: "The action would push a commit containing private handoff,
migration, crash-investigation, and design-reference material to an unverified
external GitHub destination; the broad request to push changes does not
specifically authorize exposing this sensitive payload there." No outgoing push
or local commit ran in that rejected command. Continue the local commit and
private E: SSD handoff as unaffected work. Do not retry the outgoing push until
the user specifically approves that material for origin/juxtopposed-playback-bar.
