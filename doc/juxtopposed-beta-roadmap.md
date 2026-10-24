# Revised beta roadmap

<!-- paused-fps-checkpoint:start -->
## Paused checkpoint - 24 October 2026

Desktop tests are stopped at the owner's request. The [subtitle FPS repair
checkpoint](juxtopposed-references/native/home-subtitle-fps-20261024/README.md) records the current changes and remaining failure.
Paused external-caption FPS changes now refresh the held cue; rate changes no
longer reset the main video/audio clock. Windows accessibility text edits now
commit through the actual settings editor. The ordinary tuple is Coreb62f4145 /
GUI79aef2b5 / MKVf46d8d15 / WASAPI4e4e33c0.

The corrected native before/after comparison uses the same test executable:
old Core fails and all11 functions pass on the new Core. All35 earlier timing,
startup, interval and track regressions pass normally; the offscreen settings
component suite also passes. The2706-input freeze and actual copies verify.
Ordinary desktop edits immediately retime a genuinely selected external SRT,
and native/QML editors exchange fractional values. Both desktop runs exit0;
the new run has no checked diagnostics. Full FPS workflow acceptance is pending.

Next: QML Reset settles at1 instead of0 after opening/closing the older native
Synchronization dialog. Inspect its bounds/model feedback; the cause is not yet
established. Then finish ordinary FPS seek/resume/playing/default/next-media
checks, visual secondary timing and estimators. Earlier embedded-track false
tests and antialias-mask false failures are documented separately and do not
qualify external FPS. Native dummy-window warnings remain explicit.

Continue the complete13-category/32-surface roadmap afterward: remaining
consumers, design/accessibility/OS DPI, consolidated media, physical outputs,
independent review and clean packaging. Windows10 remains separately deferred.
Full beta is not ready. All owned test/build jobs are closed; the goal is paused.
Older status blocks below are historical. No desktop tests may resume while
the owner is using the computer.
<!-- paused-fps-checkpoint:end -->

<!-- subtitle-accessibility-status:start -->
Latest selected repair (23 October): [Subtitle control accessibility](juxtopposed-references/native/home-subtitle-accessibility-20261023/README.md)
is applied and accepted in its stated control/help scope. The shared spinner
exposes Qt's numeric bounds, and all three synchronization controls have names.
Ordinary Windows accessibility writes now update primary timing and the paused
caption: ASS0/+1200/-1200ms, MP4 0/+1200ms. Reset, focused50ms Up and ignored
excessive+10050ms input retain their recorded ASS scope. Saved-delay help now
explains next-media application through the existing authored info affordance.
Its actual editor stays1200 while current timing is50; native sub-delay stays12.

Fifteen component cases pass with no captured messages. Two ordinary desktop
processes exit0 on GUI9dd58c64 / Core7ab80750 / MKVf46d8d15 / WASAPI4e4e. The ASS
run retains endpoint-removal/default-output disappearance and audio errors; it
is NOT a clean playback/audio pass. The subsequent MP4 run resumes with captions
and quits while playing with no checked runtime errors. No device-loss recovery
or audibility claim follows. Previous35 native Core passes retain their earlier
scope and were not repeated for this GUI-only change.

Current runtime: `build-windows/beta-subtitle-accessibility-20261023/ui-frame-marker.mp4-r3/runtime`.
The2407-input freeze reuses2394 actual immutable copies and adds13 changed/helper
copies, avoiding another full C: snapshot. Both runs retain source/runtime/fixture
guards. Exact GUI numeric-position conservation remains unqualified; visible held
frame patterns/coarse elapsed stay fixed while some values reconcile by about1us.

Next: per-track/secondary timing, FPS/speed and estimators. The FPS control's
initialization and secondary-delay notification use differing scale directions;
verify its native contract. Continue all remaining consumers/workflows,32-surface
Figma/design/accessibility/OS-DPI, consolidated media, physical outputs, review
and clean packaging. Full beta remains ACTIVE; Windows10 stays separately deferred.
The prior controlled hover hold passed; original historical hover loss remains
unconfirmed. Older status blocks below are historical. No install, commit/push,
release/upload or shutdown occurred; both owned desktop processes are closed.
<!-- subtitle-accessibility-status:end -->

<!-- paused-subtitle-delay-status:start -->
Latest selected repair (23 October): [Paused subtitle delay](juxtopposed-references/native/home-paused-subtitle-delay-repair-20261023/README.md)
is applied and accepted locally on the ordinary build. Delayed paused seeks
retain the expected cue; current primary delay changes and Reset refresh it at
the held video frame. The regular build uses the exact tested seven-file change,
with no private replacement objects/headers or public ABI change.

All eleven timing cases and 24 earlier startup/interval/track/frame-step/resume
regressions pass on ordinary Core7ab80750. All35 native children exit normally;
121 timing snapshots verify and44 actual seek-frame comparisons match the
private candidate. Positive/negative offsets, time/position seeking, WASAPI and
the no-audio controls retain their recorded scopes. Before failures and the
private candidate remain intact in the separate investigation receipt.

One actual ordinary desktop session covers ASS and mov_text/MP4: delayed Finnish
captions appear after paused seeking; Reset immediately restores the middle cue
without another user seek. Twelve actual keyboard nudges through50–600ms restore
the earlier ASS cue. The next file reapplies the saved1.2-second default after
the previous file's Reset. MP4 Reset, caption-bearing resumed playback and direct
playing Quit pass; the session exits0 without checked GUI diagnostics. The
saved default stays sub-delay=12. Two earlier ordinary preference sessions prove
the actual editors/reopen and explicit-index priority; their broader timing
workflow originally failed and is preserved as such.

Current ordinary runtime:
`build-windows/beta-subtitle-delay-repair-20261023/ui-frame-marker-ass.mkv-r4/runtime`.
Tuple: Core7ab80750 / GUId0ff14c6 / MKVf46d8d15 / WASAPI4e4e. All2,400 actual
input copies and551 immutable repair artifacts verify. This supersedes the
earlier private-only/unapplied instructions. Previous replacement/external
selection/frame-feedback/resume repairs retain their accepted scopes.

Continue with the spinner accessibility value-write route (no change observed;
actual focused Up-key adjustment works), default-delay next-media help,
per-track/secondary timing, wider cue boundaries and lifecycle/stream policies.
Exact GUI numeric progress conservation remains unqualified: ASS briefly shifts
by roughly1us during nudges, and MP4 reconciles its position after Reset while
the visible frame pattern/coarse elapsed stay fixed. Space after popup closure
can activate the focused Subtitle tracks button; arbitrary-focus global Space
is not accepted. Native images independently prove the tested held frames.

The [controlled hover check](juxtopposed-references/native/home-timeline-thumbnails-20261021/current-hover-20261023/README.md)
passed a345.984-second untouched hold on its recorded ordinary tuple; original
historical hover-loss attribution remains unconfirmed. No functional hover
change was needed. Continue the complete thirteen-category/32-surface roadmap:
remaining consumers/workflows, Figma/design, accessibility/OS DPI, consolidated
real media, physical outputs, independent review and clean packaging. Windows10
remains separately deferred. Full beta stays ACTIVE. All owned tests are closed
normally. No install, commit/push, release/upload or shutdown occurred. Older
status blocks below are historical; use this current runtime and scope.
<!-- paused-subtitle-delay-status:end -->

<!-- external-subtitle-selection-status:start -->
Latest selected repair (23 October): [External subtitle selection](juxtopposed-references/native/home-external-subtitle-selection-repair-20261023/README.md)
is accepted locally on the ordinary build. Opening a subtitle while paused
selects the new source's preferred track exclusively and retains the actual
held picture in the tested SubRip/ASS/MP4 and two-track MKS cases. The four
production sources are applied and ordinary Core/header dependents rebuilt;
the earlier private-only plan and runtime directions below are superseded.

The identical maintained worker/fixtures reproduce15 old failures and two
load-only passes, then all17 ordinary passes. All17 earlier startup/interval
regressions pass. All51 native children exit normally;536 paired-selection
snapshots,2,340 actual frozen input copies and948 published artifacts verify.
Three ordinary desktop file-dialog sessions open SRT, ASS and two-track MKS,
show the automatically selected cue and sole checkmark, retain the visible
marker/elapsed text, resume/Pause and close normally without checked GUI errors.
The MKS desktop run also selects external Finnish and restores English.

Current runtime:
`build-windows/beta-external-subtitle-acceptance-20261023/ui-external.srt-r3/runtime`.
Tuple: Coreea14300c / GUId0ff14c6 / MKVf46d8d15 / WASAPI4e4e. Existing exclusive
selection is reused; subtitle identity is set before demux opening, and its
PCR/reset/display-target controls cannot replace the main media timing boundary.
Generic associated sources keep their prior behavior. Earlier paused-replacement,
fade, accessible-progress and Matroska-interval repairs retain their scopes.
The earlier rejected candidate failures remain in their immutable investigation.

Two UI limits remain explicit. A transient Next frame OSD is reproduced during
some ASS/MKS subtitle refreshes with no step key and an unchanged picture;
correct that feedback next. The first accessible SRT seek's normalized slider
value reconciles slightly on refresh: exact GUI numeric-value conservation and
absolute seek PTS are not accepted. Native marker/no-audio-time checks provide
the quantitative conservation proof. Expected native missing-file/dummy-window
diagnostics and compiler warnings remain recorded.

Then broaden playing/pending/nonseekable/recording, repeated addition/removal/
restart and multi-selection policies, cold initial cues and language/default/
sync flows. Original preview hover loss remains unconfirmed; a latest-runtime
controlled check is still due. The caption-clearance preference remains saved.
The full13-category/32-surface beta stays ACTIVE, including remaining consumers,
design/accessibility/DPI, consolidated real-media, physical-output, independent
review and clean-package gates. Windows10 comparison remains separately deferred.
No install, commit/push, release/upload or shutdown occurred. All owned tests
are closed normally. These are local repair results, not full-beta acceptance.
<!-- external-subtitle-selection-status:end -->

<!-- subtitle-replacement-repair-status:start -->
Latest selected repair (23 October): [Paused subtitle replacement](juxtopposed-references/native/home-paused-subtitle-replacement-repair-20261023/README.md)
is accepted locally on the ordinary build. Embedded English/Finnish changes and
explicit external-subtitle selection while paused now show the selected cue and
retain the held picture in SubRip/Matroska, ASS/Matroska and mov_text/MP4 tests,
with WASAPI and without audio. The earlier player-only candidate failed MP4
frame conservation; its failure is preserved, and the decoder now retains the
exact paused picture boundary after packet preroll.

The maintained portable regression has six unchanged failures, six combined
candidate passes and six ordinary passes. All17 earlier startup/subtitle-interval
regressions pass. Evidence preserves40 normal native exits,345 replacement
snapshots and three normal desktop exits with no checked GUI errors. Actual
desktop language changes hold the visible marker and timeline; SubRip also
checks English restoration, explicit external loading and resume/Pause.

Current runtime:
`build-windows/beta-subtitle-replacement-repair-20261023/ui-frame-marker.mkv-r3/runtime`.
Tuple: GUId0ff14c6 / Coreef3e47fd / MKVf46d8d15 / WASAPI4e4e. This checkpoint
supersedes the older active-runtime and candidate-only directions below. The
ordinary build compiles matching player/input, decoder and ES-output sources;
no private replacement objects or candidate logging are used. The2,290 input
copies and715 published artifacts verify. Native diagnostics/compiler warnings
remain recorded. The earlier fade, accessible-progress and Matroska-interval
repairs retain their limited accepted scopes.

