# Subtitle refill investigation, 28 October 2026

The current local branch `codex/beta-20261028` preserves the user's manual
checkpoint `2db942f7bf`. Its matching temporary remote branch
`codex/upstream-20261027` was deleted as requested after checking its tip.
The alpha fallback remains on `master` and `codex/alpha-fallback-20261027`.

The prior `fps_both_track_delays_candidate2_r1` run failed after a first live
FPS change with two external subtitle sources and independent +10 s/-5 s
delays. Setup, role/clock/frame checks, normal cleanup and recorded integrity
guards pass; the secondary cue assertion fails and the captured image has no
captions. The primary assertion was not reached. Test executable SHA-256 is
`060ce7b2c81d24f61b6e67bfedd0859c1616185e73db3982a7210fdd8bad54b3`;
experimental core SHA-256 is
`ad3e761ae4cc1b0473e96b2b3f21ca5cb9418940dcbd5b3f3bb330e0f6db8016`.

Follow-up inspection decodes frame marker 624 (24.96 s) in both the prior r2
and new r3 captures. The secondary cue ends at 24.95 s after applying FPS 50
and -5 s delay; its absence at that point is correct. The old assertion is
therefore invalid for that late capture. The primary cue should still be
visible: it is missing on r2 and present on r3. The revised test starts the
live change at 23 s, compares references at 26/56 s, and requires the captured
frame to precede 24.48 s so both expectations remain inside their valid ranges.
The original artifacts are preserved unchanged.

The safe 23 s test passes on r2, so it alone does not prove the buffered-time
repair. The stronger `opposed-secondary-ended` row at 26 s correctly expects
the secondary to be empty and the primary to remain visible. On r2 it fails
the primary assertion; on r3 it passes. The new matrix also passes opposed,
swapped, both-positive and both-negative selected-track delays, each in a fresh
native process. All five r3 runs have normal setup/cleanup, exit 0 and intact
recorded guards. Run directories are `track_delays_r3_*`; the r2 control is
`track_delays_boundary_r2`, under `build-windows/beta-fps-20261028/`.

R3 core SHA-256 is
`6e67d4b1cce3e2341464a1d56e1f7446701799c0325d3fbe5a842ea43b84f8b0`;
matrix test executable SHA-256 is
`ec9b82a68fe50a47b787c0eae2696e5dcdba43055976887b7203948d3b82e718`.

The first nonseekable attempt (`nonseek_r3`) failed in fixture setup: the
selection helper seeks to establish output roles. It is not evidence about
FPS behavior. The revised fixture loads `noseek` before input creation, selects
its first external source without that helper, and advances naturally to 12 s.
`nonseek_r3_live` then fails the intended visible-caption assertion after the
first 0-to-50 FPS change with +5 s delay. Its initial caption, clock/frame,
setup/cleanup and integrity guards pass; logs confirm three loaded filters and
the refused preroll seek. Test executable SHA-256 is
`634e8d70aa5ef8a361e9d99b391c22f007eb1b926d0b3cbcdd669e9fd486ee58`.

R4 sets an earlier demux deadline before changing rate, allowing the text
subtitle demux to reset its cue index without seeking. It restores the current
deadline before normal demuxing. Demuxers lacking that deadline control retain
the source-only seek fallback. `nonseek_r4` passes the same test which fails on
r3; the logs confirm all three filters and no refused-preroll warning. R4 core
SHA-256 is `22212a926e2500f91946e884d51c83d7ae83112c839156409b9182fa74bb445f`.

Fifteen further fresh-process runs (`regression_r4_01` through `_15`) pass with
no skips, normal child exit and intact recorded guards. They cover the first
live change, paused/playing embedded-source isolation, two external sources,
all five selected-track delay rows, three category-delay rows, held-frame
fractional FPS/reset, playing changes and EOF revival, and a pre-set player
value before input creation. That pre-set value is not a saved-settings restart.
Native verbose logs retain fixture/decoder and teardown warnings; these runs
do not assert a diagnostic-free ordinary package.

## Ordinary Windows observation

