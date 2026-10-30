# Upstream integration, 27 October 2026

This is a frozen, scoped receipt, not a beta completion statement or a resume
script. The owner requested a separate integration branch, conflict resolution,
a rebuild and playback tests, while preserving the working alpha.

## Source and fallback

- Integration branch: `codex/upstream-20261027`.
- Pinned upstream: `874d229c1896037992f9d10b06ce6c341f77c0eb` (26 October),
  195 commits and 237 changed files from common base `1176b56bde`.
- Merge: `06c97dc7867ef5627467923db94002495c0b21db`.
- Ordinary production rebuild: `c7cdbdb5d7ed8bd7023252e88fad8a5344ea14c9`.
- Later changes concern test setup, build declarations, CI and documentation.
  The final native CSD test target was rebuilt at
  `7c54e67517971ef1d4daa9bf3226776c3b943b79`.
- `master` and `codex/alpha-fallback-20261027` both remain
  `0694227ab8da7166ac1cf9e483f3a4836e2cc68c`.
- The old ordinary alpha runtime remains at
  `build-windows/beta-single-frame-20261025-night/ui-record-error-fixed-r478/runtime`.
  All 532 original file hashes were checked again after testing and match
  [fallback.json](fallback.json). No branch switch or DLL replacement is needed
  to launch that fallback. Its old test profile is not a fresh profile.

This integration was not merged into master, pushed, published or released.
The earlier green master CI receipt remains historical; it does not establish
GitHub CI success for this branch.

## Merge decisions and regression repair

There were 35 overlapping files and four textual conflicts:

1. `mmdevice.c`: keep the existing owning-audio-thread stop/teardown repair.
   It already stops under the audio lock; handle closure follows worker return,
   and queued audio blocks are released. Do not stack upstream's alternative
   locking change on that architecture.
2. `MenuTopbar.qml`: combine upstream's toolbar-menu availability predicate with
   the redesigned Windows title strip's menu exclusion.
3. `NoMedialibHome.qml`: retain upstream's Open File split button and all six
   secondary routes, alongside the redesigned Add Folder action only when the
   library is available. Preserve the traditional-controls opt-out.
4. `test/modules/meson.build`: retain both the local audio/library tests and
   upstream's new blend test.

A clean textual merge also introduced a duplicate transition key in the toolbar
state machine. The appended upstream video-embedding transition overrode the
explicit auto-hide policy. The original component replay failed two cases:
leaving fullscreen and switching from legacy interactive mode to explicit Never.
Commit `c7cdbdb5d7` keeps the existing policy-aware transition and adds legacy
video-to-audio checks. The repaired suite passes **49/49**, with no Qt warnings.
The ordinary app also returns from fullscreen with controls visible and retains
them after the timeout.

The maintained no-library Home test passes four combinations of interface mode
and library availability. It checks the actual production QML's advertised menu,
all seven open routes including the primary action, and Add Folder availability,
placement and routing. An isolated copy of unmerged upstream Home fails the
missing Add Folder assertion as intended. The first negative fixture failed
earlier because the observer relied on a root import; that observer was repaired
to inspect the supplied dialog boundary directly before accepting the negative
control. This is component evidence; native picker/menu behavior is separate.

## Build and runtime identity

The established home WSL distro, GCC 13 POSIX and Qt 6.8.3 were reused. No new
dependencies were installed. [environment.json](environment.json) records the
actual cached dependency versions and pkg-config identities; updated upstream
contrib recipes were not all rebuilt. Native validation uses Windows 11 Pro
10.0.26200 x64. Windows 10 and Meson builds were not executed.

The first full build reached an obsolete `video_format_IsSameChroma` reference
in an old D3D9 object. Upstream moved that helper into a public header, but this
imported cache had 1,792 dummy dependency files. Under the single-writer build
lock, 2,417 stale object files (595,575,840 bytes) predating the changed header
were moved into a WSL quarantine with hashes verified before and after each move.
Freshly rebuilt objects were retained. The quarantine is
`/root/vlc-juxtopposed-build/owned-upstream-stale-objects-20261027`; its full
inventory remains ignored locally. The next full build and DESTDIR install
completed with exit 0 and clean source guards. The rebuilt D3D9 object no longer
imports the removed symbol. Fresh dependency lists contain actual headers.

Early preflight attempts were stopped or failed before compilation because of
slow Git access through NTFS and a WSL interop timeout. A later in-progress build
was stopped normally before repairing the toolbar source. These attempts are
retained in the ignored work directory and are not accepted build results.
Libtool emitted path-conversion warnings for uninstalled test launchers; tests
used the actual `.libs` Windows executables, whose exit results are recorded.