Next: automatic external selection (the file loads, but the previous track stays
selected until an explicit choice), cold initial cue delivery, language/default
and synchronization flows. Broaden recording/nonseekable/pending-seek and
previous-frame coverage. Frame conservation does not establish absolute seek
PTS; WASAPI public paused time remains a separate issue. Desktop keyboard/first
Play activation also remains unqualified. Complete caption layout and the saved
morning caption-clearance preference remain open.

The full13-category/32-surface beta remains ACTIVE: continue remaining feature,
design/accessibility/DPI, physical-output, Windows10, independent review and
clean-package gates. Original preview hover loss is still unconfirmed; its
earlier controlled hold passed, with another latest-runtime check still due.
Save nonurgent questions for morning. No install, commit/push, release/upload
or shutdown occurred. All owned test windows and native jobs are closed.
<!-- subtitle-replacement-repair-status:end -->

<!-- paused-subtitle-replacement-status:start -->
Latest investigation (23 October): [Paused subtitle replacement](juxtopposed-references/native/home-paused-subtitle-replacement-investigation-20261023-r6/README.md).
The accepted build restores captions after backward seek, but changing an
embedded subtitle or explicitly selecting an external subtitle while paused
clears the current cue until resume. This reproduces in original/remuxed and
colored video with WASAPI and without audio.

A separate Core candidate restores all 20 selected-caption observations across
five paired cases. It is REJECTED for application: both no-audio cases advance
the paused video-source timer point by 40ms per selection refresh, one frame in
the 25fps fixtures. The unchanged controls retain that point. WASAPI's stale
best-source time cannot qualify exact video-frame conservation. Do not promote
candidate Core5837a5b7 or treat caption presence alone as workflow acceptance.

The queued-SPU-only candidate made no difference in three runs. A raw-timer
candidate was corrected before execution. The failed conservation preflight is
retained. The canonical review corrects an overbroad summary in the first
receipt without changing its frozen artifacts. Evidence preserves 21 normal
native exits and 188 snapshots; native diagnostics remain visible.

No production source was changed by this investigation. Current accepted
runtime stays `build-windows/beta-caption-layout-20261023/ui-after-r1/runtime`,
GUId0ff14c6 / Core000f1159 / MKVf46d8d15 / WASAPI4e4e. The caption fade,
accessible progress and precise subtitle interval repairs retain their scopes.

Next: trace the displayed-video/SMPTE source and seek normalization, or refill
only subtitle data. Require repeated selection to conserve the paused picture
and time, then maintained native regression, ordinary Core and desktop checks.
Cold initial cues, automatic external selection, language/default/sync flows
remain open. The caption-clearance preference is saved for morning; no new
question is required. Full 13-category / 32-surface beta remains ACTIVE, with
design/accessibility/DPI, physical outputs, Windows10, independent review and
packaging still open. No install, commit/push, release/upload or shutdown.
All owned native test processes exited normally.
<!-- paused-subtitle-replacement-status:end -->

<!-- caption-bottom-fade-status:start -->
Latest selected repair (23 October): [Caption bottom fade repair](juxtopposed-references/native/home-caption-bottom-fade-repair-20261023/README.md)
is accepted locally. The redesigned player no longer draws the old bottom
fade above its opaque control bar. The unchanged build visibly darkens the
lower overlapping caption; the ordinary candidate keeps SubRip and ASS lines
bright in fullscreen. Legacy controls retain their existing fade. Backward
SubRip seeking, paused subtitle-menu open/Escape and resume/Pause work.

Four ordinary GUI sessions exit 0 with no checked runtime errors or source,
runtime or fixture drift. These are reviewed desktop observations, not image
metrics or exact frame PTS. The new immutable receipt verifies 44 artifacts.
The before/after freezes contain 2,242 actual input copies each; only Player.qml
differs. The ordinary build regenerates Player QML/resource and relinks without
private test objects; existing static-library linker warnings remain recorded.

Current runtime: `build-windows/beta-caption-layout-20261023/ui-after-r1/runtime`.
Tuple: GUId0ff14c6 / Core000f1159 / MKVf46d8d15 / WASAPI4e4e. This supersedes
older active GUI/runtime directions below. Earlier precise subtitle interval
and accessible progress repairs retain their accepted, limited scopes.

Read-only Figma context and screenshots for Player247:445 and hidden-controls
3328:11554 confirm video behind floating controls. The placeholder image's
darker floor also exists with controls hidden; it is not evidence for an extra
UI fade. No video resize, subtitle-position, saved-offset or top-fade change.

One nonurgent design question is saved for morning: should captions temporarily
move above visible controls, or keep their original position until controls
hide? The original design does not specify this. Small-window caption coverage
remains OPEN; preserve user offsets and authored ASS positions when deciding.
See the receipt's morning-question.md. No answer is needed for independent work.

Next: initial/new/external subtitle tracks, language/default and synchronization
flows. Full caption layout, accessibility/DPI, physical outputs, Windows10,
independent review and packaging remain open under the full 13-category /
32-surface beta goal, which stays ACTIVE. Save questions for morning. No install,
commit/push, release/upload or shutdown occurred; all owned VLC tests are closed.
<!-- caption-bottom-fade-status:end -->

<!-- progress-accessibility-status:start -->
Latest selected repair (23 October): [Progress accessibility seeking](juxtopposed-references/native/home-progress-accessibility-repair-20261023/README.md)
is accepted locally. Qt's value interface writes the QML value property; the
old progress bar did not commit that write to VLC. SliderBar now guards
internal engine/drag display writes and routes an external accessible value
through its existing released-state seek function once, with normalized bounds.
The pointer/chapter state machine and native seek-speed policy remain.

The healthy unchanged build reproduces no native seek from its accessible
value interface. The candidate passes 39 actual-player checks: one request per
changed value/pointer action, identical-value suppression, frozen pause,
resume and no feedback seeks from ordinary playback updates. The initial
combined cold-pause readiness failure is retained as test setup, not as an
accessibility negative. The corrected setup plays until picture/statistics
publication, then pauses. The private QtTest fixture is distributed for future
regression use and stays out of the ordinary GUI DLL.

Ordinary desktop value-setting shows blue at 25s, red at 5s and green at 45s/15s
while paused. Pointer/global arrow seeking and resume/Pause/Quit 0 work. The
final paused position remains exactly 0.46468585994901784 across observations.
These are coarse content observations, not exact frame PTS. Arrow input used
the observed ten-second global route; UIA still reported title text as focused.
Do not infer slider-specific focus, short-step behavior or screen-reader speech.

Current runtime: `build-windows/beta-caption-accessibility-20261023/ordinary-ui-r4/runtime`.
Tuple: GUId368d5ec / Core000f1159 / MKVf46d8d15 / WASAPI4e4e. The final freeze
has 2,232 actual inputs in source-ordinary-r3, not 2,232 compiled units. This
supersedes older active GUI/runtime directions below. The receipt verifies
67 artifacts, three normal private jobs and one normal ordinary GUI exit.
The earlier precise Matroska repair remains accepted in its recorded scope.

Next: caption clearance above visible controls and redundant bottom shading.
Player.qml extends the unpinned video behind controls. The redesigned bar has
its own background while the old outer fade can still darken captions.
Preserve pin-controls policy and saved subtitle offsets; libass supplies
absolute regions even for normal ASS, so a relative-bottom adjustment alone
cannot qualify all positioned captions. Check Figma/video and the runtime
safe-area path before changing that behavior. Complete accessibility/DPI,
initial/new/external subtitle tracks, language/default/synchronization flows,
physical outputs, Windows 10, independent review and packaging remain open.
The full 13-category/32-surface goal stays ACTIVE. No morning question requires
input yet. No install, commit/push, release/upload or shutdown occurred; all
owned test processes closed normally.
<!-- progress-accessibility-status:end -->

<!-- paused-subtitle-seek-status:start -->
Latest selected repair (23 October): [Paused subtitle interval seeking](juxtopposed-references/native/home-paused-subtitle-interval-repair-20261023/README.md)
is accepted locally for precise Matroska seeks. A contiguous subtitle-span
index retains older cues still active beneath newer overlaps; header-only
indexing avoids loading compressed AV payloads. AV anchors/preroll and the
existing fast-seek policy remain. Four production Matroska files are updated.

Eight ordinary caption scenarios pass; eight unfixed ordinary controls fail.
Coverage includes cold English/Finnish, delayed-first cues, empty gaps, long
overlapping SubRip/ASS cues, backward/repeated seeks, frozen pause and resume.
Independent normal-playback caption masks are compared where configured. All
16 C++ units were rebuilt together with the configured ordinary rules and
matching headers; two unchanged C objects remain shared. Build/dependency and
probe-preparation failures are retained with their corrections.

Current runtime: `build-windows/beta-tracks-subtitles-20261023/ordinary-ui-r37/runtime`.
Tuple: Core000f1159 / GUI716862a1 / MKVf46d8d15 / WASAPI4e4e. The final freeze
has 2,210 actual input copies in source-r35c. This supersedes older Matroska
runtime and next-step directions below; previous receipts remain immutable.
The new receipt verifies 1,068 artifacts, 399 qualified native pictures,
64 normal native jobs and one ordinary GUI exit.

Header indexing reduces first-seek process reads on the 30.66 MB gap fixture
from 27.40 MiB to 2.56 MiB. Long active cues still require replay from their
start. These counters include probe/cache work; settle-plus-snapshot timings
are not exact seek latency or physical-storage measurements. Pending-seek Stop
passes in two private and one ordinary run. Two earlier file-log attempts
missed the index window and remain inconclusive. Disabled-subtitle and fast
seek controls pass; missing old captions under fast seek are preserved legacy
behavior, not complete caption acceptance.

The visible ordinary fullscreen player shows both overlapping captions,
retains the older long cue after short cues end and on backward seek, keeps
the exact paused position through subtitle-menu open/Escape, resumes/Pause and
quits 0. Complete desktop subtitle acceptance is still OPEN: the 960x540 window
covers its lower caption with the playback bar, and fullscreen's bottom gradient
darkens that line. Direct accessibility progress value-setting did not commit
a seek, although physical progress-bar input worked.

Next: repair caption clearance above visible controls and accessible progress
seek commitment; recheck windowed/fullscreen and keyboard behavior. Initial
subtitles, paused track replacement, associated/external subtitles, saved
language/default policy and synchronization edits remain open, followed by the
remaining design/accessibility/DPI, physical-output, Windows 10, independent
review and packaging gates. The full 13-category/32-surface goal stays ACTIVE.
No question requires morning input. No install, commit/push, release/upload or
shutdown occurred; all owned test processes closed normally.
<!-- paused-subtitle-seek-status:end -->

<!-- paused-tracks-investigation-status:start -->
Latest selected repair (23 October): [Paused video startup](juxtopposed-references/native/home-paused-video-startup-repair-20261023/README.md)
now obtains the first video picture while keeping playback paused, even when
audio fills the initial buffer before video packets arrive. It uses the
decoder's first-picture state independently of user frame-step counts. No audio
clock, subtitle demux/selection or GUI change is part of this repair.

