# Shared native preferences — 21 October 2026

**Core, backend models and selected matching GUI regressions pass.** The original
ordinary two-window lost-update reproduction now passes in both closing orders.
This qualifies shared native defaults in this scope; the complete multi-instance
scenario and beta remain open.

Core records intentional pending edits and merges them into the latest saved
file under a persistent interprocess lock. Successful saves acknowledge edits,
so an older instance cannot replay them on exit. The typed checked transaction
owns validation, mutation, replacement and rollback; Qt no longer reinterprets
another process's later commit through an unlocked file read. Live values are
not refreshed from another process's defaults. Same-key last successful commit
wins. Startup and explicit reset, command-line overrides and normal default
commenting are preserved. The loader also retains a final unterminated option.

R8's real-Core console suite passes38 contract checks across27 normal processes:
600 overlapping commits,1,200 actual lock events with one file identity/no
critical-section overlaps, either close order, no-op exits, prior pending edits,
mixed types, strict invalid data, BOM/CRLF/unknown/long/Unicode content and genuine
read/replacement/read-only failure/rollback/retry. Every unexpected native error
is rejected; six intentional errors remain bound to their exact cases. The
previous pending-string value is checked during rollback; independent string
pending-exit acceptance is not claimed. No GUI or physical-audio proof is implied.

The Qt checked-float9 and Interface15 backend/model cases pass against exact R8
Core with normal exits. Matching native GUI replay passes popup7/313 and
geometry4/194, including editor save/error/retry, cold reopen, Off policy and
playing-state preservation. Both deliberate read-denial controls have exactly
one expected error; no unexpected native/Qt errors or forced cleanup occur.
Ordinary GUI is `371bc858e3bb…`; private GUI and all frozen inputs are bound in
`compilation-gui-r1.json`. Original runtime Core and other modules are preserved.

Two ordinary VLC windows use the same owned native configuration, separate Qt
stores and different generated silent WAVs. A is fullscreen on the primary
monitor; B stays in a normal window on the secondary monitor. Actual edits and
normal exits use Computer Use. A saves duration5000 and B saves geometryOff:
both values survive B→A and A→B closing orders. The surviving window retains
its own paused media. All four processes exit normally with unchanged source/
media, no native/Qt errors and no private hooks. `ordinary-ba-r1` and
`ordinary-ab-r1` bind each intermediate real-file snapshot. Raw Computer Use
screenshots were not saved. This does not prove simultaneous playing, two
physical audio outputs, or a common Qt profile/geometry store.

Additional matching backend regression passes General/setup11, Audio32,
Network17 and profile-defaults13:73 more cases,97 with the original24. Audio
includes owned PCM/real equalizer and visualization callback consumers; it does
not establish physical audibility. General/Audio passed in the initial attempt;
Network initially received the65KB libtool launcher instead of the13MB `.libs`
executable and produced no test XML. That attempt and missing return-code
recording are preserved in its classification. Restaging the actual Network/
profile binaries passes both suites; no production fix or relaxed test was used.
The sole console runner warning is QtTest's xunitxml deprecation notice.

The later ordinary simultaneous-playing run is **rejected for clean playback
acceptance**. UI observations establish separate clocks, B pause/seek298s,
volume40% versus A100%, B queue2/second media versus A queue1/original media,
fullscreen return and survivor playback after A exits. Both processes exit0,
but native logs contain late PCR768ms on A and1205/900ms on B after Qt interface
removal starts. B also logs two clipboard retries during Computer Use file-path
entry. Preserve those errors; do not describe this as a clean active playback
pass. Only one explicit MMDevice endpoint plus Default is enumerated; these are
not evidence of two separate physical outputs.

A targeted no-GUI comparison passes two real LibVLC/MMDevice/WASAPI processes
playing owned silence for90s each, then explicitly stopping before release.
Clocks advance, decoded/played counters increase, sampled lost buffers stay0,
and there are no native errors/clock warnings. `steady-audio-r1.json` corrects
the original raw clock field's label to microseconds, as specified by the native
API; values and comparisons are unchanged. This supports a GUI shutdown-order
investigation, not a completed fix. Ordinary Stop-before-Quit comparison is next.

Earlier rejected candidates remain recorded. R1 incorrectly treated Windows'
nullable locale compatibility stub as allocation failure. R2 passed26 checks but
failed first-file creation because generic Windows open did not reliably map
absence into errno. The reader now distinguishes actual absence from denial;
this also avoids the misleading first-run missing-config error. R4/R6/R7 exposed
an intermittent Windows replacement error5. One lock domain and nonoverlapping
critical sections were verified. A separate Win32-only executable, with no VLC
or Core linked, reproduced two such failures in240 replacements. Immediate
DELETE-access opens then succeeded; the responsible external cause is unproven.

R8 retains the same lock/temp and retries only Windows errors5/32 at most10 extra
times with10ms sleeps (100ms requested sleep, not a wall-clock guarantee).
Persistent denial still fails atomically and restores values/pending state.
Six naturally occurring overlap denials recovered after one retry apiece; an
owned35ms sharing denial recovered after two. Permanent sharing denial remains
a failing negative control with exactly one error. No security settings changed.

`verification.json` binds the receipts and source manifests. Core R8 is
`3a7b0a360e3c…`, frozen under
`build-windows/beta-native-settings-concurrency-20261021/source-r8`.
Windows10, active audio routing, full visual/accessibility/DPI and final package
acceptance remain open.

The [ordinary Stop-before-Quit comparison](ordinary-stop-before-quit-r2.json)
records one undisturbed B process after over four minutes playing, native
STOPPED before Qt teardown, normal exit and no native/Qt errors. A also exited
normally but is excluded from timing acceptance: an interrupted noninvasive
debugger left its threads suspended, then a separate debugger resumed and
detached successfully. Buffered file logs were not used as live stop timing.
This comparison motivates a pending application-Quit lifecycle candidate; it
does not qualify direct Quit while playing.

The earlier90s no-GUI comparison inherited a duplicate plugin path (two cache
loads/1,201 modules). Its actual output/counter/exit observations remain, but it
is not an identical single-search ordinary-runtime comparison. The later input
cancellation receipt qualifies one cache load/601 modules and records this
limitation without discarding the original result.