`ui_r4` ended before the FPS edit was committed, so it supplies no live-change
acceptance. Its child exits normally and its recorded guards pass.

`ui_r4_slow` uses the ordinary r7 GUI/resources/plugins plus the identified r4
core, an isolated profile, generated media, start time 18 s and playback rate
0.25. Saved observations show delay 10 s and initial FPS 0, then the first live
edit to 50. The settled capture at decoded frame 756 (30.24 s) displays LAST,
as required by the new rate and delay. The immediate post-keypress capture
still contains the old cue; it is not the settled result. The test performs no
seek during that edit.

Paused Reset to 0 changes LAST to MIDDLE while retaining decoded frame 990
(39.6 s) and elapsed label 21 s. The saved video-only region has identical
SHA-256 `f27636e08a8022b5bae006648ed0f2e6a8a7ec5404a9aec72a9ee280480d8baf`
before and after Reset. The different displayed elapsed origin comes from the
18 s input start option. This is one ordinary SRT journey at slow playback,
not every rate, format, profile or Windows DPI scenario.

The child exits 0 through the title-bar Close control, and all runtime, original
package, fixture, producer and human-configuration guards pass. The strict
launcher flags the sole log diagnostic `main error: window closed`; source
inspection locates that unconditional close notification in
`src/video_output/video_window.c:143`, followed by normal teardown. This is
reviewed closure evidence, not a completely clean log. Ctrl+Q did not close
the app while the timing editor popup was active; that keyboard route remains
an interaction check.

Formats beyond SRT, overlapping cues, late source attachment, saved defaults,
stream-output/recording and final-package qualification remain open.

## ASS renderer follow-up

The generated `media-formats/subtitle-style.ass` uses three matching timed
English cues and the actual libass decoder. Its first attempt (`ass_first_r4`)
stops during the fixture's Freetype appearance-availability check. It does not
reach the FPS test. An explicit timing-only fixture mode removes that unrelated
renderer-appearance prerequisite, retaining native track, frame, clock and cue
assertions.

With the unmodified libass plugin, `ass_first_r4_v2` reaches and fails the
intended first-live-change assertion: the old FIRST cue remains, even though
the demux log resends MIDDLE with new timestamps. The plugin's Flush resets
its decoder timing fields but retains libass events and their duplicate
ReadOrder identifiers. The candidate calls `ass_flush_events` under the same
mutex used for rendering and event updates.

`ass_first_flush_r1` then passes with core r4 unchanged and only the plugin
replaced. Plugin SHA-256 is
`89518e54ae4841cea5bae94d99b4ea9be54d44d5b6297985cf720b82613450c6`;
test executable SHA-256 is
`f79dd278efb5c3c81473a77f6fc9842c3dff50e8f83b31d474d363f115208a77`.
Seven further fresh runs (`ass_regression_01` through `_07`) pass held-frame
fractional FPS/reset, playing/EOF, positive delay, mixed ASS/SRT track delays,
nonseekable input, a pre-set FPS before input creation and playing embedded
source isolation. All have normal exit and intact recorded guards, with no
skipped functions. These are generated-format native checks; ordinary ASS,
overlapping events and the remaining format/persistence gates stay open.

## Overlapping events

The `media-overlap` ASS fixture extends FIRST to 50 s while MIDDLE spans
20–40 s. A new test builds its reference by uninterrupted playback to 24 s,
showing both cues; it does not use the seek index as its oracle. It then returns
to 12 s and performs the first live 0-to-50 FPS edit. Both cues should overlap
there under the new rate.

`overlap_before` loses FIRST and fails the intended caption assertion, with
normal setup/cleanup and intact guards. The text demux had restarted only from
the latest cue's start. Its candidate index now selects the earliest explicitly
timed cue still active at the target, retaining the existing fallback for
unknown ends. `overlap_after` passes with only that demux plugin additionally
replaced. Plugin SHA-256 is
`1e8abb4593f82b5badf9dc9d103f655d4a0f30334c28f89e24b102b301139ea8`;
test executable SHA-256 is
`b0f4fce01f4b581d4dd87b5457d0a8e2d06e3fa97d0f0330fa125cecbfbcdfc9`.