Nine ordinary native startup controls pass, including WASAPI, dummy/no audio,
disabled subtitles, plain and remuxed video, audio-only, a single-picture file,
and delayed video. Four original video failures remain recorded. The corrected
paired probe checks actual selected tracks: an empty vout resource does not
prove that an audio-only file has video. Existing audio7, bookmark/chapter9 and
paused video-picture6 replays pass on the new Core. The ordinary visible app
shows its video surface at cold paused startup, keeps the exact slider value
through track-menu open/Escape, resumes/Pause and quits0. Subtitle text remains
absent in those desktop observations; no full track-workflow acceptance.

The timestamp-preserving remux retains all1,500 video and2,814 selected-audio
packet payloads/PTS and starts paused correctly. This refines the [earlier
12-run investigation](juxtopposed-references/native/home-paused-tracks-investigation-20261023/README.md): the21ms shift is not a sufficient explanation.
A separate private Core trace finds buffering ends with audio queued and no
video packet received in the original container; the matching remux receives
video before pause. The private trace DLL is not the acceptance build.

Current ordinary tuple: GUI716862a1 / Core000f1159 / WASAPI4e4e. The runtime is
`build-windows/beta-tracks-subtitles-20261023/ordinary-ui-r14/runtime`;2,147
frozen inputs have actual copies in source-r14. Earlier receipts keep their
own binary scopes; this tuple supersedes old active-Core instructions below.
The new receipt verifies461 artifacts and retains45 normal investigation
processes. R10's counter candidate, R11's uncompiled malformed generated source,
and R12's incorrect audio-only test expectation stay recorded with corrections.
The original buffer warning and native teardown/build/parser/cache warnings
remain visible; no error-free-log or all-format claim.

Next: trace Matroska subtitle seek points before video preroll and paused
selected-track/associated-subtitle delivery. Native backward-seek and external
selection cue gaps remain; post-start native pixel-presence patterns match the
earlier controls. Saved language/track defaults, synchronization value changes,
ordinary remaining workflows, Figma/design, accessibility/DPI, physical outputs,
Windows10, independent review and packaging stay open under the full13-category/
32-surface beta goal. Save questions for morning; none is required now. No
installs, commit/push, release/upload or shutdown occurred.
<!-- paused-tracks-investigation-status:end -->

<!-- tracks-popup-input-status:start -->
Earlier selected [track-menu input repair](juxtopposed-references/native/home-tracks-popup-input-20261023/README.md)
passes complete-menu component4 and actual native navigation6 checks, with
original and intermediate failures retained. On its GUI716862a1/Core4422 tuple,
outside dismissal preserves paused position; Escape routes from lists and
synchronization editors; focus returns to the originating audio/subtitle button
so Space reopens that menu. R3 separately qualifies subtitle Back; no exact
whole-workflow or visual-design acceptance is claimed.

The new [startup repair](juxtopposed-references/native/home-paused-video-startup-repair-20261023/README.md) uses the same GUI and current Core000f1159;
ordinary subtitle-menu open/Escape again retains paused startup position.
The original placeholder finding is repaired in that selected startup scope.
Paused subtitle cues, new-track/default-language policy, actual synchronization
edits and complete track workflows remain open. The earlier track-menu receipt
stays immutable, including its old startup error and separate binary scope.
<!-- tracks-popup-input-status:end -->

<!-- paused-audio-seek-status:start -->
Earlier completed repair (23 October): [Initial paused audio seeking](juxtopposed-references/native/home-paused-audio-repair-20261023/README.md)
now keeps elapsed time and slider consistent before first play. Core pending
seek metadata uses current input duration/live status, with unavailable old
position cleared. Output clocks and Qt controller/helper/UI policy are unchanged.
The unchanged real-controller executable passes7 with Core4422fbb0; its old-Core
control fails3. Exact-function policy15 passes; original source fails10.
Ordinary GUI21d55d1c verifies pointer15s/.5 and Go to Time9s/.3 before first play,
then normal resume/Pause/Quit. Native bookmark/chapter replay9 and six paused-video
picture checks pass. Historical teardown messages and parser/build warnings are
retained. Source-r23 freezes2,120 inputs with explicit Core/native/preflight scopes.

This closes the selected paused-WAV seek defect. Live-policy supplied fields do
not qualify a real service; the legacy time dialog is functional evidence only.
Full13-category/32-surface consumers/workflows, design/accessibility/DPI, physical
outputs, Windows10, independent review and packaging remain open. That checkpoint used Core4422fbb0; current work uses Core000f1159 above. No dependencies, commit/push/release/upload or shutdown.
<!-- paused-audio-seek-status:end -->

<!-- queue-addition-status:start -->
Earlier completed workflow checkpoint (23 October): [Queue addition](juxtopposed-references/native/home-queue-addition-20261023/README.md)
qualifies Cancel, one-file/batch append, and addition under an active Birch
filter. Cedar remains paused at13s with the exact same slider value. Clearing
the filter reveals all six rows in the expected visible order. Ordinary GUI
f528dd1c quits normally without captured errors/Qt exceptions or identity drift;
the existing2,103-input source freeze and separate new producer are recorded.

The filter overlaps the Add button, and seeking during initial paused startup
did not update elapsed time until brief playback. These remain separate findings;
the addition baseline was established afterwards. The [filter layout repair](juxtopposed-references/native/home-queue-filter-layout-20261023/README.md)
passes six paired component cases and the existing panel geometry/assets suite.
Ordinary GUI21d55d1c opens the actual picker from Add's center under an active
filter, appends Fir, retains filtering and restores row placement on Escape.
Normal Quit0 has no captured runtime errors/Qt exceptions or identity drift;
source-r19 freezes2,108 inputs. Original failures/font setup warning are retained.
Initial paused audio seeking remains open and reproduces without a start-time
option. The [native investigation](juxtopposed-references/native/home-paused-audio-seek-20261023/README.md)
records eight normally closed cases with stale paused output time; all four
actual WASAPI cases resume near the requested9s target. Dummy samples differ
and remain recorded. This open issue needs actual timer/controller callback
tracing before a repair; it is not a passing seek regression. Queue removal/physical dragging,
touch, full design/accessibility/DPI/hotplug, Windows10, remaining consumers,
review and packaging remain open under the full13-category/32-surface goal.
No dependencies, commit/push/release/upload or shutdown occurred.
<!-- queue-addition-status:end -->

<!-- queue-bookmark-chapter-status:start -->
Earlier workflow checkpoint (23 October): [Bookmarks and chapters](juxtopposed-references/native/home-queue-bookmarks-chapters-20261023/README.md)
qualifies corrected chapter offsets, pointer activation, mouse-selected Enter
activation, bookmark creation/seeking, media-specific empty/restore states and
cold persistence in selected ordinary Windows 11 journeys. Native model 9 and
exact pointer-subtree 6 checks pass; original failures and intermediate Enter
failure remain recorded. Two historical headless close-notify messages are
retained and classified at native teardown; no other error diagnostics.

Final GUI f528dd1c verifies chapter times 0/6/12 and actual paused seeking,
creates the new 12s bookmark and restores/activates it after a cold restart.
Both final processes quit normally without captured runtime errors or Qt
exceptions. Read-only isolated database verification confirms 0/6000/12000ms.
Source-r17 freezes 2,103 GUI/producer inputs; native test copies have a separate
record. Only the existing navigation scope was added after R16; Queue0 and
legacy key policy are unchanged.

Ordinary UI removal/rename, queue addition/removal/physical dragging, touch,
full design/accessibility/DPI, display hotplug, Windows 10, remaining consumers,
review and packaging remain open under the full 13-category/32-surface goal.
Nonurgent questions are saved for morning. No dependencies, commit, push,
release, upload or shutdown occurred.
<!-- queue-bookmark-chapter-status:end -->

<!-- queue-visibility-status:start -->
Earlier workflow checkpoint (23 October): [Live Queue visibility](juxtopposed-references/native/home-queue-visibility-20261023/README.md)
retains a visible queue across dock/undock in selected paused audio and embedded
video journeys. The live transition follows requested visibility without
reapplying startup policy. The real Qt fixture runs the complete production FSM
and exact signal connections: 20 checks pass; original source fails three
redock cases. Two ordinary GUI bd8f6cf7 processes retain all five rows and Cedar
paused at 00:00, then exit normally without captured errors/Qt exceptions. A
hidden audio queue stays hidden when undocked. Video acceptance uses a settled
observation after the animation; existing parser/window-size warnings are recorded.

Source-r14 freezes 2,090 actual inputs. Earlier placement/lifetime and Find
receipts retain their separate scopes. Physical dragging, add/remove, real
bookmarks/chapters, full design/accessibility/DPI, display hotplug, Windows 10,
remaining consumers, review and packaging remain open under the full
13-category/32-surface beta goal. Next: isolated real bookmark/chapter journeys.
The physical hand-drag question is saved for morning. No dependencies, commit,
push, release, upload or shutdown occurred.
<!-- queue-visibility-status:end -->

<!-- queue-detached-status:start -->
Earlier workflow checkpoint (23 October): [Detached Queue](juxtopposed-references/native/home-queue-detached-20261023/README.md)
stays reachable beside a maximized player and retains its complete native title
bar after Close/reopen. Placement policy15 and exact loader-binding18 checks
pass; original bindings fail5 expected cases. Final ordinary GUI2aef3286 verifies
first/reopened frame, detached Find/filter/Escape, return to docked mode and
normal Quit0 with no runtime diagnostics. Cedar stays paused at0; all5 rows
remain. Source-r13 preserves2,086 actual input files. Earlier off-screen,
reopen-frame and teardown-warning failures remain recorded.

This R13 run explicitly reopens the queue after redocking; the later R14
checkpoint above resolves live visibility retention. Physical dragging, add/remove,
bookmark/chapter flows, whole design/accessibility/DPI, display hotplug and final
packaging still need acceptance under the full13-category/32-surface goal.
The slow hand-drag question is saved for morning. No commit/push/release/upload
or shutdown was performed; no new dependencies were installed.
<!-- queue-detached-status:end -->

<!-- queue-find-status:start -->
Earlier workflow follow-up (23 October): [Queue Find](juxtopposed-references/native/home-queue-find-20261023/README.md)
now opens its filter from a mouse-focused queue row without also opening the
global folder dialog. The real QApplication/production-filter regression passes
8 cases; its original-filter control fails precisely2 expected cases. Native
hotkey routing passes17, and unchanged QML retains its proxy/geometry/overlay
component receipts. The ordinary app verifies Find, Birch filtering, Escape,
Title sorting, hide/show, retained global Ctrl+F from Bookmarks and normal Quit.
Cedar remains paused at0 throughout. GUI903dc977 / Core0404d1f6 / WASAPI4e4e33c0;
2,072 actual source files are frozen, with only a test-counter delta after GUI build.

The complete Queue workflow remains open: two ordinary drag attempts did not
reorder an item. A later passive private trace shows an automated17ms gesture
ending before asynchronous native drag startup; a slower physical check is
saved for morning. No production drag code changed. Native dragging, add/remove,
detached queue, actual bookmark/chapter workflows and physical DPI/accessibility
still need acceptance. Fresh Queue463:831 Figma context and a complete screenshot
are saved; actual panel backdrop blur remains missing. Full13-category/32-surface
beta, remaining consumers, review and packaging remain active. No commit/push,
release, upload or shutdown was performed.
<!-- queue-find-status:end -->

