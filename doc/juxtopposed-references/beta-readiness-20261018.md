# Beta readiness snapshot — updated 21 October 2026

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

21 October work in progress: [precise seeking and MP4 sample lookup](native/home-precise-seek-20261021/README.md).
The paired seek control is enabled in the candidate, with Playback model cases (24)
passing. Testing exposed and repaired an MP4 absolute-index/local-count mistake:
the actual helper changes from 1,121 failures to zero across 1,624 checks. With
the same ordinary GUI `393619cf5176…` and corrected MP4 `917af1d1a006…`, the
previous Fast 34-second request now displays the 30-second keyframe instead of 10.
Core `d04e9134a43f…` and cache `1190b018b5e8…` are unchanged. Native R4 passes
all six explicit seek routes in both modes and the old policy after save denial.
Final R5 passes 231/32/19 checks in three normal processes, 14 edit seeks plus both
cold policies, actual bounded pointer retry and resumed decoded playback. One
intentional replacement denial is bound; all other diagnostics are clean.
Two P2s remain: paused elapsed/slider retains requested 34 while video lands at 30;
and adding save-error feedback moves the focused Toggle completely below the
viewport. Explicit reveal restores it and retry succeeds. Trace timer/media
generations, then repair both UI behaviors; do not claim full precise-setting,
visual/package or beta acceptance. All 1,295 frozen inputs and runtime/media match.

Prior selected checkpoint: [profile conflict error visibility](native/home-profile-error-20261021/README.md)
passes component30 and native30 checks, plus two ordinary shared-store playing
processes on GUI `b27d70c7f249…`, Core `d04e9134a43f…`, cache `1190b018b5e8…`.
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

Earlier 21 October: [shared Qt appearance and profile checks](native/home-shared-qt-20261021/README.md)
qualify ordinary GUI `aa6fc26df153…` with unchanged Core `d04e9134a43f…` and
cache `1190b018b5e8…`. The reproduced stale-window theme overwrite is repaired:
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

Earlier 21 October cancellation checkpoint: the [input cancellation continuation](native/home-input-cancellation-20261021/README.md)
passes the rapid-Start and restart-before-Quit-finalizer cases that failed on
Core R8. Current Core `d04e9134a43f…`, cache `1190b018b5e8…`, unchanged ordinary
GUI `d148035d54be…`. Four console processes cover18 cancellations, playback/seek
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

Earlier 21 October R8 checkpoint: the [shared native settings repair](native/home-settings-concurrency-20261021/README.md)
passes real-Core38/27 processes,600 concurrent commits/1,200 nonoverlapping lock
events, Qt backend/model24, matching popup7/313 and geometry4/194. Both ordinary
two-window closing orders preserve unrelated saved edits and independent paused
media, with one fullscreen window. Core `3a7b0a360e3c…`, ordinary GUI
`371bc858e3bb…`; matching private GUI and source identities are in the receipt.
No unexpected native/Qt errors or forced cleanup. Shared Qt profile/geometry
ownership, simultaneous playback/two-output routing, remaining consumers,
visual/accessibility/OS-DPI and final-package acceptance stay open. Other settings
consumer regression adds General11, Audio32, Network17 and profile-defaults13
(73 additional cases;97 selected model cases total). An ordinary simultaneous
playing run showed independent pause/seek/queue/volume but was rejected for
shutdown-time PCR errors and clipboard warnings. Two90s no-GUI real-output
comparisons and one undisturbed ordinary Stop-before-Quit comparison are clean.
The [application Quit repair](native/home-quit-stop-20261021/README.md)
passes ordinary audio/audio and movie-plus-music pairs (four processes): direct
fullscreen Quit leaves the other playing, then its direct Quit is clean too.
All preserve normal window bounds. Four earlier private audio cases and three
corrected private video/combined-media cases pass. Rapid-start cancellation
diagnostics remained open on that tuple; GUI `d148035d54be…`, Core R8 unchanged. Only one explicit audio endpoint is available; Default aliases it.
The complete beta goal remains active.

The earlier [whole-row popup focus repair](native/home-popup-duration-activation-20261020/focus-repair-r4/README.md)
passes its distinct native7/313, ordinary1 and component12 scopes after the old-
source negative control. Title, slider, scale and full note remain visible.
Its old Core/GUI identities are historical; the new matching replay is above.
Native narrow/Light, title alignment and actual OS-DPI remain open.