The combined candidate is core r4, the libass flush candidate and this text
demux candidate. Sixteen SRT (`combined_srt_01`–`_16`) and eight ASS
(`combined_ass_01`–`_08`) fresh-process regressions pass with no skips, normal
child exit and intact recorded guards. `overlap_paused_seek` additionally
compares a paused seek with the uninterrupted two-cue reference and passes,
then repeats the live FPS overlap check. Its executable SHA-256 is
`4e518b5a5477857e9794fc9ad5c99c1853986ef1966e03409686bdea342d928a`.
The ordinary SRT observation above predates the two plugin changes.

## Remaining ordinary ASS failure

`ui_combined_ass` uses the combined tuple, an ordinary isolated profile,
start-paused/start-time 18 and rate 0.25. Initial and early playing screenshots
have no ASS caption. A later MIDDLE cue appears before the first FPS edit.
After FPS 50 is committed with delay 10 s, the settled capture at decoded
frame 918 (36.72 s) has no caption although LAST should remain visible until
39.95 s. The log confirms the new LAST event was sent. The still-later paused
capture at 53.88 s is outside that interval and proves no missing cue by itself.
This is an unresolved ordinary failure, not acceptance of the combined tuple.

The child exits normally; source, runtime, original package, fixture, producer
and human-configuration guards all pass. As with the earlier title-bar Close,
the strict launcher flags the reviewed `window closed` notification. The
timing/output failure is independently visible in the saved observations.
Native slow-rate tests are being used to separate the rate/delay intersection
from startup at a nonzero position before further production changes.

## Slow-playback source-clock conversion

`slow_ass_01` fails while obtaining nonempty reference captions, before its
FPS setter runs. The two delayed cases also have blank references; their
failures cannot establish an FPS-specific defect. These fresh inputs do not
use a clipped start, separating the fault from the ordinary test's start time.

The libass updater previously added elapsed display time directly to the
source cue timestamp. At quarter speed this advances its event clock too
quickly. Candidate `ass-plugin-r2` uses the already exported
`subpicture_GetSourceTime` API, whose SPU mapping includes rate and pause
adjustments. Its SHA-256 is
`b622871c205345373343a463f193271b12cf875853e7c913c707acadb16e9b40`.
Core r4 and the subtitle demux plugin remain unchanged.

`slow_fixed_01` restores both reference captions, then fails the live-change
assertion after the original fixed 700 ms wall-time wait. Its log has not yet
processed the rate refill by that capture. The revised fixture allows 700 ms
of media-time demux progress at the selected rate, retaining the independent
clock/frame bounds; it does not claim immediate response. Delayed cases now
also explicitly require a rendered, nonempty reference before the FPS edit.

`slow_mapped_01`–`_03` pass the first live FPS change with 0, 5 and 10 s delays
at rate 0.25. Setup, selected function and cleanup pass with no skips, native
exit 0 and intact source/test/fixture/runtime/producer guards. Test executable
SHA-256 is `4db3af93057d80660052efe8761ed4319accdf20ec52bdd4cd238c43cd0f32bf`;
source SHA-256 is
`bbb4887d2bf8324b755c5202f9017851348d688e69e144562b4744ce1418d255`.
`mapped_normal_01` and `_02` additionally pass the normal-rate first change and
held-frame fractional-FPS/Reset checks. `mapped_overlap_01` passes both paused
seek and live FPS overlap against the uninterrupted reference. All three have
exit 0, no skips and the same intact guards. Ordinary ASS replay, response
latency, remaining formats and final-package qualification stay open.

## Ordinary ASS with the source-clock mapping

`ui_ass_mapped_r2` repeats the ordinary clipped-start/quarter-speed journey
with core r4, libass r2 and text demux r1, using the frozen
`run-ui-ass-mapped-r2.py` producer. The initial paused frame 449 (17.96 s) now
shows FIRST, including after applying a 10 s delay. FIRST is also visible
during playback at frame 591 (23.64 s), with FPS still 0.

