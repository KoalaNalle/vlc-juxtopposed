# Windows playback follow-up, 27 October 2026

This is a scoped evidence receipt, not a beta completion statement. It follows
the [upstream integration receipt](../home-upstream-integration-20261027/README.md).
The runs below used generated local media, isolated profiles and ordinary Windows
11 desktop runtimes unless identified as component or private native tests.
Raw observations, screenshots, frozen patches and runtime manifests remain in
the ignored `build-windows/beta-follow-20261027/` directory. The earlier
`build-windows/upstream-integration-20261027/ui-keyboard-r2/` contains the
baseline keyboard captures. These paths are local evidence, not release assets.

## Build identities

Each follow-up GUI build reports terminal exit 0 and unchanged frozen source.
All share base commit `5994f80169715db8e4ad28b7492a362304b40bbe`; their source
patch hashes distinguish the tested, uncommitted changes. The corresponding
`gui-build-rN-status.json` includes helper and untracked test identities, and
`runtime-rN-manifest.json` records the complete staged binary hashes and guards.

| Build | Source patch SHA-256 | GUI plugin SHA-256 |
| --- | --- | --- |
| r1 | `aa82711733484c025d7cb03596587864aa800543713a8e9f1ec04bff2745345a` | `315dae0b34ff1d38fee1ae225b8ad05263c0f804a230550d207d85214806c0eb` |
| r3 | `add603484cdd6ed876fb0664669d6686a40242cdd54cc5864af8b4837d896dae` | `915fbd58b739cc4e5eafc2a987d1d9fd84901fdfe62c7d6cb59bf1d8030755e6` |
| r4 | `a3063b01c017d95a050b03472a6fbecb2abf9e9f9f5ec953c4d1e8a1ea194485` | `a9990b17a86f16722b0a3b6588f22520ca6e281a78784ac9b179e9ab24265b88` |
| r5 | `c2500a4cd33baee0534d5e56ca8a13189c01459568544ff8d2557868f91b6010` | `2b3846c7b730442b6e94a0e5cdd11871cc2119bd1b9004902a852b6b67d74f3e` |
| r6 | `c1dbd2d2e743f756a405f59d83109a26ad742f2bd7cf22fa6b9c044b47c91f4c` | `3aebd3127dfcf72c729c8b37d9738a61ed80f8721b3d0891073e1603b906e475` |
| r7 | `d8492912a94fabe585f7be3783a98b683097449ef6c20df134e86e941f367084` | `7d03624539d3dfc8e15176f0a4b95da0bec2635472de31621f294e12508ee7ce` |

The manifest guards report the prior runtime and alpha fallback unchanged. A
GUI rebuild over the previously built upstream runtime is not a clean full
rebuild, final package or Windows 10 result. The runtime manifest preserves the
exact complete binary identity; the patch hash alone does not identify untracked
test files.

## Fullscreen and keyboard

The upstream baseline `ui-keyboard-r2` persisted Space play/pause and F entry
followed by Escape exit. It also captured title-bar entry followed by Escape
remaining fullscreen, F entry followed by title-bar clicks remaining fullscreen,
and F11 leaving the title-bar route. Captures 06–19 and `verification.json`
are retained locally; normal process exit does not itself prove the assertions.

The redesigned title strip and F11 were changed to route active embedded-video
fullscreen through the video owner while preserving a separate interface hold.
The offscreen title-bar component test fails the new F11 entry assertion against
the previous shortcut and passes the candidate; it uses supplied synchronous
player and window-holder seams, not native hotkey dispatch. The live
`ui-fullscreen-r2` and `ui-fullscreen-r4` sessions exercised cross-route and Home
transitions. The r3 `ui-f11-escape-r3` session observed the remaining pre-r5
F11-to-Escape failure.