Use with the [revised roadmap](../juxtopposed-beta-roadmap.md). This reconciles
the 32 surfaces in the earlier plan with published checkpoint `ed2f5d0155` and
its scoped receipts. The original filename is retained for existing links.
It is a planning snapshot, not a new runtime acceptance receipt.

The later [geometry activation](native/home-geometry-activation-20261020/README.md)
passes selected native4/194 and three ordinary sessions, including actual editor,
Off-save/cold startup policy and active-media preservation. The
[popup activation](native/home-popup-duration-activation-20261020/README.md)
passes native7/283 and three ordinary sessions with real Windows duration hints;
a subsequent actual-capture review identified whole-row keyboard focus clipping.
Its component and matching R4 Windows scopes pass as recorded above. Historical
baseline gated snapshots retain their own identities. The
[multi-instance reproduction](native/home-multi-instance-20261020/README.md)
retains the original P1 lost-update failure; the newer repair above passes both
ordinary close orders without accepting the broader multi-instance scenario.
All 32 surfaces retain separate complete visual, accessibility, OS-DPI and package
gates. No final beta acceptance is implied.

**Implementation** below means code/foundation is present, not that every
designed action is complete. **Native evidence** distinguishes backend fixtures
from actual app journeys. **Visual** is open for complete surface acceptance;
specific original artwork/geometry checks are identified separately.
**Final package verification is open for all 32 rows.** Historical receipts
retain their own source, binary and platform identities.