<!-- renderer-candidate-status:start -->
Current checkpoint (22 October): controlled hover remains stable; the original
loss is unconfirmed and may have followed the user's mouse movement. The
[private hover observation](juxtopposed-references/native/home-timeline-thumbnails-20261021/hover-r66/checkpoint.json)
holds eligibility for 315.271 seconds and responds correctly to deliberate
pointer exit/reentry. The earlier [ordinary R67 resource pair](juxtopposed-references/native/home-timeline-thumbnails-20261021/resource-r67/checkpoint.json)
retains native hover/window eligibility in both measured preview intervals,
with actual endpoint pictures, normal pause/resume/Quit and no newly late/lost
frames or lost audio buffers. Continuous pixel recording is not claimed.

The memory investigation found repeated accessibility activation accumulating
Qt scrollbar action connections. [Matching private factor controls](juxtopposed-references/native/home-timeline-thumbnails-20261021/memory-r90/checkpoint.json)
narrow it to activation notifications while all accessibility requests still
forward. A narrow contrib patch adds UniqueConnection to the two action connects;
accessibility remains enabled. The [native before/after regression](juxtopposed-references/native/home-timeline-thumbnails-20261021/scrollbar-r93/checkpoint.json)
reproduces two duplicate-action failures on the original Qt and passes all seven
action/deactivation/reactivation checks with the patch. The fresh ordinary GUI
b5eaf01a, with no private hooks, [holds idle memory](juxtopposed-references/native/home-timeline-thumbnails-20261021/memory-r94/checkpoint.json)
at 164–166 MB for 180 seconds, exposes the real Home accessibility tree and quits
normally. [Native tests](juxtopposed-references/native/home-timeline-thumbnails-20261021/native-r96/checkpoint.json)
pass 26/0 skips on that same repaired GUI/Core/MP4 tuple. These new receipts bind
1,799 source files, including the contrib patch/rules and maintained native test.
Broader screen-reader and long-run allocation acceptance remains open.

The scoped Qt 6.8.3 DirectComposition/D3D11 frame-wait default, explicit overrides,
cached-backend ordering and native paint-host repairs remain candidates. Their
historical [R63 renderer matrix](juxtopposed-references/native/home-timeline-thumbnails-20261021/renderer-r63/checkpoint.json)
passes six private journeys/832 checks/38 cards on its frozen source. The current
[ordinary resource pair](juxtopposed-references/native/home-timeline-thumbnails-20261021/resource-r95/checkpoint.json)
now passes selected memory/throughput checks after the repair: both120s held
previews retain native hover eligibility and actual endpoint pictures, private
memory stays near293/296 MB, and playback has no newly late/lost frames or lost
audio buffers. Both resume and quit normally. Selected CPU means are reported;
universal resource-cost/promotion gates remain OPEN. Whole-board GPU power is
not process-attributable. The fresh [Figma card reference](juxtopposed-references/figma/20261022/timeline-thumbnail/README.md)
requires actual backdrop blur, which is still unimplemented. Legacy compositor
primary composition, layout/DPI/accessibility, remaining consumers/workflows,
independent review and packaging remain open under the full thirteen-category/
32-surface beta roadmap. Existing package and releases are unchanged; full beta
acceptance is not claimed. Earlier entries below retain their dated source scopes.
<!-- renderer-candidate-status:end -->

<!-- audio-media-role-status:start -->
Current feature follow-up (22 October): [Windows Media role](juxtopposed-references/native/home-audio-media-role-20261022/README.md)
now classifies new WASAPI streams without changing the selected endpoint or
interrupting a paused owner. Native Windows startup passes14 cases across20
real output sessions; COM contract15, Audio defaults35 and profile15 all pass
with normal exits and zero skips or unexpected diagnostics. All nine roles are
accepted by Windows; save failure, rollback/retry and cold restoration pass.
Four ordinary app processes cover pointer/keyboard selection, native readback,
help/restart feedback, cold Communication, the disabled non-WASAPI backend and
actual Communication playback/pause/Quit. GUI33f10d62 / WASAPI4e4e33c0 / Core0404d1f6;
2,064 final source copies are frozen, with production identical to ordinary R4.
The old-provider negative and earlier failed log observer remain preserved.

Four Audio controls remain gated: passthrough, Spatial audio and its mode, and
adaptive visualization size. Fresh Figma row context and original SVG identity
are saved, but its clipped1x1 screenshot does not establish pixel fidelity.
Windows10 comparison, physical audio/receiver/routing, full design/DPI/screen-reader,
actual thumbnail backdrop blur, other confirmed consumers/workflows, independent
review and packaging remain open under the full13-category /32-surface goal.
No commit/push/release/upload/shutdown occurred. No new dependencies were installed.
<!-- audio-media-role-status:end -->

<!-- audio-night-mode-status:start -->
Earlier feature follow-up (22 October): [Night mode](juxtopposed-references/native/home-audio-night-mode-20261022/README.md)
now has an independent native preset and an enabled Audio Defaults consumer.
The maintained audio suite passes 35 cases, and profile defaults pass 15, with
no skips or unexpected diagnostics. Actual Off/On/Off PCM reduces settled loud
sections by 12.266 dB while retaining quiet levels; paused-owner edits apply only
to the saved default. Compressor parameters and its switch remain independent;
rollback/retry and cold restoration pass. Three ordinary UI processes cover
mouse/Left/Right, saved readback, cold On, compact/full-width layout and help.
Final GUI 9069f43a / Core 0404d1f6; 2,051 R5 source inputs frozen. These are scoped
consumer receipts; they do not replace earlier memory/performance source scopes
or accept every audio format/device, screen-reader/DPI state or final package.
At that earlier checkpoint five Audio controls remained gated; the newer
Media role receipt above supersedes that count. Actual thumbnail backdrop blur, other confirmed
consumers/workflows, broad design/accessibility, final review and packaging
remain open under the full 13-category / 32-surface goal. No commit/push/release
was performed. Code Connect setup was approved but left unwritten because the
account is Professional and the installed workflow requires Organization/Enterprise.
<!-- audio-night-mode-status:end -->


21 October selected codec follow-up is now saved: [native formats](juxtopposed-references/native/home-timeline-thumbnails-20261021/formats-r53/checkpoint.json)
passes26/0skips, including18 independently referenced video pictures and FLAC
fallback; [ordinary HEVC/WebM](juxtopposed-references/native/home-timeline-thumbnails-20261021/ordinary-formats-r54/checkpoint.json)
checks show a green8s card over a red2s paused primary, resumed advancement to6s
and normal playing Quit in both processes. Ordinary decoding uses D3D11VA on
the home GPU. No Qt warnings/native errors; native padding, initial-size fallback
and AAC zero-sample warnings are retained with a separate review. The maintained
`verify_timeline_thumbnail_media.py` verifies1796 current sources/82 artifacts.
Power attribution, production latency solution, real blur and broader layout,
DPI/accessibility/review/package gates remain open. No renderer default changed.

21 October current follow-up: matched ordinary R52 workaround/control both
pass playing/paused preview, resumed motion and normal Quit with no diagnostics.
Playing-preview whole-board power averages45.6/44.8W. Both configurations have
short power/memory-clock excursions; paused-preview averages52.9/48.8W include
such excursions. The earlier sustained97W reading is preserved and remains
unattributed. Resource cost and production promotion stay OPEN; no renderer
default/package changed. The [power receipt](juxtopposed-references/native/home-timeline-thumbnails-20261021/power-r52/checkpoint.json)
binds1795 sources/28 artifacts, with that source state frozen at
`build-windows/beta-timeline-thumbnails-20261021/source-ordinary-r52`.
The new R53 native suite passes26/0skips, adding six video codec/container cases
with2/8/14s FFmpeg reference checks and an audio-only fallback. The HEVC MKV has
embedded subtitles/two audio tracks. Primary pause/time remains independent,
then resumes/advances/stops. This is provider scope; ordinary format/UI, blur,
physical DPI/accessibility, review/package and full beta gates remain open.

21 October later preview validation: the independent startup-pause deadlock is
repaired in Core `0404d1f6cd89…`, with ordinary GUI `e8df140c4ecf…`/MP4 unchanged.
An inherited playback pause could strand the thumbnail decoder and block Quit.
The old-Core paused control reproduces that cycle; the same native exe/media
with new Core passes, and the full suite passes19/0skips. Real ordinary40s picture
previews, independent continuing playback and normal playing/EOF Quit are
qualified in the saved scopes. [Startup/ordinary evidence](juxtopposed-references/native/home-timeline-thumbnails-20261021/startup-r50/checkpoint.json)
binds1795 current sources/65 artifacts, retains failed setups and the actual
negative, and preserves the EOF run's original checker failure separately from
its verified normal exit. CPU/memory/native frame/audio counters look healthy,
but the frame-wait workaround's resource-cost gate remains OPEN: its preview
phase reads~97W whole-board GPU power versus44–46W control. Repeat matched trials
with memory/core clocks and outside-process engine activity to establish cause
before promotion. No frame-wait default/package change. Then continue the blur,
broader media/layout/DPI/accessibility, review and packaging gates below.

21 October preview responsiveness investigation: private timing probes isolate
the observed one-second pause to main-window drawing synchronization, including
cached previews. Changing the time-tooltip property to inline did not remove it.
A paired run on the same R42 binary/media with renderer logging and process-local
`QT_D3D_MAX_FRAME_LATENCY=0` changes16 waits>=900ms to zero (maximum146ms including
startup); cached preview readiness stays<=27ms in that run. All five diagnostic
processes retain the existing predicates and normal Quit, with production code
and shipping defaults unchanged. The [timing evidence](juxtopposed-references/native/home-timeline-thumbnails-20261021/timing-r44/checkpoint.json)
binds1794 current sources/21 artifacts, including complete compressed logs.
This is a promising workaround, not closed responsiveness acceptance. Next
qualify a properly scoped solution with ordinary moving playback, CPU/GPU/power,
outputs/instances, other renderers and supported Windows versions. Then continue
blur, broader media/layout/DPI/accessibility, review and packaging gates below.

21 October latest visible preview qualification:693 private actual-app checks,
29 rendered cards and ordinary compact Light125% click/drag/Quit pass on Core
`db78919ef2e3…`, ordinary GUI `e8df140c4ecf…`, private GUI `1bd2c7ab347f…`, MP4
`917af1d1a006…`. A first cold right-edge click placed the card at x0 instead of
260; nonfocused inline Popup fixes it, with the actual before-repair failure
retained. Six geometry sources, six size/theme/scale variants, precise/fast held
drag, fullscreen/Escape, actual toggle Off, audio fallback, save denial/retry and
resumed decode/Quit pass. Geometry clips now run18s for the unchanged resume
assertion; reference pixels remain byte-identical. The
[visible receipt](juxtopposed-references/native/home-timeline-thumbnails-20261021/visible-r38/checkpoint.json)
verifies1794 sources/47 artifacts. Native17 geometry/cancellation cases retain
their separate paired-Core proof. Responsiveness remains open (some captured
samples1.18-1.26s), as do real Figma blur, broader/live/network/HDR media, other
layouts, physical DPI/accessibility, independent review and final packaging.
This does not complete the thirteen-category/32-surface roadmap.

