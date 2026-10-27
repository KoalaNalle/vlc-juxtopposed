# Precise-seeking activation and MP4 sample lookup

21 October checkpoint: both observed seeking UI P2s pass selected repair checks.
Core `9f9702bec96d…`, ordinary GUI `c382ebb2f89a…` and matching
private GUI `054a3f54b009…`. R8 native passes325 checks in three
normal processes:17 serialized paused seeks, two actual held-pointer timeline
cases and one playing seek. Both cold policies and actual save denial/retry pass.
The held preview differs from the landed position, then release restores0.5;
pointer-specific MP4 branch/request logs are independently checked. Component16
groups include16 pointer/Space error/retry combinations in Dark/Light and four
layouts. Ordinary repeated Fast34.128/38.154 now shows frame750/time30/slider0.5
without Resume; repeated Precise near24.1 reconciles too, then decoded playback
and normal Quit remain clean. All1300 current/frozen inputs and runtime/media
match. The stricter old-Core failure and ordinary old-slider negative remain
bound. Existing player mock tests remain rejected Windows setup evidence.
This closes the two reproduced P2s in these scopes, not full setting/beta,
frame-exact/all-format, physical DPI, visual or final-package acceptance. Continue
the remaining settings consumers and thirteen-category/32-surface roadmap.

Earlier checkpoints below retain their original identities and scopes.

Historical R1-R5 narrative follows. Later R8 receipts above qualify the two
reproduced UI repairs; full setting/beta acceptance remains open.

MainCtx now pairs the Playback precise-seeking capability with its existing
PlayerController consumer. Each of six explicit seek routes reads the process's
saved `input-fast-seek` flag: absolute time/position, relative time, timeline,
short forward and short backward. The help text describes next-seek timing and
media-format differences. Timeline thumbnails remain independently unavailable.

The rebuilt Playback model passes 24 cases, including a new genuinely changed
precise-seek flag under a native config replacement lock, rollback and retry.
Its executable and runtime identities are bound in `model-r4.json`.

## Reproduced MP4 defect and correction

An owned one-minute H.264 MP4 contains 1,500 frames, fixed keyframes at
0/10/20/30/40/50 seconds, no B frames and no chapters. The old MP4 reader maps
two different later-chunk requests to sample 457, then the 10-second keyframe.
Precise seeking hides the poor starting point by decoding forward; Fast exposes
it. Ordinary timeline 24.161 displays 24.160/frame604, but after actual Toggle Off,
timeline 34.084 displays 10.000/frame250 and resumes there. Native R1 independently
passes Precise 24.2 then fails the expected Fast landing. Both exit normally with
clean diagnostics. These are retained product negatives, not tolerance failures.

`STTSToSampleChunk` initializes an absolute track sample index, then incorrectly
compares it with a chunk-local sample count. The corrected guard compares its
offset from the chunk's first sample. It preserves chunk search, edit-list
mapping, sync-sample selection and accurate/fast policy. The preserved old source
matches the Git HEAD blob `c11b998b13ac88a2d5d96de90d31d9d97cc2ed39`.

The extracted production helper regression changes from 1,121 failures to zero
across 1,624 checks: all 1,500 CFR indices, later chunks, variable-duration run
seams, a small nonzero base that previously stopped early, track end and large
valid absolute indices. The harness uses small field-compatible structs and the
actual helper text; it is not whole parser/demux acceptance. The maintained
preparer/runner and C harness are `test/modules/demux/mp4_seek_samples.py` and
`test/modules/demux/mp4_seek_samples.c`. No compiler is installed implicitly.

Ordinary GUI `393619cf5176…`, unchanged Core `d04e9134a43f…` and cache
`1190b018b5e8…`, with corrected MP4 `917af1d1a006…`, replays the same failure:
Fast 34.084 now displays 30.000/frame750. Actual Toggle On restores precise
timeline behavior. Resumed video 40.600 and elapsed 40 agree; pause and normal Quit
remain clean. The process loads one plugin cache and acknowledges Stop before
Qt teardown. Source/runtime/media identities are unchanged. Go-to-Time form
delivery was not qualified by the computer tool: no 44-second seek was issued.
Controller `setTime` testing is a separate native scope.

## Native test revisions and remaining work

R3's corrected reader passes initial precise/Fast/time/position rows, then the
test incorrectly waits for a buffered live log line. The complete line exists
after normal exit. R4 removes live log polling; the parent still requires one
correct MP4 branch and request time between each named BEGIN/END pair after exit.
Paused native clock and a newly displayed picture serialize the requests.

R4 passes all six routes in both modes and the old Fast policy after a rejected
save (13 measured seeks). Its pointer retry is unqualified: the helper checked
window bounds but not the ancestor Flickable's clipping. The single intentional
native replacement denial is retained. No second save error establishes another
store failure. R5 adds explicit production reveal, full viewport bounds,
pointer-state captures and retry signal/state observations. R5 retry and both
cold startup modes pass 231/32/19 checks in three normal processes: 14 edit seeks
and one in each cold process. Actual Toggle restores Fast on one cold start,
then saves Precise for the next. Both reach the matching native MP4 branch,
new displayed picture and paused clock before resuming decoded playback.
The rejected edit preserves exact disk/policy/input/clock; after explicit reveal,
the actual pointer retry emits one values change, clears the error and drives a
Precise seek. One exact native configuration replacement denial is expected and
bound to the deliberate lock, preserved rollback and successful retry checks.
All other native/Qt diagnostics are rejected. No forced cleanup is used.
Private GUI `f85bb0c62638…` shares the ordinary GUI's production objects; its
fixture translation unit is absent from ordinary GUI `393619cf5176…`.

**P2 remains:** while paused after Fast timeline 34.084 lands on the 30.000 frame,
the elapsed label and slider can retain requested 34 until Resume. Requested time
is useful transient feedback, but must reconcile to the actual landing position.
The existing timer path suppresses updates while seeking and clears that flag
without publishing a final point; this is a source-supported cause to investigate,
not yet a proven callback ordering. Qualify the actual timer/media generations
before repairing it. Do not query player state synchronously from native timer
callbacks without checking lock ordering.

**A second P2 is confirmed:** adding the save-error banner moves the already
focused toggle from within the viewport to top 815/bottom 847 in an 815px viewport.
The complete toggle is clipped. R5's explicit production reveal scrolls it back
to top 778/bottom 810, after which pointer retry works. The generated
after-failure/before-retry captures and geometry bind this observation. Functional
retry acceptance includes that explicit scroll setup; it does not close focused
control visibility or the need for readable feedback alongside the control.

Current acceptance covers the recorded MP4/sample lookup and selected explicit
Qt policy paths, not frame-exact output, every format, all settings, physical DPI,
visual fidelity or a clean distributable. Frozen sources, compiled modules,
owned fixtures, complete logs and drivers remain under
`build-windows/beta-precise-seek-20261021`. Earlier profile/theme/quit receipts keep
their original GUI/MP4 tuples. The full beta roadmap remains active.

Independent read-only review found no material new defect or acceptance
overclaim. It verified all 18 published artifact bindings, 1,295 current inputs,
raw logs/reports, model XML, helper negatives/positives, media index and staged
runtime/protected-plugin identities. The two UI P2s above remain explicit.
No further repetition of the accepted unchanged paths is required before
investigating those defects.