The first live 0-to-50 edit has an immediate capture at frame 676 (27.04 s)
which still shows FIRST. A capture approximately 8 s after the Return action
shows MIDDLE at frame 725 (29 s); later frame 784 (31.36 s) shows LAST. These
are the expected settled cues with 10 s delay. The observations bound the
transition only loosely and do not establish an immediate response.

At paused frame 826 (33.04 s), entering 0 in the classic Synchronization FPS
field changes LAST to MIDDLE. Reopening the redesigned panel shows FPS 0 and
delay 10 s. The video-only region SHA-256 before/after is identical:
`5eebfd4f3cce0f96f219c664e7f00278f188e9daac15a84e9134362d598763fb`.
The displayed elapsed label refreshes from 14 to 15 seconds; the frame remains
826 throughout. This is decoded-frame preservation, not an independent native
clock measurement. Raw observations and `frame-analysis.json` are under the
run's `observations/`; the frozen analyzer records each full-image hash too.

Observations named `09-cross-editor-reset` and `10-cross-editor-reset-settled`
actually show the classic **refresh** button, which leaves values unchanged.
The successful restoration is `11-classic-fps-zero-settled` and
`12-redesigned-fps-zero`. The refresh button has no accessible name and its
source tooltip incorrectly says "Eject the disc"; this remains an interaction
cleanup item. The cross-monitor dialog required its own screenshot coordinates:
parent-window element clicking was rejected as out of bounds. Keyboard text
entry worked after focusing the actual field using that screenshot.

The ordinary child exits 0 through the title-bar Close control. All source,
runtime, baseline package, fixture, human configuration and producer guards
pass. The strict launcher exits 1 for the sole reviewed `main error: window
closed` notification, as in the earlier ordinary run. No owned test processes
remain. This qualifies the recorded generated ASS journey, not all W1 formats,
saved defaults, response deadlines or the final package.

## Prompt FPS command handling

`prompt_before_r4` enables the explicit short-response fixture mode at playback
rate 0.25. References are valid, but after the 700 ms wall-time settling wait
the rendered caption still matches FIRST instead of MIDDLE. The native child
exits 1 at that assertion; setup, cleanup and all recorded guards pass.

The FPS input control previously changed the requested rate, then waited for
the next main demux iteration to apply it to associated sources. Candidate
core r5 invokes the existing source demux path when handling that control,
including when the main demux is waiting on buffered output or playback is
paused. It retains the source-only refill/flush and does not seek the primary
input. Core SHA-256 is
`f8fc980bbe03da57908f79045f266710dd27a16c6ee66b6c45d60cd1f9b93598`.

`prompt_after_01`–`_03` pass the same short wait with 0, 5 and 10 s delays,
using unchanged libass r2 and text demux r1. The first case is a direct
before/after comparison. Test executable SHA-256 is
`6346aad75d437ede05b9257ac4ffcc866cd75253040ee50d2a1dd256b4ffccc3`;
source SHA-256 is
`11e640fcec9d81b8e233518731daf8bdb90a8275b230295bd69301f2a2389b0a`.
The 700 ms settling wait excludes callback/snapshot overhead and does not
establish an exact UI latency bound. Ordinary core r5 verification remains open.

Five further r5 runs pass: `core5_held_ass` covers held frames and clock at
50/29.970/23.976/0 FPS; `core5_playing_ass` covers playing changes and subtitle
EOF revival; `core5_overlap` covers paused seek and live overlapping cues;
`core5_nonseek_srt` covers the nonseekable delayed source; `core5_embedded`
covers playing external-source retiming with embedded-secondary isolation.
Together with the three prompt-response runs, all eight have no skips, native
exit 0 and intact recorded guards. All owned build/test jobs are terminal.

## Ordinary core r5 verification