The ordinary r5 run `ui-f11-r5/verification.json` reports exit 0, normal exit,
unchanged runtime, package, fixture, profile and producer guards, and no
diagnostics. During the live session, windowed results were observed after
F11/Escape, F11/title bar, title bar/F11, F11/Home/title bar and Home/F11.
Video stop also returned the window to windowed: fullscreen acquired through
this route follows the video output lifetime. The follow-up capture helpers
used stale lexical bindings, so the `ui-fullscreen-r2`, `ui-fullscreen-r4` and
`ui-f11-r5` saved screenshot labels cannot independently establish those
sequences. A corrected repeat, `ui-f11-evidence-r5`, records a 602×421 window,
2560×1440 after F11, and 602×421 after Escape; it then records F11 fullscreen,
Home while fullscreen and a 602×421 window after the Home title-bar exit. Its
verifier reports normal exit 0, unchanged guards and no diagnostics. The last
Home accessibility snapshot still labels the button “Leave fullscreen” even
though the saved window is windowed. A separate `ui-home-label-r6` settled
windowed Home observation reports the correct “Fullscreen” label; its run exits
normally with intact guards and no diagnostics. It establishes the settled r6
label, not the exact timing of the earlier r5 transition.

Home entry without active player video, audio-only, detached video and classic
controls retain their interface route. Native Escape may still leave that owner
held. Remapped shortcuts, modal and popup priority, rapid toggles, video output
replacement, multiple monitors and independent multi-instance behavior require
separate native/desktop acceptance; the component tests do not establish them.

## Subtitle controls

`ui-fps-r1` has normal exit 0, intact guards and no recorded diagnostics. Its
settled observations show the redesigned external-subtitle frame-rate control at
50, the value from the classic synchronization dialog reflected at approximately
29.97, and the redesigned reset returning it to zero. These are UI state
observations with the generated fixture; they alone do not measure subtitle cue
timing at all rates or establish complete subtitle accessibility.

The corrected `ui-fps-playing-r5` ordinary desktop run exercises the same
generated external subtitles while playing. After setting 50 FPS and seeking,
observation 05 shows the MIDDLE cue at approximately 16 seconds. After resetting
FPS to 0 and seeking, observations 06–07 show the FIRST cue at approximately
14–15 seconds. The run exits 0 with unchanged guards and no diagnostics. On the
initial rate change the old cue remained visible until playback advanced or a
seek occurred, so immediate cue retiming remains unproven.

The separate r6 `ui-fps-live-change-r6` run captures FIRST at about 19 seconds
with FPS 0 and MIDDLE at about 23, 30 and 38 seconds after setting 50 FPS while
playing without a seek. With this fixture's 25 FPS baseline and source cue
times, MIDDLE at 30 and 38 seconds is inconsistent with the 50 FPS timing; the
r5 post-seek run also shows MIDDLE at about 16 seconds. A likely cause is that
the first 0-to-50 change does not flush queued cues when the source subtitle
rate is the zero sentinel. That cause remains unproven until native reproduction
and a tested repair. The earlier native FPS test changed the rate while paused
first and did not cover the untouched sentinel. The r6 desktop verifier records
normal exit 0, unchanged guards and no diagnostics; a clean exit is not a pass
for the observed timing mismatch.

The focused native negative control `fps-fresh-negative-r1` now reproduces the
first live change against the untouched r7 core, in a fresh process using
Direct3D11 snapshots and generated media. FPS initially reported 0; after the
first change to 50 without a player seek, both the old FIRST cue and the new
MIDDLE cue appeared together. The main clock advanced from 12.105 to 12.890 s
and the decoded frame remained in the expected interval. Setup and cleanup
passed, the intended caption assertion failed, and the recorded source, test,
fixture, runtime and producer guards were unchanged. The test executable hash
was `58701525fa47380a57b57b78537859745431a83e947fd516c3ad0b07ca7a1751`.
This supports the flush hypothesis but does not prove a candidate fix.