| ID | Surface | Implementation / native evidence already available | Remaining implementation or verification | Complete visual acceptance |
| --- | --- | --- | --- | --- |
| 01 | Player transport and volume | Native controls and scoped actual video/audio, pause/seek, volume/mute journeys; R4 fullscreen audio/video On/Off138 verifies hidden-input rejection, focus and paused state. Fullscreen setting is enabled with actual editor/restart proof. Ordinary mouse-focused Left/Right use the saved volume step without seeking. Auto-hide/delay are enabled with actual12/666 and ordinary cold/hover proof across the named mode/docking/playback states; complete visual/package status stays separate. | Current-tuple tracks, media changes, remaining pinned/floating/fullscreen/representative media and video-window warning review | Open |
| 02 | Main timeline and preview | Native seek, timer and guest live-clock repairs; ordinary HLS replays | Marks/preview, unknown/long duration, live limitations and source-switch behavior | Open |
| 03 | Queue | Real playlist adapters; component and earlier integration evidence | Full actual filter/selection/reorder/drop/remove, state retention and detach/dock lifecycle | Open |
| 04 | Bookmarks | Native backend persistence/seek fixture included in the nine-case bookmark/chapter proof | Actual panel add/rename/remove/seek, indexing readiness, rapid media changes and unavailable-library behavior | Open |
| 05 | Chapters | Native chapter backend and generated fixture | Actual panel/current chapter/timeline marks, keyboard and empty-state journeys | Open |
| 06 | Audio/subtitle tracks | Existing native track controllers and panel foundation | Multiple tracks, selected state, external subtitles, source changes, dismissal and exact reference mapping | Open |
| 07 | Subtitle appearance/synchronization | Native style/duration/delay and renderer repairs; earlier app journeys | Final-current menu/default/language/persistence, renderer/format variants and source changes | Open |
| 08 | Playback panel | Existing four-tab panel and native speed/controller evidence; Playback/Audio console R18 covers actual native track/rendition/startup/repeat/shuffle/stop/finish policies in its scoped cases | Actual editor/action journeys, remaining consumers, numeric/slider synchronization and advertised limits; constructor-only time-shift proof does not establish live disk behavior | Open |
| 09 | Video defaults/effects | Native preference page, rendered transforms and scoped app default/restart proof | Audit remaining consumers, hardware/non-square/current-media behavior and persistence | Open |
| 10 | Audio equalizer/effects | Native preference page, real PCM/EQ, scoped app restart proof, Windows endpoint/startup-volume/step and next-input delay proof. Authored step has R4 settings265/ordinary Left/Right scope. Later device/fixed-startup activation has actual five-process290/component20 and ordinary Windows typing/device/policy/reopen proof; receiver stays gated. | Passthrough/physical device loss, remaining confirmed effects/consumers, formats and complete panel journeys | Open |
| 11 | Shared navigation | Ordered typed routes/pins and Library-OFF Recent; focus R2 native compact video741/audio742, recovery321, Home ON/OFF188 each; keyboard-return rings pass in native captures and direct ordinary Windows input | Remaining whole-app accessibility/OS-DPI and final-package selection/input coverage | Open; original Videos/Music bytes and slot geometry pass |
| 12 | Shared title/window strip | Original 40-pixel strip/assets; actual Windows system drag/double-click, compact and close checks | Actual DPI transitions, multi-monitor reachability, Windows 11 Snap and variants | Open; scoped original geometry/assets pass |
| 13 | Library header | Native history/search/sort/view adapters and scoped app journeys | Cross-category selection/history, long text, rapid filters and compact focus | Open |
| 14 | Home/Add Folder | All five sections have native app content/resume/privacy/restart proof; preview bounds repaired | Moved/new-path/parent-folder identity/history, shell-native picker, broader rapid UI and states | Open |
| 15 | Sidebar pins | Ordered media/folder/playlist/artist/source activation, persistence/retry and identity guards | Remaining presentation, reorder/input/error paths and variants on final tuple | Open |
| 16 | Shared playlists/content | Real mixed collections; native creation/order and app content/cover/restart evidence; explicit playlist origin with actual Library On/Off67, shared-media names, selection, rename/deletion and paused-state preservation | Complete selection/search/edit/remove/drop behavior and remaining primary-player independence/package coverage | Open |
| 17 | Video library | Native indexing/metadata/search/activation plus app routes | Referenced group/folder/grid/list variants, pointer/drag and return/resume | Open |
| 18 | Music category landing | Songs/Albums/Artists/Genres routes exist; scoped component/native entry evidence | Complete actual transitions, counts/search/sort and native visual review | Open |
| 19 | Music collection/detail pages | Real song/album/artist/genre models and some native content journeys | Audit every confirmed detail layout, drill-down, actions and play/return | Open |
| 20 | Browse/local sources | Native source browser and corrected empty-artwork handling | Traversal, identity/history, unavailable sources and complete opening/return states | Open |
| 21 | Open/source menu | Existing native providers and mounted routes | Reconcile original labels/icons/shortcuts and exercise each designed route; avoid inventing unseen forms | Open |
| 22 | Open Network Stream | Native ordered URL/playlist handling, earlier actual Yle video and modal foundation | Current actual Save/Cancel/errors/retry, header drag, protocol limits and active-player behavior | Open |
| 23 | New Playlist | Native create/persistence plus actual picker/ordered mixed save/cover proof; source is 713 × 589 | Actual drop/empty/cancel/partial-save/retry, active-primary preservation and current visual states | Open |
| 24 | Conversion/export | Real controller, exact-PCM/output-path and later encoder preference proofs | Supported-profile/progress/cancel/EOS/failure lifecycle, full output and source/primary preservation; original form remains unconfirmed | Open; functional adaptation is labelled |
| 25 | Subtitle scope | Current-file subtitle opening exists; complete folder/playlist association support is not established by the cited receipts | Reconcile actual file/folder/playlist support and persistence; test scope/source changes against original design | Open |
| 26 | Subtitle presets | Native persistence plus actual paused Save/edit/retry/restart/Default proof | Playing/source-switch states and final-current menu/focus/format coverage | Open |
| 27 | Subtitle cue timeline | Selected-local UTF-8 SRT snapshot service and native controller proof | Authoritative embedded/renderer timing, original navigation/zoom/drag semantics and app lifecycle; text editing/export unconfirmed | Open |
| 28 | Music player/waveform | Native decoded peaks, source-folder/metadata, actual seek/return/media replacement | Artwork blur, missing art, formats/tracks, narrow/DPI and confirmed lyrics/settings actions | Open |
| 29 | Global Settings | All 13 native bodies mounted and component scopes pass; scoped playing/paused/restart/checked-store proofs; new Playback/Audio and fullscreen consumers have bounded native/app evidence | [Per-field consumer audit](../juxtopposed-settings-consumer-audit.md), staged activation of verified gates, actual editors/reset/error/retry and final-current regression | Open |
| 30 | Appearance/interface | Real theme/accent persistence; native Light contrast/accessibility repairs | Actual System notifications, font/accent states, OS DPI, blur and reachability | Open |
| 31 | First-run setup | Confirmed two-page theme/accent/shortcut flow implemented with draft-only preview and checked saving. Private R4 Cancel/Finish/retry/fresh-process reopen pass (49/60/18); ordinary R2 Next/Finish and R3 completed-profile reopen pass through actual Windows input. Three native visual defects repaired; [receipt](native/home-setup-20261019/verification.json) | Actual new-user consent widget, Windows DPI and complete visual/package states | Open |
| 32 | Profiles/customizer/shortcuts | Profiles79/79/15 and customizer111/15/111; layout11, host115, transport persistence71; [customizer receipts](native/home-customizer-20261019/README.md). Favorite183 covers native state/source races, persistence and real write failure/retry. Ordinary GUIfd7f8af9b480 verifies pointer/Space/Return with independent database readback; [current-media receipts](native/home-current-media-20261019/README.md). Authoritative playlist labels also pass actual app checks | Finish HDR/other confirmed tile actions, shortcut consumers and full visual/accessibility/OS-DPI/reset/conflict/package journeys. Screen-reader focus remains unaccepted; monitor-edge tooltip placement needs follow-up | Open |