`ui_ass_prompt_r5` uses frozen launcher `run-ui-ass-prompt-r5.py` (SHA-256
`004bbd03c521aaadddade1029c52862110456088365093682418b744024177d0`),
r7 GUI/resources, core r5, libass r2 and text demux r1. The generated ASS
fixture starts paused at frame 449/17.96 s, then plays at quarter speed with
a 10 s subtitle delay. FIRST is visible before the live 0-to-50 FPS change.
The immediate capture 196 ms after Return still shows FIRST at frame 652;
the next capture, 6.362 s after Return, shows MIDDLE at frame 690/27.60 s.
LAST follows at frame 779/31.16 s. These observations establish correct live
output, but do not establish a subsecond live-response bound.

After pausing settles on frame 781/31.24 s, entering FPS 0 in the redesigned
editor changes LAST to MIDDLE. Both the immediate and later capture (993 ms
after Return, including a 700 ms wait) preserve frame 781 and the video-region
hash `1dcb1b8d1044d31f627d5a59494b93d1537eae9f09c11245d5bf2414bda69d7d`.
The displayed elapsed label stays 13 s. This is ordinary held-frame evidence;
the separate native tests provide independent clock checks.

`analyze-prompt-r5-observations.py` records hashes and markers in the run's
`observations/frame-analysis.json`. Exclude `00-initial-r5` from marker
interpretation: it captures the maximize animation before the video geometry
settles, so the fixed-coordinate decoder does not apply. The settled `01`
capture supplies startup evidence. The delay-change OSD also overlaps the
video-only comparison region in `02`; that image is not a frame-hash equality
control. Neither limitation affects the held-frame comparison above.

VLC exits 0 normally, loads 603 modules, and all source/runtime/baseline/
fixture/human-configuration/producer guards pass. The launcher exits 1 for
the sole reviewed `main error: window closed` notification. No owned player
remains. Saved defaults/restart, added-source and recording/format coverage
remain open; this is not final-package qualification.

## Cold saved-preset and per-file default restoration on r5

`ui_cold_preset_r5` uses the same binaries and a copy of the generated profile
from `beta-single-frame-20261025-night/ui-late-cold-r270`. Its frozen launcher
`run-ui-cold-preset-r5.py` has SHA-256
`04f59b48531c87d11077862a480250c05ea3000c937d1da646a4dc814d15fd11`.
The copied native config remains at `sub-fps=23.976`; unlike the older replay,
this launch has no command-line FPS override. The copied catalog retains the
first file's named 30 FPS preset and its persisted subtitle attachment.

At cold paused source frame 425/17.00 s, the first file shows LATE PRESET CUE
and actual FPS 30. The underlying cue starts at 20 s at normal timing, so this
also supplies rendered-output evidence for restoration before that normal
start. The launcher requested 17.04 s; the observed marker, not the requested
time, establishes the captured frame.

Next starts the second file in ordinary playback; it is then explicitly paused.
At frame 902/36.08 s, its own ATTACHED EPISODE TWO caption is visible and the
settings show Default Preset with FPS 23.976. Previous returns to the first
file, which is explicitly paused again: at frame 618/24.72 s, the named preset,
actual FPS 30 and LATE PRESET CUE are restored. No claim that Next/Previous
preserve pause is made. Observations and their image/marker hashes are under
`ui_cold_preset_r5/observations/`, with analyzer `analyze-cold-r5-observations.py`.

VLC exits 0 with 603 modules. Source/runtime/baseline/fixture/producer guards,
the original copied-from profile guard and human-configuration guards all pass.
The launcher retains its strict failure for the sole reviewed Close notification.
No owned tests remain. This qualifies restoration of existing saved state on
the current candidate, including per-file/global separation; it does not repeat
new preset creation, rejected-write recovery or every default/language/format
combination. Newly added sources and recording coverage remain next.

Source inspection shows that the previous refill used master demux time,
whereas reported playback time subtracts the output's buffered duration.
Negative track delays increase that duration. The new candidate asks the output
for a source-specific refill boundary using the same buffering calculation as
playback reporting, then subtracts any positive delay of the selected source.
Only that subtitle demux is repositioned and flushed; the primary media is
not sought. This remains a candidate until native and ordinary playback tests
qualify it. Active build and run artifacts are ignored under
`build-windows/beta-fps-20261028/`; unchanged earlier runtimes are preserved.

