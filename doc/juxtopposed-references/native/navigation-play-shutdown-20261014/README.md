# Playlist Play, restored navigation and Windows quit — 14 October 2026

This incremental checkpoint repairs duplicate playlist Play requests, empty-card
player transitions, Music/Network history property forwarding, and selection
loss when replacing grid/list views. It also fixes a reproduced MinGW Qt module
shutdown deadlock. The ordinary registered-source Windows build now finishes
actual playback and automatic quit, and fully terminates after normal Ctrl+Q
following native Network folder browsing. Full beta acceptance remains open.

| Verification | Recorded result and scope |
| --- | --- |
| Music/Network paired component gates | Original 27 fail; repaired 27 pass. Genuine route properties, exact-route writers, model readiness/identity and one-time row restoration are checked with supplied offscreen Qt adapters. |
| Playlist paired component gates | Same 13-case oracle: original 7 fail/6 pass; repaired 13 pass. Known empty Play preserves the library; filled/unknown collections issue one request. Never-cleared diagnostics cover construction through explicit teardown. This is a windowless adapted QML test. |
| Stricter loader cursor test | Removing the fixture's automatic cursor restoration exposes the original production failure at row 9. The repaired loader passes all 11 cases with the same helper. Deliberate unknown-property diagnostics remain identified as expected negatives. |
| Windows component/source workflow | All 19 suites pass locally with their exact script and workflow hashes. C++ DLL/actual application checks below are separate. |
| Earlier Qt056 native playlist/music journey | Empty Play retains the paused primary. One filled-card Play adds three ordered entries once to the existing one-item queue. Actual second album and artist identities return after Home/Back. Those generated-fixture captures are retained unchanged; broader focus/reactivation and populated Genres are unaccepted. |
| Static Windows DLL regression | Actual winpthreads keys pass normal create/set/get/delete/error and explicit FreeLibrary checks. Original process-exit destruction with a worker holding the TLS lock times out despite exit status 0; the prototype fix passes all three modes. The exact registered wrapper separately passes all three modes. |
| Actual automatic playback and quit | Unwrapped Qtca793 times out after 30 seconds despite exit status 0. The prototype Qt77c75 terminates in 6.907 seconds; ordinary registered Qt7c7f99 terminates in 10.828 seconds, both with exit 0. Timing includes launch and playback of the same generated video. |
| Ordinary-build native folder journey | Pointer input opens the authorized subtree. Its fourth selected child survives grid-to-list and Home/Back. Normal Ctrl+Q signals the owned Windows process handle with exit 0; later inventory finds no PID and the exact Qt DLL opens without sharing, confirming its file lock is released. |

## Native runtime distinctions

The original Qt056 run is useful partial acceptance, with two real Network
failures: a later child selection returns to row zero, and pointer input logs
`BrowseDeviceView.qml:189: TypeError: Cannot call method 'forceActiveFocus' of undefined`.
Its quit wait times out. The [published interpretation](desktop/qt056/observations-corrected.json)
corrects two unsupported early booleans in the private original observations;
the original hash and failing wait are preserved. A closed window, finished
QApp loop, flushed logger and reported exit status 0 do **not** establish process
termination. Earlier frozen proof bundles retain their original narrower
exit-status evidence and are not rewritten as bounded-termination passes.

Qtca793 contains the production cursor/focus repairs. Its first Home/Back
returns to the parent drive; a later settled fourth-child history journey
passes. Invalid native tree frames could explain that first result, but this
remains an inference. Rapid refresh, same-route generations and tree lifetime
remain open gates. Both its Network quit and its separate automatic playback
quit leave an unsignaled process handle. No forced cleanup is counted as a pass.

Read-only, noninvasive inspection of the latter process maps the remaining
thread through a winpthreads condition wait, rwlock writer and
`pthread_key_delete`, reached from the C++ static thread-exit key destructor.
The supplied native DLL fixture reproduces the failure with a real worker
inside its TLS destructor. Windows terminates other threads before DLL process
detach, which can strand a lock held by one of those threads. See Microsoft's
[ExitProcess documentation](https://learn.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-exitprocess).

The MinGW-only module wrapper bypasses TLS-key deletion only while
`RtlDllShutdownInProgress()` reports final process shutdown. Ordinary deletion,
invalid-key handling and explicit module unloading still use the real
winpthreads implementation. Autotools and Meson register the source and module
link flag; only the ordinary Autotools build is exercised here. Meson remains
source-reviewed. The original failing and fixed probe sources, compiler
recipes and exact results are under `native/`. Binary artifacts are not committed.

The actual registered Qt DLL is
`7c7f99054ee0017faf5cd2dd538f7775f3b45c4cf0505a173a54f733e13fddaf`.
Its [post-generation manifest](runtime/registered-exit-repair-source-freeze-final-20261014.json)
freezes 1,073 Qt-directory files, including tests/unused assets and ignored
generated Makefile.in. Earlier setup failures are retained. Relative to the v2
manifest, Makefile.am was corrected before the successful ordinary build and
Makefile.in was regenerated. No exact final pre-build manifest or clean final
beta build is claimed. The actual compilation/resource logs, staging recipe,
owned wait and source/runtime hashes remain distinct from the temporary
prototype's extra object/link overlay.

## Captures, privacy and remaining gates

The [fresh Home capture](captures/registered/01-generated-home-monitor2.jpg)
uses the registered runtime. Earlier playlist/music captures are explicitly
under `captures/qt056/`. All are unchanged generated-fixture captures. The
actual Network screenshots, personal paths, full native logs, preferences,
library databases and media remain private; only their generic observations
and relevant original hashes are published. Logs included here are build and
isolated component/probe outputs. Text/JSON use generic path labels;
[evidence-index.json](evidence-index.json) records 132 original/public pairs.

The fresh desktop initially restores a 306×353 window on monitor 2. A refreshed
observed title-bar action maximizes it to 1920×1032 on that same monitor before
the native journey. That small restored geometry remains an unresolved window
behavior; this is not narrow/DPI acceptance or a canonical Figma comparison.

Generated-profile checks do not establish first-run/clean-profile behavior.
Keyboard focus/reactivation, populated Genres, all Music/Network routes,
rapid tree refresh, restart, theme/scaling, embedded subtitle cues, complete
preferences, Figma fidelity and the final relocatable licensed beta package
remain required by the [beta plan](../../../juxtopposed-beta-plan.md).