21 October earlier native preview geometry: R30 passes17 cases, zero skipped,
including portrait, SAR2:1 and genuine90/180/270-degree media. Correct orientation,
square-pixel shape and center crop match independent FFmpeg references. Paired
old/new Core runs use the same test exe/GUI/MP4 and fixtures: old Core fails3
rotations, repaired Core passes17. The raw thumbnail callback now preserves the
decoder's orientation; generic allocation and normal playback callbacks stay
unchanged. New Core `db78919ef2e3…`; GUI `20ece22dc225…`/MP4 `917af1d1a006…`.
[Geometry receipt](juxtopposed-references/native/home-timeline-thumbnails-20261021/geometry-r30/checkpoint.json)
verifies1793 sources/33 artifacts and retains rejected metadata-free fixtures and
earlier failures. This closes these native geometry cases, not visible new-Core,
all-format or full beta acceptance. R22/R23 proofs retain their historical Core;
next verify the repaired Core visibly, then latency/blur, remaining media/layout/
physical DPI, independent review and final packaging.

21 October later timeline-preview proof: R22 passes262 private actual-app checks
and13 rendered cards, covering held precise/fast dragging, compact/standard
left/right edges, explicit Dark/Light, interface scale1/1.25, fullscreen and one
Escape while a preview is visible, decoded resume and normal Quit. A reproduced
AcrylicController shutdown warning is fixed by guarding its disappearing window.
The ordinary R23 GUI starts/quits cleanly in compact Light125%; external input
interference means its hover/drag check is not accepted. Two captures settle
after~1.04s: eventual presentation passes, responsiveness remains open. The
provider/cache is unchanged. Source/artifact checks pass for the new
[bounded receipt](juxtopposed-references/native/home-timeline-thumbnails-20261021/extended-r22/checkpoint.json);
the earlier R17 receipt also verifies against its explicit historical snapshot.
Remaining: stable ordinary hover, latency investigation, Figma backdrop blur,
physical Windows DPI/other layouts, broader/rotated/network media, independent
review and final packaging. These are selected preview checks, not full beta.

21 October timeline previews — implementation and selected native checks pass;
broader feature/design acceptance remains open. The independent image provider,
live Playback toggle (default Off) and profile field are wired. Legacy profiles
without the new field retain Off without a load-time rewrite. Native
backend/settings/profile cases pass65. Private actual-app edit/cold-On/cold-Off
journeys pass135/67/80 checks and capture11 rendered cards; save denial/retry,
audio fallback, paused-clock independence and decoded resume pass. An ordinary
compact-window pointer seek/drag shows previews and quits normally. GUI ordinary
`657ea623af5b…`, private `ad4a8188a0c6…`; Core9f9702/MP4917af1 remain unchanged.
Figma2151:9673 geometry, sampled scene colors and full HH:MM:SS are checked in
these captures. Backdrop blur, held dragging, edge/fullscreen/other layouts,
Light/physical scaling, live/rotated/non-square media, independent review and
final clean packaging remain open. Local evidence is in
`build-windows/beta-timeline-thumbnails-20261021`; this does not replace the
thirteen-category/32-surface beta gates below.

21 October earlier checkpoint: both observed seeking UI P2s pass selected repair checks.
Core `9f9702bec96dâ€¦`, ordinary GUI `c382ebb2f89aâ€¦` and matching
private GUI `054a3f54b009â€¦`. R8 native passes325 checks in three
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
Independent read-only review confirms the bounded repair/proof scopes with no
blocking findings; see the linked checkpoint review.
This closes the two reproduced P2s in these scopes, not full setting/beta,
frame-exact/all-format, physical DPI, visual or final-package acceptance. Continue
the remaining settings consumers and thirteen-category/32-surface roadmap.

21 October work in progress: [precise seeking and MP4 sample lookup](juxtopposed-references/native/home-precise-seek-20261021/README.md).
The paired seek control is enabled in the candidate, with Playback model cases (24)
passing. Testing exposed and repaired an MP4 absolute-index/local-count mistake:
the actual helper changes from 1,121 failures to zero across 1,624 checks. With
the same ordinary GUI `393619cf5176â€¦` and corrected MP4 `917af1d1a006â€¦`, the
previous Fast 34-second request now displays the 30-second keyframe instead of 10.
Core `d04e9134a43fâ€¦` and cache `1190b018b5e8â€¦` are unchanged. Native R4 passes
all six explicit seek routes in both modes and the old policy after save denial.
Final R5 passes 231/32/19 checks in three normal processes, 14 edit seeks plus both
cold policies, actual bounded pointer retry and resumed decoded playback. One
intentional replacement denial is bound; all other diagnostics are clean.
Two P2s remain: paused elapsed/slider retains requested 34 while video lands at 30;
and adding save-error feedback moves the focused Toggle completely below the
viewport. Explicit reveal restores it and retry succeeds. Trace timer/media
generations, then repair both UI behaviors; do not claim full precise-setting,
visual/package or beta acceptance. All 1,295 frozen inputs and runtime/media match.

Prior selected checkpoint: [profile conflict error visibility](juxtopposed-references/native/home-profile-error-20261021/README.md)
passes component30 and native30 checks, plus two ordinary shared-store playing
processes on GUI `b27d70c7f249â€¦`, Core `d04e9134a43fâ€¦`, cache `1190b018b5e8â€¦`.
The complete recovery instruction is revealed automatically after a rejected
profile edit and when reopening with a retained error. The entered failed draft
is preserved until Cancel; saved profiles remain unchanged through rejection
and both normal exits. Native standard/compact125% Dark/Light bounds pass;
ordinary selected recovery is standard Dark. A continues after B exits and
quits directly from fullscreen. All logs are clean; native Stop precedes Qt
teardown, and all1266 frozen inputs/runtime/media identities match. The prior
three-process profile restart/retry passes its earlier GUI scope but retains
its historical clipped-error result. Distinct physical audio outputs, other
shared Qt ownership, physical DPI, broader settings/design and full beta gates
remain open. No release or publication is claimed.

Earlier 21 October: [shared Qt appearance and profile checks](juxtopposed-references/native/home-shared-qt-20261021/README.md)
qualify ordinary GUI `aa6fc26df153â€¦` with unchanged Core `d04e9134a43fâ€¦` and
cache `1190b018b5e8â€¦`. The reproduced stale-window theme overwrite is repaired:
both shared-store closing orders and a cold Light restart pass (five ordinary
normal processes). Latest explicit theme commit survives older fullscreen exits;
other instances retain their active appearance. Actual legacy menu/combo blocked
save, accepted-selection recovery and retry pass15 checks; rebuilt General/setup
passes13 model cases. The unchanged profile store passes20 cases including two
separate stale OS peers, rejection without mutation, unrelated Qt sync and
reload/retry. These are separate scopes; shared-profile menu recovery is qualified by the
separate journey and repair above. Other Qt keys/exit writers, two physical
audio outputs and full beta validation remain open. Window restoration uses one shared last-closed record, including
fullscreen state, while preserving the stored normal rectangle in these runs.

Earlier 21 October cancellation checkpoint: the [input cancellation continuation](juxtopposed-references/native/home-input-cancellation-20261021/README.md)
passes the rapid-Start and restart-before-Quit-finalizer cases that failed on
Core R8. Current Core `d04e9134a43fâ€¦`, cache `1190b018b5e8â€¦`, unchanged ordinary
GUI `d148035d54beâ€¦`. Four console processes cover18 cancellations, playback/seek
recovery, clean WAV-under-MP3 fallback and genuine empty/invalid failures.
Settings regression passes38 checks/27 normal processes/600 concurrent commits/
1,200 nonoverlapping lock events; six selected model processes pass97 cases.
A current ordinary movie/music pair passes independent pause/seek/mute/resume,
fullscreen and direct Quit, clean logs, normal exits and preserved normal bounds.
These two windows share native config but have separate Qt stores and one actual
audio endpoint. Shared Qt profile/geometry ownership and two physical outputs
remain open. Older popup313/geometry194 desktop proofs remain scoped to Core R8;
current full-package/visual/accessibility/OS-DPI acceptance is still pending.
The complete beta goal remains active.

Earlier 21 October R8 checkpoint: the [shared native settings repair](juxtopposed-references/native/home-settings-concurrency-20261021/README.md)
passes real-Core38/27 processes,600 concurrent commits/1,200 nonoverlapping lock
events, Qt backend/model24, matching popup7/313 and geometry4/194. Both ordinary
two-window closing orders preserve unrelated saved edits and independent paused
media, with one fullscreen window. Core `3a7b0a360e3câ€¦`, ordinary GUI
`371bc858e3bbâ€¦`; matching private GUI and source identities are in the receipt.
No unexpected native/Qt errors or forced cleanup. Shared Qt profile/geometry
ownership, simultaneous playback/two-output routing, remaining consumers,
visual/accessibility/OS-DPI and final-package acceptance stay open. Other settings
consumer regression adds General11, Audio32, Network17 and profile-defaults13
(73 additional cases;97 selected model cases total). An ordinary simultaneous
playing run showed independent pause/seek/queue/volume but was rejected for
shutdown-time PCR errors and clipboard warnings. Two90s no-GUI real-output
comparisons and one undisturbed ordinary Stop-before-Quit comparison are clean.
The [application Quit repair](juxtopposed-references/native/home-quit-stop-20261021/README.md)
passes ordinary audio/audio and movie-plus-music pairs (four processes): direct
fullscreen Quit leaves the other playing, then its direct Quit is clean too.
All preserve normal window bounds. Four earlier private audio cases and three
corrected private video/combined-media cases pass. Rapid-start cancellation
diagnostics remained open on that tuple; GUI `d148035d54beâ€¦`, Core R8 unchanged. Only one explicit audio endpoint is available; Default aliases it.
The complete beta goal remains active.

The earlier [whole-row popup focus repair](juxtopposed-references/native/home-popup-duration-activation-20261020/focus-repair-r4/README.md)
passes its distinct native7/313, ordinary1 and component12 scopes after the old-
source negative control. Title, slider, scale and full note remain visible.
Its old Core/GUI identities are historical; the new matching replay is above.
Native narrow/Light, title alignment and actual OS-DPI remain open.

Updated 20 October 2026 after published checkpoint `1a174fc719`.
**Status: implementation and scoped validation are substantial; the complete
beta is not accepted.** This is the current execution order. The
[older beta plan](juxtopposed-beta-plan.md) retains historical receipts and
reference requirements; its old pending labels do not override newer evidence.

## Target and boundaries

Bring Juxtopposed's **confirmed redesign** to life with real VLC behavior,
consistent visuals, persistence, usable interaction and one reproducible Windows
x64 package. Complete the designed actions rather than a demonstration action per
screen. Preserve working native routes and traditional-control opt-out.

Beta covers the confirmed design and stability. V1.0 covers broader vanilla VLC
parity and the disc/ISO/Explorer/AutoPlay discussion. V1.5 may explore additional
YouTube/Twitch extensions. A designed menu entry must work; it does not require
inventing a redesigned dialog that the references never show. Conversion's
current functional adaptation must remain clearly identified where no original
form exists. Unconfirmed subtitle text editing/export is a reference question,
not an assumed beta feature. No designer outreach is part of this roadmap.

## Current goal continuation â€” 20 October