## Attachment and recording follow-up

The r21 `core5_attachment_restore` and `core5_attachment_recovery` runs both
exit 0, with setup/selected-function/cleanup passing and all five preservation
guards true. They cover native paused attachment, duplicate refresh, fresh-input
restoration, missing-source feedback and invalid-subtitle replacement. The held
frame remains 75, clock 3000001, with unchanged audio selection/count. They do
not replace the separate ordinary attachment journey.

The opt-in recording test begins before the first 0-to-50 FPS edit. The r22
three-second capture ended before the fixture's next keyframe at 20 s: its SRT
output had audio/subtitle data but no decodable video. Preserve this failed output
in `core5_record_srt`; a passing API check was insufficient. Test r23 keeps
recording for another ten seconds after the caption check. Its source SHA-256 is
`80838e7b8198e5cd4f92d7b400016e536ed6d08c6e5820e2a10d5e8d7439bf65`,
executable `64846efd9fab959687dc99a6c8571b731900f5ff7d512ef61bca7ba907ad241b`.
`core5_record_srt_long` passes native state/caption/clock assertions and full
output decoding. No test assertion was relaxed to accept an empty video stream.

The longer ASS case, `core5_record_ass_long_before`, fails with an empty Matroska
file: libavformat rejects the legacy SSA codec identifier. The first targeted
mux repair selects `AV_CODEC_ID_ASS` for VLC's SSA/ASS event-packet layout.
`core5_record_ass_av1` then produces decodable AV and ASS text, but an independent
subtitle export finds zero cue durations. That partial pass is preserved. The
second repair passes each positive subtitle block duration to libavformat in
the stream's time base; audio/video packet handling and the shared decoder codec
mapping are unchanged.

The new merged avcodec plugin r2 has SHA-256
`545f715418c4d20659dce5d6c81ee33b292b21c87243fafa62036b9ffd739e1f`;
`modules/demux/avformat/mux.c` has SHA-256
`d1de834e1108e03565b3a347830f30debf9948b64315355ba5ae57e0acef777a`.
Both builds finish 0 under the established lock/two-worker setup, with source
inventories and baseline guards intact. `core5_record_ass_av2` and
`core5_record_srt_av2` use r23, core r5/libass r2/text demux r1 and this plugin.
Both native processes exit 0 with no skips and all five preservation guards true.

| New output | Decoded video | Decoded audio | SHA-256 |
| --- | --- | --- | --- |
| ASS / Matroska | 106 consecutive frames, markers 500–605 | 201728 mono samples, 48 kHz | `31c5f013b6bfd418065a8ee05a4131353e5990bd621a64a8e00426a319636a8e` |
| SRT / MP4 | 106 consecutive frames, markers 500–605 | 203776 mono samples, 48 kHz | `a8d291b10c919a8e3aae2dc0cc8937c26315b3814ca72f7b410437172d3dc9e0` |

`verify-recordings-r6.py` decodes every video/audio packet, requires consecutive
source markers and no decoder errors, and rechecks output hashes. It clarifies
the earlier r5 report's scope: recording stays enabled across the edit, but video
starts at the next keyframe **after** that edit. These outputs do not establish
encoded AV continuity across the edit or exact record-toggle boundaries. Silent
generated audio is not perceptual sync.

`verify-ass-recording-r3.py` checks exported ASS text, styles, resolution, packet
timestamps and durations against the generated cues at 50 FPS. Matroska omits
initial H.264 decode timestamps, so the validator matches the first recorded
video PTS (20 s) to the immutable source keyframe's DTS (19.92 s). The earlier r2
validator's missing-DTS error is not a product failure. MIDDLE is retained for
0.08 s from 19.92 s; LAST starts at 20 s and lasts 9.95 s. The r1 output fails
both duration checks; r2 passes, with the exact two cue texts and original
style/resolution preserved. These are packet/export checks, not ordinary
recorded-file playback or final-package acceptance.