## Global Settings field audit

The current QML host lists these 13 categories, each with an existing typed
preference service and mounted body. The newer settings-quality checkpoint
exercised all categories; older claims that eight pages are unmounted are stale.
This does not prove every confirmed field has a working consumer.

| Group | Categories | Remaining audit |
| --- | --- | --- |
| App | General, Interface, Privacy, Controls | Defaults/ranges, real OS/live consumers, configured versus applied state, checked persistence/reset and complete shortcuts/input behavior |
| Media | Playback, Video, Audio, Subtitles, Library | Actual media consumers, delay/time units, current-versus-next semantics, supported formats, inheritance, indexing/history/privacy and reset without resurrection |
| System | Network, Performance, Recording, Extensions | Actual networking/proxy/TLS, plugin/encoder consumers, explicit scheduling refusal, native recording behavior, install/remove/save retry and advertised hardware/service limits |

For each original field record label/reference, native owner, units/default,
availability condition, consumer, when applied, restart requirement, error/retry
and reset behavior. Derive gaps from the current source/runtime; do not copy old
unsupported-field counts forward without checking the later consumer work.

## Evidence sources and unresolved release checks

- [Current settings activation R4](native/home-settings-activation-20261019/README.md):
  settings265, routing12 and fullscreen138 pass normally; direct ordinary Windows
  numeric editing and mouse-focused volume Left/Right pass without seeking.
  Ordinary GUI `758df308cca3…`, private `f7e7d95db75e…`, local `source-r4`.
  The tuple includes the [exact FLAC-tail repair](native/home-flac-coded-tail-20261019/README.md).
  Tooltip/video warning, broader workflows and visual/DPI/package work remain open.
- [Title-strip tooltip R7](native/home-titlebar-tooltip-20261019/README.md):
  actual39 and ordinary maximized keyboard/hover help pass; original geometry12
  and paired duplicate-help regression pass separately. Tooltip-scope ordinary GUI
  `9892032f4456…`, private `a14c5d7c7cb4…`, local `source-r7`.
  Copied-startup cropping is traced to an off-screen owned fixture; fresh in-bounds
  R7b coordinate Restore/Maximize passes. Full geometry/DPI remains open; the
  video-warning receipt below has its own scope. This does not replay every R4
  result or accept the final package.
- [Ordinary R7 video-warning review](native/home-video-warning-20261019/README.md)
  observes actual generated H264 color frames in paused/playing/fullscreen/windowed
  states, later nonzero Direct3D11 sizes, normal exit and no Qt/native errors.
  The initial fallback is accepted only in this tested scope; small-window resize,
  all-format/GPU/DPI and final-package proof remain separate.
- [Windows Audio activation](native/home-audio-device-activation-20261019/README.md):
  actual stale31/paused83/reopen59/playing95/unmanaged22 pass; component20 and
  ordinary Windows typing55%, device/policy editing and reopen pass. Five normal
  native exits, no Qt diagnostics/unexpected native errors; three intentional
  negative errors retained. Cold real output uses saved endpoint65%. Current
  Audio-scope ordinary `3857fa66519c…`, private `de7506005fca…`, source-r1;
  Core/FLAC/audio/library identities unchanged. Receiver/unplug, complete visual/
  DPI and final-package gates stay separate; hardware labels remain private.
- [Interface title activation](native/home-interface-title-20261019/README.md):
  actual initial52/Off45/restored46 =143 and two ordinary Windows sessions pass
  actual editor/retry/restart, cold audio/video captions and same-instance-next
  policy. Current ordinary `d40f11aeefdf…`, private `4e09011ce1ee…`, source-r1;
  protected modules unchanged. Later auto-hide proof below supersedes two gates;
  prior GUI receipts keep their scopes. No full visual/DPI/package acceptance.
