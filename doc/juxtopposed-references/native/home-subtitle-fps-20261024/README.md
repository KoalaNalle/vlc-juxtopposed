# Subtitle FPS repair checkpoint - 24 October 2026

Paused at the owner's request; all owned test processes have finished. The new
Core repairs paused external-subtitle retiming and preserves the main clock
during slave-rate changes. The shared settings editor also commits Windows
accessibility text writes. This is a tested development checkpoint, with one
remaining ordinary desktop Reset failure; the FPS workflow and full beta are
not accepted yet.

## Verified scope

The same native test executable fails against old Core7ab80750 and passes all
eleven functions (including setup/cleanup) on regular Coreb62f4145. An actual
associated SRT is selected by source identity, even when the media contains
embedded tracks. At a held 12-second frame, FPS50 immediately shows the middle
cue;29.97/23.976/0 restore the first cue without a user seek. The held video region
matches exactly; native time stays within50ms. An explicit seek at50 and subsequent
Reset also pass. This test uses an opt-in snapshot environment and a dummy output;
it establishes native behavior, not ordinary desktop or physical-output acceptance.

The eleven earlier delay cases and24 startup/interval/track cases all pass on the
new regular Core. Every child exits normally. The offscreen production-settings
component suite is rerun at wrap-up and passes, including accessibility-style
text-property edits and model refreshes without writes back to the engine.

The ordinary desktop tuple is Coreb62f4145 / GUI79aef2b5 / MKVf46d8d15 /
WASAPI4e4e33c0. Explicit Track3 selection proves external captions. A real Windows
accessibility edit commits50 FPS and immediately changes the paused caption,
with the visible marker and coarse12-second time held. The older Synchronization
dialog reads50 and setting29.97 there immediately restores the earlier cue.
The QML spinner receives that value, accepts23.976 and a focused0.1 Up increment,
and changing to50 immediately shows the middle cue again. The process then exits0
normally with no checked runtime diagnostics; all source/runtime/fixture guards
remain true. These are individual observed passes, not whole-workflow acceptance.

## Open failure and next step

After the older Synchronization dialog has been opened and closed, QML Reset
settles at1 FPS instead of normal timing0. The caption returns to the first cue,
but native state is wrong. Inspect the legacy widget's allowed range and model
feedback next; a clamp/write-back is a hypothesis, not an established cause.
Do not treat this session as an accepted Reset workflow.

Then test ordinary seek at50, Reset after that seek, resumed playback, changing
FPS while playing, next-media/default behavior and direct playing Quit. Visual
secondary timing and estimators remain next on the larger beta roadmap. Do not
reopen desktop tests while the owner is using the computer. The tested runtime
is local at build-windows/beta-subtitle-fps-20261024/ui-fps-r7/runtime; it is not a
release package. No Windows10, audibility or final design/scaling claim is made.

## Evidence corrections and limits

Earlier native r2/r3 rendered attempts selected embedded captions before external
tracks loaded, so they do not test external FPS. The corrected r5 comparison
used external captions but an exact binary caption mask falsely rejected five
antialias threshold pixels over different backgrounds. r6 uses99.5% mask IoU
(actual correct cue99.819%, stale cue17.565%) and retains exact held-video matching.
The r6 before/after images, XML and raw stderr are preserved here. Raw XML and
selected output streams use gzip compression without changing their bytes;
checkpoint.json records their uncompressed hashes. The before
verification inherited phase=after; its old Core hash and failure are authoritative.
The original records are not rewritten. The comparison JSON's future-tense
rerun field predates the completed r6 executions; r6 XML supersedes that status.

The corrected native fixture uses supported --no-hw-dec for dummy software
blending. Native warning/window-closed diagnostics remain recorded; these are
not clean desktop playback runs. Earlier ordinary r1 failures (discarded
accessibility text, stale paused caption, seek settling near10s after a12s
request) are preserved separately. Exact GUI numeric-position conservation is
still unqualified because values can reconcile by about1us.

The2706-input manifest and every actual immutable source copy were rechecked
before wrap-up documentation updates. Copies are reused from ignored local
workspaces; manifests retain those absolute references and do not bundle every
input. Binaries, full native replay image trees and GUI logs remain local;
portable reports contain exact hashes and structured results. Protected unrelated
audio experiments are excluded. Build logs retain compiler/libtool warnings.
No release or installation is produced by this checkpoint.