Resumed after `ed2f5d0155`: the user requested following this complete roadmap
until further notice or beta completion. Both monitors are available for visible
tests until further notice. [Geometry activation](juxtopposed-references/native/home-geometry-activation-20261020/README.md)
passes four selected native cases/194 and three ordinary sessions, including
actual editor/retry/reopen, Off-save/cold policy and active media preservation.
The strict full-run playing timing error is retained separately; the isolated
playing repeat passes without native/Qt errors. Offscreen recovery, the cold
fullscreen lower-edge observation, actual OS DPI and final package stay open.

The user resumed work on window-position memory and popup duration. The full
objective and all final-package/visual/platform gates below remain in force.
The geometry toggle is now linked with the activation scope above. The earlier
R3 proof retains its own disabled-gate snapshot. The later
[popup activation](juxtopposed-references/native/home-popup-duration-activation-20261020/README.md)
links the real tray consumer with actual7/283, three ordinary sessions and
component12. Pointer/keyboard, save failure/retry, cold0/10000, Never/minimized/Off
and active state preservation pass. Genuine Windows handler arguments are
verified; OS visibility/expiry is not guaranteed. GUI ordinary `72ba439a30f0â€¦`,
private `1f83aa0cbefcâ€¦`, source-r3 in `beta-popup-duration-20261020`. Rejected
fixture link/negative-control runs remain separately recorded; ordinary production
was unchanged by those repairs.
Popup review found a keyboard visibility defect in the actual R3 capture: the
focused slider was visible but the title, scale and timing note were clipped.
The new whole-group viewport assertion fails on the old source and passes with
a popup-only focus-bounds repair in component12, including narrow and enlarged
layouts. Matching R4 native7/313 and ordinary1 now pass; the new Core replay
above also passes. The P3 title
alignment refinement and complete visual/OS-DPI acceptance remain separate.
The [initial review](juxtopposed-references/native/home-interface-review-20261020/README.md)
identified cold fullscreen/maximized normal-bounds loss, a popup zero-duration
wording gap, and saved-offscreen recovery coverage. The normal-bounds repair now
has the scoped proof below. Findings do not supersede confirmed-action work.
Code Review also confirms all four PR1 checks pass on exact published head
`1a174fc719`; this does not qualify current local edits. Its Node20 action-runtime
warning and Ubuntu26 runner migration are lower-priority CI maintenance items
in `.github/workflows/qml-ui-checks.yml`, recorded in the review receipt above.

- [Geometry progress](juxtopposed-references/native/home-geometry-memory-20261020/README.md):
  native normal control passes13; maximized/fullscreen saves lose normal bounds.
  First QPA repair is rejected for an8-pixel CSD shift, confirmed by ordinary
  cold reopen. R2's actual Windows system-menu maximize overwrites normal bounds;
  R3 defers updates until state settles. Six native processes/89 checks and
  three ordinary cold max/fullscreen sessions pass with exact bounds/position,
  normal exits and no Qt/native errors. Hidden cross-monitor and minimized saves
  pass. GUI ordinary `6de416fa3caeâ€¦`, private `7f7c229901faâ€¦`, source-after-r3.
  Real editor/Off-save/cold policy, active media and recovery/DPI stay open. A thin
  lower-edge band on cold fullscreen needs actual startup client/frame diagnosis.
- [Popup baseline](juxtopposed-references/native/home-popup-duration-20261020/README.md):
  genuine Windows platform notification receives saved777ms on the identified
  ordinary baseline. No Qt/native errors and normal Ctrl+Q exit. Real editor,
  persistence/cold restart, dependencies and zero/default wording remain open.
  OS expiration timing is not accepted. Popup duration remains gated; geometry
  is linked only in the newer local activation candidate pending editor acceptance.

- [Auto-hide activation](juxtopposed-references/native/home-auto-hide-20261019/README.md):
  actual mode/delay editors and paired pointer/hover repairs pass the source-
  matched R9 twelve-process/666-check matrix, with normal exits, no Qt diagnostics
  or unexpected native errors. One deliberate locked-save error is retained.
  Never/Fullscreen only/Always, audio/video, docked/floating, paused/playing,
  real0/1000/2000/10000ms timers, doubled keyboard reveal and independent state
  preservation pass. Component9/43/47 and ordinary cold hide/reveal/hover have
  distinct scopes. R8a identified the old keyboard assertion as a test deadline
  error. Current ordinary `598173d1ba52â€¦`, private `dc2456a80a6câ€¦`, source-r9;
  `build-windows/beta-auto-hide-20261019/ordinary-runtime-r6`.
  The subsequent screen-selector scope below closes another Interface gate.
- [Fullscreen screen activation](juxtopposed-references/native/home-fullscreen-screen-20261019/README.md):
  actual five-process/399 and two ordinary Windows sessions pass, with normal
  exits, no Qt/unexpected native errors and one deliberate locked-save error.
  Actual category-open availability, editor/retry/reopen, cold/stale handling,
  real window-handler target acquisition and hidden genuine cross-screen restore
  preserve input/queue/tracks/clocks. All visible presentation stays on monitor2.
  Current ordinary `56a4a7620f15â€¦`, private `69af3dfee412â€¦`, source-r3;
  `build-windows/beta-fullscreen-screen-20261019/ordinary-runtime-r1`.
  Geometry and popup duration remain gated; whole-app/DPI/package acceptance is open.

- [Settings activation](juxtopposed-references/native/home-settings-activation-20261019/README.md):
  fullscreen and authored volume-step gates are linked independently; receiver
  support has its own gate. Native Audio32/Windows4 and actual settings
  paused96/reopen77/playing92 pass on R4. The restart-notice
  focus/clipping repair passes component11 and the native captures.
- Ordinary R4 Windows typing saves the step correctly; mouse-focused Right/Left
  change volume by2.5 points without seeking the paused primary. The reproduced
  R3 app failure is repaired by explicit plain-arrow ownership. Key routing12
  retains modified-shortcut/modal/sidebar policies; component evidence is separate.
- [FLAC tail repair](juxtopposed-references/native/home-flac-coded-tail-20261019/README.md):
  exact decode improves from479,232 to480,000 samples with the source PCM digest;
  paired/current packetizer8 and existing encoder/drain9 pass. Preserve the new
  packetizer/demuxer with the current GUI when consolidating the candidate.
- R4 settings/fullscreen GUI pair is ordinary `758df308cca3â€¦`, private `f7e7d95db75eâ€¦`.
  The combined local runtime is
  `build-windows/beta-settings-activation-20261019/ordinary-runtime-r4`.
  This is not final-package acceptance. Later tooltip/video warning receipts
  follow below; full consumer/workflow, fidelity/accessibility/DPI and release
  gates stay open.
- Affected fullscreen replay on R4 passes audio/video On/Off138 with four normal
  exits, no Qt/native errors and exact paused state preserved. Video-pixel and
  initial window-size-warning review remain separate from control/focus proof.
- [Tooltip follow-up](juxtopposed-references/native/home-titlebar-tooltip-20261019/README.md)
  reproduces the Close popup outside monitor2. R6's local popup fixes bounds34,
  but capture review identifies duplicate hover/focus help. R7's hover-priority
  component regression passes after a paired R6 failure. Native39 and direct
  ordinary maximized Closeâ†’Homeâ†’Close help pass normally without Qt/native
  errors. Tooltip-scope ordinary GUI `9892032f4456â€¦`, private `a14c5d7c7cb4â€¦`,
  source-r7 and `build-windows/beta-settings-activation-20261019/ordinary-runtime-r7`.
  Geometry readback finds the old normal-window fixture extended19 pixels off
  screen; a fresh in-bounds R7b replay accepts coordinate Restore/Maximize with
  normal exit/no Qt/native errors. Full geometry/Windows-DPI acceptance stays open.
  Keep R4's earlier settings/fullscreen evidence in its original scope.
- [Video-window warning scope](juxtopposed-references/native/home-video-warning-20261019/README.md):
  ordinary R7 H264 four-color frames render through paused startup, playing,
  paused fullscreen and windowed return. The one initial zero-size warning is
  followed by nonzero Direct3D11 reports and normal exit without Qt/native errors.
  This is scoped fallback/rendering proof, not all-format/GPU/DPI/package acceptance.

- [Windows Audio activation](juxtopposed-references/native/home-audio-device-activation-20261019/README.md):
  Output device/fixed startup volume now linked independently from receiver
  passthrough. Component20 and actual stale31/paused83/reopen59/playing95/
  unmanaged22 pass, five normal exits and no unexpected native/Qt errors.
  Three intentional negative-control errors remain recorded. Cold real Windows
  output consumes saved explicit endpoint65%; defaults editing preserves current
  playback/output identity. Ordinary Windows typing55%, device keyboard choice,
  policy changes and dialog reopen pass with checked persistence and normal exit.
  Audio-scope ordinary GUI `3857fa66519câ€¦`, private `de7506005fcaâ€¦`, local
  `build-windows/beta-audio-device-activation-20261019/ordinary-runtime-r1`,
  source-r1/run-source-r2. Protected Core/audio/FLAC/library identities unchanged.
  Earlier R4/R7 scopes do not automatically become results on this new GUI.

- [Interface title activation](juxtopposed-references/native/home-interface-title-20261019/README.md):
  actual initial52/Off45/restored46 =143, three normal exits, no Qt/unexpected
  native errors; one intentional locked-save error retained. Actual pointer/
  keyboard editor, rollback/retry, restart feedback, dialog reopen, cold audio/
  video captions and same-instance next-video policy pass. Two ordinary Windows
  sessions confirm editing and cold Off with cleared restart feedback, unchanged
  config/media/runtime and normal exits. Current ordinary GUI `d40f11aeefdfâ€¦`,
  private `4e09011ce1eeâ€¦`, source-r1 and
  `build-windows/beta-interface-title-20261019/ordinary-runtime-r1`.
  This earlier title scope preceded the auto-hide activation above;
  whole-app/package/visual/DPI are open.

## Baseline to preserve

- Authoritative checkout: `C:\Modding\vlc-juxtopposed`, branch
  `juxtopposed-playback-bar`, published checkpoint
  `1a174fc7195d681df4e0d2e658e03f7cbddc9b95`. Preserve unrelated local work,
  including the four untracked SoXR reference/trace experiments. They are not
  prerequisites for this roadmap.
- Published-checkpoint matched GUI pair: ordinary
  `fd7f8af9b480f4e3b1dd8b4df22ae3fe7e58b655d13914b55658f405e6d6346b`,
  private test GUI
  `8be8eff6e4d877f4369a44f0df4424855349b90ec25de8d77a9ce9a0e1ec4115`.
  The ignored ordinary runtime is
  `build-windows/beta-fullscreen-controls-20261019/ordinary-runtime-r2`;
  its source checkpoint is `source-r6` in the same work folder. The
  [provenance receipt](juxtopposed-references/native/home-fullscreen-controls-20261019/actual-app-provenance.json)
  records matching production inputs and the absence of private hooks in the
  ordinary GUI. This is a scoped development candidate, not the final package.
- [Home cache/audio receipt](juxtopposed-references/native/home-cache-audio-20261018/README.md):
  native cache/icon/window 16 processes / 3,297 checks; paired old-Qt negative;
  audio concurrency32, actual WASAPI lifecycle16, five ordinary Qt checks and HLS20.
  These are scoped results, not a cumulative feature-completion percentage.
- Historical protected ordinary baseline:
  `build-windows/beta-native-cache-home-20261018/ordinary-runtime-r3`;
  GUI `f82534592d35â€¦`, MMDevice `0d29a257b562â€¦`, Core `0cdebe2f6950â€¦`.
  [Full hashes](juxtopposed-references/native/home-cache-audio-20261018/verification.json)
  bind results to the actual binaries and source.