The source-only flush candidate rebuilt as core SHA-256
`f52731a0b55b55b1576179710a85bfa58b5265da794783162e86f0af9380bb52`.
With the same executable, fixtures and r7 runtime otherwise unchanged,
`fps-fresh-candidate-r1` passes that first-change assertion: the MIDDLE cue
matches, the old FIRST cue is absent, and the clock advanced from 12.112 to
12.881 s without a seek. Setup and cleanup pass, the process exits normally,
and recorded fixture/runtime/producer guards remain intact. This is one scoped
native red/green result; startup, delays, source isolation, ordinary UI and
the final package are not qualified by it. The verbose test log contains media
and decoder warnings and a window-closed error during teardown; those diagnostics
are not evidence of a clean ordinary desktop run.

The candidate is not acceptable yet: in `fps_extra_candidate_positive_r1`, a
fresh process with a +5 s subtitle delay showed no cue after the first live FPS
change at about 12 s, although the expected FIRST cue was visible in the
baseline run. The test's caption assertion fails while the clock advances and
the recorded guards and cleanup pass. Since FIRST is expected both before and
after this particular change, the baseline pass alone does not test replacement;
the candidate's missing cue is nevertheless an observable regression. The
rate-change refill must also account for delayed cues.

The candidate passed a separate -5 s case that fails on the baseline, with a
matching MIDDLE caption. A pre-set player FPS of 50 before input creation also
showed the expected retimed cue on both baseline and candidate. This emulates
an inherited player value and is not a saved-settings restart test. These
fresh-process runs recorded normal test teardown and intact guards.

A second experimental core, SHA-256
`ad3e761ae4cc1b0473e96b2b3f21ca5cb9418940dcbd5b3f3bb330e0f6db8016`,
refills the affected subtitle source from its positive-delay preroll after
changing rate. `fps-fresh-candidate-r2` passes the first-change test and
`fps_extra_candidate2_positive_r1` passes the +5 s case which failed with the
first candidate. Further fresh-process tests with this candidate pass -5 s,
and a +10 s delay at 24 s where the correct cue changes from FIRST to MIDDLE;
the latter case fails on the baseline. A pre-set player FPS of 50 before input
creation also passes. The processes record normal cleanup and intact guards.
The pre-set value is not proof of saved-settings persistence or restart.
The expanded test executable SHA-256 is
`01c52235fd1d80b8b30b835cda57ca9a120bf1e74ad2626d04f2cf68a73ea288`;
results are in the `fps_extra_candidate2_*_r1` and
`fps_extra_baseline_change_r1` run directories under `beta-follow-20261027`.

This is still limited native evidence, not general acceptance. If a subtitle
source refuses the preroll seek, the code warns and continues best-effort;
it may still lose an earlier delayed cue. Nonseekable sources, overlapping or
short cues, multiple per-track delays, other formats, saved settings, newly
added sources, EOF, ordinary playback and stream-output interactions remain
open. These results do not qualify a final package.

Before accepting a guard change, verify startup with a saved nonzero FPS, newly
added and selected subtitle sources, and that the change causes no audio/video
seek. Source inspection also suggests that refilling after a rate change may
start too late for a positively delayed cue; reproduce or disprove this with
positive and negative delays, including per-track delays.

In `ui-cue-focus-r6`, the corrected desktop captures show repeated Enter on
the focused subtitle-timeline Zoom In control changing the visible range from
15 to 7 to 3 seconds. The verifier records normal exit 0, unchanged guards and
no diagnostics. This does not establish focus behavior for every cue source
change or all keyboard navigation paths.

`ui-tracks-r3` also exits normally with clean guards. Its captures show primary
and secondary values of +5000 and -9000 and frame rate 50. A screenshot helper
captured stale state for the later reset and estimate observations, so those
files still show earlier values despite changes seen in live tool output. The
saved r3 observations do **not** qualify successful resets or estimates.