- [Auto-hide activation](native/home-auto-hide-20261019/README.md): actual12/666,
  normal exits/no Qt or unexpected native errors, real mode/delay editor and
  retry/reopen, all modes, paused/playing audio/video, docked/floating and actual
  zero/ten-second boundaries. Pointer/hover paired repairs and doubled keyboard
  timers preserve primary input/queue/tracks/clock. Component9/43/47 and ordinary
  cold hide/reveal/hover have separate scopes. Current ordinary `598173d1ba52…`,
  private `dc2456a80a6c…`, source-r9; protected modules unchanged. The later
  screen-selector activation below closes another gate. All32 surfaces and their
  remaining workflow/visual/accessibility/DPI/package requirements remain open.
- [Fullscreen screen activation](native/home-fullscreen-screen-20261019/README.md):
  actual5/399 and two ordinary Windows sessions pass editor/retry/reopen, cold/
  stale values, genuine screen choices and MainCtx/window-handler targeting.
  Hidden cross-screen selection/restoration uses the real app window; visible
  presentation stays on monitor2. Primary/queue/tracks/clocks are preserved;
  normal exits, no Qt/unexpected native errors. Current ordinary `56a4a7620f15…`,
  private `69af3dfee412…`, source-r3; protected modules unchanged. Geometry and
  popup duration remain gated; full visual/accessibility/DPI/package stay open.
- Historical [published-checkpoint GUI provenance](native/home-fullscreen-controls-20261019/actual-app-provenance.json):
  ordinary `fd7f8af9b480…`, private `8be8eff6e4d8…`, local `source-r6`.
  Fullscreen138 and [playlist-origin67](native/home-playlist-context-20261019/README.md)
  pass actual app journeys with normal exits and no Qt/native errors. The
  [ordinary Favorite replay](native/home-current-media-20261019/ordinary-direct-input-replay-20261019.json)
  passes direct Windows input on the same ordinary GUI. These scopes do not
  replay every older result or establish final-package/whole-app acceptance.
- [Playback/Audio consumers](native/home-playback-consumers-20261019/README.md),
  [Windows Audio](native/home-audio-consumers-20261019/README.md) and the current
  [field audit](../juxtopposed-settings-consumer-audit.md) distinguish verified
  consumers, remaining gates, absent behavior and actual-desktop gaps.
- [Home/current cache/audio tuple](native/home-cache-audio-20261018/README.md)
  and [exact receipt](native/home-cache-audio-20261018/verification.json).
- [Home interaction checkpoint](native/home-compact-dialogs-20261018/README.md):
  R4 twelve modal cases pass; R10 six native processes cover compact video/audio,
  error recovery with visibility, Home menus with Library on/off, and playing
  conversion. Five ordinary R10 playback smokes pass. The user confirmed physical
  Shift+F10 **fails** in historical R10 after right-click/Escape. Its repair now
  passes direct Windows input in ordinary setup R2, with Menu/F10 and an off-pin
  negative; see the [repair receipt](native/home-compact-dialogs-20261018/native-keyboard-repair.json).
  A new user physical retest and remaining visual/accessibility/DPI/package
  acceptance stay open.
- [Home/resume](native/guest-home-content-resume-20261018/README.md),
  [pins](native/guest-sidebar-pins-20261018/README.md),
  [Recent without Library](native/guest-recent-history-only-20261018/README.md),
  [shared title strip](native/guest-shared-strip-20261018/README.md).
- [Settings quality](native/guest-beta-settings-quality-20261017/README.md),
  [subtitle presets](native/guest-subtitle-presets-ui-20261017/README.md),
  [movable-modal component/native-build scope](native/guest-popup-20261017/README.md).
- [Cached original Figma inventory](figma/20261014/README.md), the earlier
  [coverage ledger](coverage-ledger.md), and historical
  [beta requirements](../juxtopposed-beta-plan.md#every-known-design-surface).
  Their older row statuses must be interpreted with the newer receipts above.
- [Setup/customizer reconciliation](setup-customization-20261018.md) identifies
  the confirmed second onboarding page, the 31-tile customizer and alternative
  layout, profile scope and the shortcut-reference inconsistency using cached
  originals and the local video; it is not new implementation acceptance.

Windows 10 comparison is explicitly deferred by the user; its historical AUDIOSES
crash remains open. The kit exists and passed Windows 11 self-checks only. No full
final-package, whole-app visual, actual OS-DPI/System-notification, complete
first-run/profile or physical device-loss acceptance is established by this
snapshot. Native media warnings and monitor-edge tooltip placement remain open
for targeted review. The old human firewall question was answered; it is not a
current blocker. Any new permission dialog still requires its own handling.
Continue implementation and Windows 11 validation when the goal is resumed.