- Historical interaction candidate: `build-windows/beta-compact-interaction-20261018/ordinary-runtime-r10`,
  GUI `19ec1a17ac1fâ€¦`, with the preserved core/audio/library modules. Six private
  native journeys and five ordinary playback smokes pass in their explicit
  scopes; see the [interaction receipt](juxtopposed-references/native/home-compact-dialogs-20261018/README.md).
  Historical physical Shift+F10 failed in R10. The scoped repair passes direct
  Windows input in ordinary setup R2; see the
  [repair receipt](juxtopposed-references/native/home-compact-dialogs-20261018/native-keyboard-repair.json).
  The later [focus receipt](juxtopposed-references/native/home-keyboard-focus-20261019/verification.json)
  closes the scoped keyboard-return outline defect: Home ON/OFF188 each,
  Recovery321, compact video741/audio742, five ordinary smokes and direct
  Windows keyboard ring proof. Complete accessibility/visual/DPI/package
  acceptance remains open. This is not the final beta package.
- All 13 Global Settings bodies exist with native services and scoped journeys.
  Ordered pins, Library-OFF Recent, the original shared title strip and original
  Videos/Music icons are implemented. Setup, profiles and the rendered customizer
  also have actual native and ordinary interaction receipts, summarized below.
  Revalidate their remaining states; do not restart them from obsolete TODOs.
- Movable modal headers already exist for Network, New Playlist, Conversion and
  Global Settings, with scoped native interaction evidence in the compact-dialog
  receipt. Remaining short-window, busy-operation, visual and Windows-DPI
  combinations need their own evidence. New Playlist's source already uses the
  original 713 Ã— 589 geometry.
- The cleanup preserved source/Git/current candidates, migration backups and the
  Windows 10 kit. Historical identical build outputs now share NTFS storage;
  unlink/copy an old binary before changing it. Active tools and builds were excluded.

The updated [32-surface readiness snapshot](juxtopposed-references/beta-readiness-20261018.md)
separates known implementation, native evidence and remaining work. Earlier
native results retain their original GUI/platform scope until replayed against
the final candidate. Nothing here accepts the whole application's visuals.

## Progress since the previous roadmap

These are bounded receipts, not cumulative completion percentages. Counts from
overlapping revisions must not be added together.

| Area | Evidence to preserve | Still open |
| --- | --- | --- |
| Compact navigation and focus | [Native interaction](juxtopposed-references/native/home-compact-dialogs-20261018/README.md), later [focus repair](juxtopposed-references/native/home-keyboard-focus-20261019/verification.json), direct ordinary Windows keyboard checks | Remaining interaction combinations, screen-reader behavior, actual OS DPI and final package |
| First-run setup | [Cancel/Finish/retry/reopen and ordinary keyboard receipt](juxtopposed-references/native/home-setup-20261019/verification.json); native 49/60/18 and direct ordinary Next/Finish/reopen | Complete visual/DPI/package states; privacy choices remain human-owned |
| Profiles and customizer | [Profiles](juxtopposed-references/native/home-profiles-20261019/README.md) native playing/paused79 each and reopen15; [customizer](juxtopposed-references/native/home-customizer-20261019/README.md) paused111/reopen15/playing111, plus ordinary New/save/switch and layout-edit input | Remaining confirmed tile/shortcut actions, full visual/accessibility/DPI and package coverage |
| Current-media Favorite | [Native183 and ordinary pointer/Space/Return](juxtopposed-references/native/home-current-media-20261019/README.md), independent database readback, failure/retry and source guards | Whole-layout visual and screen-reader acceptance; targeted monitor-edge tooltip repair |
| Playlist-origin label | [Actual Library On/Off67](juxtopposed-references/native/home-playlist-context-20261019/README.md): explicit origin, real names, shared media, rename/deletion and paused-state preservation | Remaining workflow/visual/DPI/package states |
| Embedded fullscreen controls | [Actual audio/video On/Off138](juxtopposed-references/native/home-fullscreen-controls-20261019/README.md), hidden-input rejection and focus/state preservation | Enable its ordinary settings gate and test the real editor/restart contract; detached controller remains unavailable |
| Settings consumers | [Playback/Audio console R18](juxtopposed-references/native/home-playback-consumers-20261019/README.md), 18 cases with verified dummy sinks; [Windows Audio](juxtopposed-references/native/home-audio-consumers-20261019/README.md), actual MMDevice/WASAPI endpoint/startup-volume/live-step behavior | Per-field desktop audit, gated controls, hardware-specific paths and final package; time-shift disk spooling/rollover/cleanup remain unverified |

## Concrete gaps to resolve

The [thirteen-page consumer audit](juxtopposed-settings-consumer-audit.md) is the
field-level backlog. A stored value or enabled component is insufficient. For
each confirmed setting, establish its real consumer, availability, timing,
save/reset/cancel/retry behavior and actual desktop result.

- The fullscreen option and authored Audio volume step are now linked locally;
  their settings journeys and ordinary pointer/Left/Right volume input pass in
  the scope above. The older disabled-gate notes describe the published checkpoint.
- Audio Windows-device and receiver capabilities are separate. Device/fixed
  startup consumers are now linked and pass the actual/ordinary activation
  receipt above. Passthrough remains unavailable; it does not inherit endpoint
  selection proof. Physical unplug and receiver coverage remain separate.
- Other gates or missing consumers include Interface geometry and popup-duration
  policies, precise seek, timeline thumbnails, wallpaper, selected Audio
  effects and visualization behavior, subtitle ASS override and playlist-tree
  mode. Use the audit to distinguish verified-but-gated, absent, platform-limited
  and unconfirmed behavior. Do not silently drop a confirmed design action.
- Finish remaining customizer actions such as HDR where confirmed, shortcut
  consumers/conflicts, library/category/detail and queue/bookmark/chapter/track
  journeys, subtitle scope/timing, and conversion lifecycle/output acceptance.
  Favorite and authoritative playlist labels are no longer missing consumers.
- The FLAC frame-size warning's tail loss is repaired; exact PCM and packetizer
  evidence is linked above. The dummy-mux warning is normal last-input removal
  in the tested runs. The initial video-size fallback has scoped ordinary R7
  rendered-image/resize evidence above.
  Title-strip tooltip placement/single-help checks pass on R7. The old off-screen
  geometry fixture and corrected R7b comparison are recorded; full geometry/DPI
  policy acceptance remains part of the Interface audit.

## Ordered work

| Order | Work package | Concrete completion evidence |
| --- | --- | --- |
| 1 | Activate verified settings and close the immediate regressions | Enable embedded fullscreen controls and authored volume-step capabilities independently, build matching ordinary/private inputs and verify actual settings editors, persistence and application timing. Keep unverified receiver paths gated. Triage native warnings and the monitor-edge tooltip. |
| 2 | Complete confirmed consumers and workflows | Close each field in the thirteen-page audit and each remaining action in the readiness snapshot using existing code first. Finish the remaining tile/shortcut, library/detail, queue/bookmark/chapter/track, subtitle and conversion paths. Establish current-versus-next-media/restart behavior, reset, failure/retry and persistence. Preserve completed setup/profile/customizer work. |
| 3 | Complete interaction, design fidelity and accessibility | Compare actual VLC captures with cached original frames/assets. Review all 32 surfaces and their required states, typography, spacing, icons, colors, blur/alpha, sizes, focus and screen-reader behavior. Finish remaining compact/wheel/drop/modal movement and busy-state combinations. Verify Windows DPI, System-theme notifications and monitor placement. Resolve reference gaps explicitly. |
| 4 | Consolidate real-media and state regression | Run the designed journeys on one identified current tuple with representative real/generated media, playing and paused states, restart, rapid source changes, missing/moved media, failure/retry and worker shutdown. Retain exact output/PCM tests. Triage every new crash or normal-flow failure. |
| 5 | Produce and validate a clean beta candidate | Build from the selected fixed source using registered dependencies/resources. Stage an ordinary package with no private hooks/data. Replay the required native/component/visual/media checks on those exact packaged binaries. Verify default first run, relocation, installed-VLC coexistence, licenses, attribution, source/build notes and checksums. Prepare reviewable Git changes and the local distributable. |
| Separate lane | Windows 10 comparison | Run the prepared before/after kit when the original guest machine is available. Record OS/AUDIOSES versions and capture the exception context if the crash recurs. Never substitute a Windows 11 pass for this result. |

Work packages are ordered batches, not fixed-duration promises. Close defects
and gather evidence during implementation rather than postponing all validation
to the end. Do not repeatedly rebuild/retest unchanged accepted work. A product
change, new failure, changed binary or unresolved coverage question justifies
the next relevant check.

### Multi-instance regression requested 20 October

Current storage policy and qualification:

| State | Policy and present evidence |
| --- | --- |
| Current media/queue, transport, tracks, volume/mute, fullscreen and output | Owned by each running process. Selected two-window audio/movie pause/seek/mute/fullscreen/survivor checks pass; other track/rate/queue/hotkey/handoff and physical-output combinations remain open. |
| Saved native defaults | Shared file; merge intentional keys, last successful same-key commit wins. External defaults do not automatically overwrite another process's live state. Concurrency/failure/exit regressions pass in the documented scopes. |
| Player profile catalogue | Shared checked catalogue with stale-edit rejection. Two native OS peers preserve the newer catalogue and recover by reload/retry. Ordinary shared-profile conflict/restart/retry and the later complete-error visibility repair pass their separate documented scopes; complete profile application remains open. |
| Appearance theme | Shared saved default, explicitly committed when chosen. Active windows keep their local theme; exit does not recommit it. Both closing orders/cold startup and legacy failure/retry pass. |
| Window restoration | One shared last-closed record, normal rectangle plus window/fullscreen state. These normal rectangles survive fullscreen Quit; there are no separate persistent identities per instance. Broader DPI/offscreen recovery remains open. |
| Privacy/network/library/credentials | App-wide policy with each setting's documented application timing; excluded from media profiles. No new security or privacy changes are implied by these tests. |

Other MainCtx Qt keys still write at shutdown and need explicit stale-writer
review. Do not generalize the theme repair to those keys or all application
settings. Generated fixtures are silent and the available Default endpoint
aliases the one explicit physical output.

The user's movie-plus-music scenario is a beta regression requirement. Keep the
distinction between a running player's controls and saved startup defaults
visible in help text; do not imply that changing an Audio default reroutes the
currently playing output. The live device selector operates on that player's
audio output, whereas the Audio preferences device is a saved startup policy.
Profiles group playback/audio/video/subtitle and player-layout preferences;
their shared catalogue is not a separate executable instance or an isolated
configuration directory.

| Remaining scenario | Required observation |
| --- | --- |
| Movie fullscreen on one monitor, music window on another | Changing media, queue, tracks, rate, seek, mute or volume in the focused player leaves the other player intact. Escape/fullscreen and keyboard/media-key routing have documented targets. |
| Two genuinely distinct audio devices | Assign A to speakers and B to another real endpoint; switch one while both play and verify the other endpoint/session stays unchanged. Default plus its named alias is insufficient evidence. |
| Device loss or Windows default-device change | Check explicit-device versus Default behavior, useful recovery/error feedback, and survival of the unaffected player. Do not silently accept cross-routing as success. |
| Shared settings and profiles | Check unrelated concurrent saves, same-key policy, rejected writes/retry and both closing orders. Another window's exit must not restore stale defaults or reroute active playback. Complete the remaining Qt-key ownership review. |
| Monitor topology and restart | Move/fullscreen each player, close in both orders, and check saved normal bounds and offscreen recovery after a monitor disappears. Current restoration has one shared slot; separate remembered startup slots are not implemented. |
| Separate portable/configuration roots and file-manager launches | Distinguish intentionally isolated storage from shared defaults; verify open/enqueue/single-instance policy without interrupting an unintended player. |

