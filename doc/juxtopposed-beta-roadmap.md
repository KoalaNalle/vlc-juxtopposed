# Revised beta roadmap

21 October checkpoint: both observed seeking UI P2s pass selected repair checks.
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