Fresh ordinary runtime:
`build-windows/upstream-integration-20261027/runtime-r2`.
[runtime.json](runtime.json) records all **532 files / 2,829,710,060 bytes**.
It contains the fresh install's executable, matching Core/GUI/codecs, Lua and
resources; no DLLs were inherited from the old alpha. It has no private native
test hooks. Separate desktop clones used fresh portable profiles and loaded
603 plugins. The first staging attempt used the wrong installed path for the
cache generator and was abandoned; `runtime-r1` is not an accepted runtime.
The large debug runtime is a development test folder, not a licensed final
distribution or package-size result.

## Accepted checks

| Scope | Result |
| --- | --- |
| Local workflow commands | 31 Windows and four Linux checks pass in the combined local evidence. The 29 unaffected Windows suites come from the complete replay; the repaired toolbar and new Home checks were run separately. This is not a new GitHub CI run. |
| Tooltip component | Five literal-text/shaping cases pass with no Qt messages. |
| MMDevice production implementation, controlled backend | 32 checks pass, including worker ownership, stop order, released blocks and closed handles; normal exit, no forced cleanup. |
| Real WASAPI lifecycle | 16 stop/reopen or device-reselection cycles plus eight missing-endpoint fallbacks pass; actual decoded/played audio and normal release. |
| Recording start unit | 22 checks, zero failures. |
| Recording consumers | Both native-recording paths produce actual output that decodes through VLC. |
| Paused seek | Generated RGB video seeks to distinct red/blue/green target frames, holds its clock and resumes. |
| Subtitles and playback | Marker MP4, B-frame MP4 and Matroska: actual embedded/external tracks, held seek, delay Reset, captions, resume, WASAPI and normal teardown pass. Matroska also selects both embedded languages. |
| CSD model native QtTest | Nine methods plus init/cleanup: 11 passed, zero failed/skipped/warnings, normal exit. |
| Ordinary redesigned app | MP4 cold paused/external subtitle, pointer play, Matroska native picker/open, play/pause/held seek, embedded caption, fullscreen idle hide, pointer fullscreen exit/control retention, maximize/restore and Close. Actual VLC exits 0; all binary/fixture/human-config guards pass. |
| Ordinary traditional app | Six native split-menu entries visible; Escape dismissal, primary native picker/open, Matroska play/pause/held seek and Ctrl+Q normal exit. Observer exits 0, no diagnostics, all guards pass. |

See [workflow-local.json](workflow-local.json), [native-summary.json](native-summary.json),
[csd-native.txt](csd-native.txt) and [desktop-reviewed.json](desktop-reviewed.json).
[Native test executable identities](native-test-binaries.json) are separate from
the ordinary runtime manifest.
Generated fixtures were used; personal media and raw personal logs are excluded.
There were no firewall rules, security/privacy settings or installed-user VLC
configuration changes. All owned desktop/native test processes are closed.

The new upstream CSD standalone test initially timed out before producing results:
static Windows Qt needed its offscreen platform plugin registered and linked.
The setup now imports the static plugin, uses the existing narrow winpthreads
shutdown shim, links the plugin before its Qt dependencies and defines real file
prerequisites. Initial link-order and Automake prerequisite attempts failed;
they remain in `csd-r6`, `csd-r8` and `csd-r9`. Only final `csd-r10` plus its
normal native run qualifies the final Autotools target. Meson's equivalent setup
uses Qt's plugin directory but has only been reviewed, not built here.

## Limits and diagnostic review

The redesigned Close run's strict observer exited 1 for exactly one
`main error: window closed` line. The actual app exited 0 after full teardown.
Source `vout_display_window_CloseNotify` emits that line at error level for its
close callback; it followed the owned Close click. The original failure remains
preserved. The review classifies only that one close callback; it does not ignore
other diagnostics. The traditional Ctrl+Q run has no such diagnostic.

Some immediate accessibility/capture observations lagged the input. Settled
frames and clocks were used for acceptance. An early indexed Play click sought
instead; later pointer actions were qualified. Redesigned Space, F and Escape
attempts did not establish the intended action in this run, so their keyboard
paths remain unqualified. The pointer fullscreen exit passed. Do not infer a
keyboard repair or blame the app without a controlled reproduction/comparison.

The redesigned screen captures were displayed during testing, but the initial
evidence helper saved only PNG payloads while the tool supplied JPEGs. Its saved
accessibility/time/dimension observations are retained under `observations/`;
there is no persisted redesigned image comparison in this receipt. The classic
JPEG captures under `captures/` were saved correctly. This does not establish
visual fidelity scores, screen-reader behavior, real DPI coverage or any beta
readiness gate. Windows 10, hardware endpoint variety and the outstanding beta
work in BETA/SETTINGS remain open.

Frozen producers are retained under `producers/` for provenance. They contain
one-use paths and preconditions; review and adapt them before any future run.