Recorder logs include rejected muxers during its existing format search; the
selected Matroska output now accepts all three streams and writes its header.
The reviewed Close notification remains. All owned jobs are terminal. Next:
ordinary ASS Record-control/output playback, a non-ASS libavformat-mux regression,
remaining formats and W1 defaults/language/role combinations. WebVTT and TTML
rate-control behavior needs explicit qualification; their demuxers do not
implement the text demuxer's rate-control path.

## Ordinary ASS Record and replay on avcodec r2

`ui_ass_record_av2` uses the r7 ordinary GUI, core r5, libass r2, text demux
r1 and avcodec r2 in an isolated profile. The ordinary Playback > Record
action starts recording while paused at source frame 199, then playback runs
at normal speed with saved subtitle FPS 50. The same action stops recording;
primary playback continues. VLC exits 0, all preservation guards pass and 603
modules load. The strict launcher exits 1 for rejected trial muxers and the
reviewed title-bar Close notification; those diagnostics are retained. The
selected Matroska writer accepts all three streams and writes its header.

The 653927-byte recording has SHA-256
`02af134206448f473d37c59de53678b5f06770d57e55674614409894df9c10eb`.
Full decoding passes 1056 consecutive video markers 250–1305 and 2025472 mono
48 kHz audio samples, without decoder errors. ASS packet/export checks retain
the exact original styles/resolution and MIDDLE at 10 s for 10 s, then LAST at
20 s for 9.95 s. FIRST was already consumed before recording began; this does
not assert its retention or exact record-toggle boundaries.

`ui_ass_replay_av2` opens only that recording, with no external subtitle and
normal subtitle timing. Its embedded Track 1 initially is unselected. Selecting
it displays MIDDLE on the unchanged paused frame 250; an ordinary paused seek
shows LAST on frame 591. Playback resumes and LAST has expired by frame 1004.
VLC and launcher both exit 0 with no checked diagnostics and all guards intact.
This qualifies the recorded captions through ordinary playback, separately from
the packet/export validator. It does not qualify perceptual audio sync or the
final package.

The [ordinary evidence](ordinary-recording-followup/evidence.json) and frozen
producers bind these outcomes. Marker analysis v2 excludes three recording-menu
captures where menus cover marker bits, and the initial unsettled replay window.
The earlier ignored v1 analysis remains preserved but is superseded.

The Playback menu still presents the same Record label/icon while recording
and stopped. Exposing its active state remains W3 interaction work. A further
native SRT/FLAC-source run, `core5_record_srt_mkv_av2`, exits 0 with intact guards
and decoded AV, but selects MP4. It therefore does not test the modified
libavformat subtitle-duration branch. An explicitly forced non-ASS Matroska
mux regression and the remaining W1 formats/defaults are still required.

## Forced non-ASS Matroska comparison

The later `forced_srt_matroska_clean_baseline` and `_candidate` explicitly use
the libavformat Matroska writer with generated H.264, FLAC and two SRT tracks.
Both exit 0, load 603 modules, retain all guards and have no logged errors.
The unchanged r7 avcodec plugin loses all six subtitle durations; avcodec r2
retains their exact 0/20/40 s starts and 19.999/19.999/19.9 s durations. Both
candidate SRT exports exactly match the input. Both outputs decode all 1500
consecutive video markers. [Comparison evidence](forced-srt-followup/forced-srt-comparison.json)
includes exact binary/output identities and frozen producers.

An independent audio check does **not** pass input equality: both baseline and
candidate decode the same 2880000 samples, versus 2880528 in the generated input.
Their decoded PCM hashes are identical. This identifies a shared 528-sample
difference, not a regression introduced by the subtitle-duration repair; its
cause (including source padding/demux trimming) remains unqualified. Do not
claim full input audio preservation from the scoped subtitle pass. The initial
strict validator's failed equality assertion is preserved in ignored output.
An earlier launcher supplied a redundant plugin path, loaded 1205 modules and
reported duplicate interface attempts; the clean paired runs remove that
launcher error and retain the original attempt separately.