The corrected ordinary `ui-tracks-evidence-r5` capture shows +5000/-9000/50,
then primary reset to 0/-9000/50, secondary reset to 0/0/50, and frame-rate
reset to 0/0/0. Subsequent primary and secondary estimates display -3817/0 and
-3817/+4003 respectively. The run exits 0 with no diagnostics and its guards
intact. These are observed UI values in one generated-media scenario, not an
independent timing-accuracy measurement across all media and rates.

`subtitle-audio-selection-r4/verification.json` and its r5 counterpart each
pass audio selection and unselection timing-context cancellation in the actual
production controller, with generated media and dummy audio/video outputs.
They are private native fixtures, not ordinary desktop or WASAPI acceptance;
both record normal exit 0, clean diagnostics and unchanged source and runtime
guards. Their test executable SHA-256 is
`ca83559b91acbef6ca11a60db7f4f11ab5de71dcdb5dda91539c3977ca55805e`.

The fresh `subtitle-audio-full-matrix-r6/verification.json` passes all 16
`timingContextCancellation` data rows plus `audioTimingRevisionGuardsNativeChanges`,
with no skips, Qt diagnostics or forced cleanup. Its source, package, runtime,
fixtures, frozen pages, profile and producer guards pass. It uses a private
native fixture with dummy outputs, not an ordinary desktop or WASAPI check.
The tested executable SHA-256 is
`d3e904e932392ff1b35ec642e131ff9242367a54ad53734c21f4f0a5c28b344f`.

The ordinary `ui-two-audio-r6` capture shows an initial pending audio mark,
an edited delay of 500 ms clearing it, a new first mark leaving 500 ms intact,
and the completed pair producing -6313 ms. Its verifier reports normal exit 0,
no diagnostics and intact runtime, package, fixture, profile and producer guards.
The separate `ui-two-audio-global-r6` track-cycle attempt left track A selected;
it does not establish behavior after a global track switch despite clean exit
and guards. The controller's selected-track override and races involving queued
rate, playback-state or seek events remain open for focused review and tests.

The r7 `subtitle-audio-full-matrix-r7/verification.json` passes the same 16
timing-context rows and both native audio revision checks, with no skips or Qt
diagnostics and all recorded guards intact. Its private fixture uses dummy
outputs; the test executable SHA-256 is
`8bac84ff74bd6bc2f54e85b16ca5ad1db53511ca316bca1877575ab846618c36`.
Ordinary `ui-audio-timing-r7` observations 01–04 show a pending mark, a 500 ms
delay edit clearing it, a fresh first mark keeping 500 ms and a completed pair
at -5621 ms. The verifier reports normal exit 0, intact guards and no
diagnostics. These results do not establish that the selected-track override or
the queued seek race is fixed.

## Recording error notification

`ui-notice-r4/verification.json` records the expected controlled recording
output-open failure, normal exit 0, no unexpected diagnostics and intact
runtime/profile/fixture guards. Observation 01 captures the real recording
error notice and Errors details; observation 03 captures a later state with no
notice. The seven-second hover hold was seen during the live session, but saved
observation 02 and its accessibility tree captured stale state and do not show
the notice. The persisted evidence does not independently establish that hover
result.

The r5b repeat used a corrected capture helper. Its saved observations
`03-details-focus-settled` and `04-enter-real-messages` show the visible keyboard
focus indicator on Show Details and the real Messages/Errors pane after Enter.
The notice remained on hover for more than one minute during the live session;
`05-dismiss-focus` and `06-space-dismissed` capture the subsequent keyboard
dismissal. `ui-notice-r5b/verification.json` reports the expected controlled
output-open failure, normal exit 0, no unexpected diagnostics and intact guards.

The `beta-notice-interaction-20261027/component-r12/verification.json` passes
eleven notice lifetime, pointer/keyboard focus, dismissal, focus-restoration and
optional-notification cases against production QML with supplied boundaries and
no Qt messages. That is component evidence; native focus, screen-reader use and
any later repeat need separate records.