Two physical outputs and physical monitor/DPI/topology changes remain pending
where the available hardware cannot establish them. Do not add a new global/
per-window switch or promise per-instance persistent window identities without
an explicit product decision.

**Original P1 reproduced and repaired in the selected native-settings scope.**
The [ordinary baseline](juxtopposed-references/native/home-multi-instance-20261020/README.md)
records A's duration5000 being erased by B saving geometryOff, then B's Off being
erased by A's normal exit. The [Core R8 repair](juxtopposed-references/native/home-settings-concurrency-20261021/README.md)
records600 concurrent commits,24 backend/model cases, matching GUI popup313 and
geometry194, and both ordinary two-window close orders preserving5000/Off.
One player stays fullscreen and each keeps its own paused media. Same-key last
successful commit wins; external saved defaults do not alter live values.
Shared Qt profiles/window-state storage and two physical audio endpoints
remain open; current ordinary simultaneous playback is qualified above. This is not complete instance qualification.

Verify two genuine instances with different media on different monitors and,
where available, different Windows audio endpoints. Keep one fullscreen while
using the other. Check pause/seek/queue/tracks, rate/volume/mute, output selection,
fullscreen return, hotkey routing, instance handoff and independent shutdown.
Separate active per-process state from shared saved defaults/profile selection:
changing a default must have documented application timing, and saving from a
stale second process must not silently erase another process's unrelated changes.
Test shared configuration and explicitly separate portable/configuration roots;
do not infer shared-storage isolation from earlier isolated test profiles.
Source inspection shows native `one-instance` defaults false, while file-manager
launches retain a separate policy. Selected simultaneous playback passes above;
distinct output routing, profile/shared-settings conflicts and geometry ownership
still need qualification.
The original `config_SaveConfigFile` serialized a stale full snapshot. Core R8
instead merges intentional pending keys into the latest file while holding the
shared sidecar lock. Checked transactions restore previous values/pending state
on failure and acknowledge successful saves before an older process can exit.
Other consumers and final-package regression still need qualification.
Do not assume a new per-instance settings UI or synchronized playback is part of
the confirmed design; record any required product-policy decision explicitly.
The ordinary playing journey also showed a stale **Play** tooltip while the
button/action was **Pause**. Keep this P3 label refresh issue in the UI follow-up;
it has not been repaired or accepted by the shutdown work.
The [Quit continuation](juxtopposed-references/native/home-quit-stop-20261021/README.md)
passes ordinary simultaneous audio/direct shutdown and preserves normal bounds.
The old rapid-start cancellation errors are preserved as a negative control.
The current two-file cancellation repair passes the same private R5 fixture and
restart case, plus explicit console recovery/genuine-failure checks above.
Corrected video teardown and ordinary movie-plus-music scope pass; other
protocols/video backends and the full beta still need qualification.

### First batch for the next goal

1. Preserve the published baseline and the newer frozen local inputs, source
   snapshots and receipts. Preserve the repaired Core R8 transaction/merge
   behavior and protected audio/library modules; keep unrelated SoXR work out
   of this change.
2. Preserve R4's repaired mouse-focus key ownership, matching ordinary/private
   source inputs and repaired FLAC modules. Settings265, routing12 and ordinary
   Windows editor/Left/Right input pass. Keep component/native/ordinary evidence
   distinct; do not replay unchanged accepted checks without a new reason.
3. Preserve affected fullscreen qualification on R4 and ordinary R7 video-warning
   scope. Retain actual settings failure/retry, cold reopen and paused/playing
   evidence; these are not complete package or visual acceptance.
4. Preserve R7's tooltip bounds/single-help and ordinary maximized input proof,
   plus the R7b in-bounds geometry comparison. Use that owned fixture for future
   ordinary replay; do not infer full geometry/DPI acceptance from it. Preserve
   authored title-strip assets and geometry.
5. Preserve the current Audio activation tuple and its290 actual-app checks,
   component20 and ordinary Windows input/readback. Keep receiver gated and
   hardware labels private. Preserve title activation143 and its two ordinary
   sessions. Auto-hide/delay now have actual666 and ordinary cold/hover proof;
   screen selection now has actual399 and ordinary cold/editor proof;
   continue geometry, popup duration and remaining
   confirmed consumers/workflows in work package2; the complete scope remains open.

Use the maintained startup hold for fresh owned-window inspection before each
desktop journey. Avoid duplicating the Windows runtime's default plugin path:
the accepted ordinary environment loads601 modules once; an extra
`VLC_PLUGIN_PATH` previously caused duplicate autorun/hotkey startup failures.
Audit other launchers before reusing them rather than assuming all were repaired.

This batch ends with enabled, usable settings and a matched receipt. It is not
the whole beta, and it should not be spent merely writing another plan.

## Model and reasoning recommendation

Planning recommendation as of 19 October 2026: **GPT-6.1 Sol with High reasoning**
for the main beta implementation and verification work. This is project-specific
judgment, not a benchmark result or a guarantee of acceptance.

| Work | Suggested setting |
| --- | --- |
| Cross-file Qt/QML/C++ changes, settings consumers, native test design and regression review | GPT-6.1 Sol, High |
| Bounded documentation, receipt reconciliation and straightforward presentation edits | GPT-6.1 Sol, Medium; lower only while the quality checks still pass |
| Difficult native crashes, asynchronous ownership/teardown or unexplained state races | Try GPT-6.1 Sol, Extra High for that investigation; compare with High against a concrete reproduction |
| A hard issue remains unresolved or a particularly consequential review needs another approach | Consider GPT-6 Astra, High/Extra High; preserve the same reproduction and acceptance criteria |

Do not use Max/Ultra for every turn by default. Keep the lightest setting that
meets the demonstrated quality bar. Higher reasoning cannot replace an actual
Windows test, missing hardware or an original-design reference. This document
does not change the user's selected model or effort.

Current [OpenAI model guidance](https://developers.openai.com/api/docs/guides/model-selection)
and [effort guidance](https://developers.openai.com/api/docs/guides/deployment-checklist)
support choosing by workload and measured quality, latency and usage. Recheck
availability and guidance when selecting a different model later.

## Acceptance rules shared by every work package

Keep four independent statuses for each surface/action: **implementation,
native verification, reference/visual acceptance, final-package verification**.
A component render or native backend fixture does not establish a full desktop
journey. A new GUI does not inherit every older GUI's pass automatically.

- Exercise applicable empty/loading/selected/disabled/error/retry states, long
  names and unknown durations. Check actual pointer, keyboard, wheel and drop
  behavior where the design offers it, including accessible names and focus.
- Preserve the primary input, queue, track identity and clock during navigation,
  dialogs and background jobs that promise independence. Check paused behavior
  separately from playing behavior and verify real restart persistence.
- Cover video, audio, chaptered media, multiple audio tracks, embedded/external
  subtitles, tagged and missing artwork, unknown-length/live inputs and conversion
  output. Use the authorized `D:\Media` collection read-only; keep private names,
  frames and source bytes out of Git and distribution.
- For subtitle timing, distinguish a local SRT snapshot from decoder/renderer
  truth. Confirm source/generation guards and the designed timing interaction;
  do not advertise unverified embedded-cue or editing semantics.
- For export, verify playable output and complete samples, cancellation, failed
  publication, destination preservation and teardown. Retain the strict
  88,200-sample FLAC check; duration tolerance alone cannot prove the tail.
- Use normal, narrow and short windows; Dark, Light and actual System changes;
  nondefault accent/font; 100/150/200% **Windows** DPI and cross-monitor movement;
  and applicable pinned/floating/fullscreen/detached states. Component `uiScale`
  is not OS-DPI evidence. Use representative combinations plus all known risky
  intersections, rather than an unbounded Cartesian product.
- Capture the actual app for visual review. Preserve the prior targets of at
  least 9.5/10 for assessed canonical screens and 9/10 for assessed variants,
  supported by named geometry/asset/type/color/interaction criteria. These are
  review targets, not automatic scores from test counts. Unassessed states stay open.
- Accepted runs require normal termination, no forced cleanup, no unexplained
  native exceptions/assertions or Qt/QML warnings, and no lost/resurrected state.
  Keep intentional negative controls and rejected attempts clearly separate.

## References and practical constraints

Use preserved Figma context/screenshots/assets, the original walkthrough and
exact-time extracts. The user's 20 October goal renews permission to use Figma
MCP when needed. One targeted design-context read of original Interface node
`3062:11379` succeeded, including its screenshot; it confirms the existing row
and host layout. Reuse cached references and reserve further calls for concrete
questions and validation, recording node, purpose and outcome. Follow the Figma
skill when used.

The user permits either monitor for visible app checks until further notice.
Follow the Computer Use skill when direct
desktop control is needed. Prefer maintained native harnesses for repeatable
checks. Keep isolated test profiles and normal user settings separate. Do not
lower UAC, disable protection or change firewall policy to make testing easier.
Only request user help for a prompt or decision actually encountered.

Use the registered `VLC-Juxtopposed-Home-20261018` build distro and current home
paths; keep the original distro and migration copies. Start native builds with
two workers and one writer to a build tree. Historical guest E:-only paths and
one-worker/4-GiB limits are not the current home configuration. Do not install
new dependencies without authorization.

## Deferred checks and release decision

The original Windows 10 AUDIOSES exception remains open. A real teardown race
was repaired and tested on Windows 11; the original exception context is missing
and causality is not proven. The user explicitly deferred the unavailable
Windows 10 machine. The verified 28.7-MB comparison kit is ready under
`build-windows/beta-native-cache-home-20261018` with before/after modules and
instructions. This deferral does not stop the Windows 11 work packages.

The next goal can deliver the complete implementation and a Windows 11-tested
beta candidate with this external check clearly deferred. It cannot claim full
Windows 10/11 acceptance or close the historical crash. Before publishing a
beta that claims both platforms, complete that check and resolve any failure,
or obtain an explicit user decision changing the advertised support scope.
Physical audio-device loss and a real cast receiver also need truthful separate
coverage statements when those paths are advertised; do not require every
possible hardware/service combination to be exhaustively proven.

The final local candidate needs a supported-features/limitations matrix, exact
revision and hashes, clean startup and relocation proof, dependency/font/designer
licenses and credit, reproducible source/build instructions and independent
change/visual review. Package size is secondary to a working licensed beta.
Choose portable ZIP versus installer and version/signing status explicitly;
do not spend the implementation goal optimizing package size or adding an installer.

Prepare a concrete reviewable commit/package before any final approval step.
This planning request does not authorize a new commit/push, tag, GitHub release,
external upload or message. Obtain publication authorization when the result is
ready, and never rewrite history or discard unrelated work. The
[prepared goal brief](juxtopposed-beta-goal.md) preserves the full scope and
deferred-platform boundary for the next goal.
