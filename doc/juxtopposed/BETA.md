# Beta roadmap

This is the single definition of beta scope, work order and readiness. Read
[STATUS](STATUS.md) for the current checkpoint and next concrete action;
[the project guide](README.md) explains how to resume and pause sessions.

## Objective and release boundaries

Deliver Juxtopposed's confirmed VLC redesign as a coherent, reviewed Windows x64
beta candidate: real media behavior, persistence, usable interaction, faithful
presentation, stability and a reproducible ordinary package. Complete every
confirmed action in the surfaces below, with explicit treatment of unsupported
platform/hardware behavior and reference gaps.

| Release | Agreed scope |
| --- | --- |
| Alpha (current development stage) | Working implementation with incomplete features and graphical acceptance; no claim that the full design vertical slice or beta gates are complete. |
| Graphical alpha (next milestone) | Every confirmed design surface and important visual state is reachable and coherent. Explicit development previews may stand in for unfinished functions; they do not satisfy beta behavior or stability gates. |
| Beta | Confirmed Juxtopposed design and stability, including the native work needed to make its actions real. |
| V1.0 | Broader vanilla VLC parity, including the disc/ISO/Explorer/AutoPlay placement discussion with the designer. |
| V1.5 | Possible additions such as YouTube/Twitch integrations; these are ideas, not beta commitments. |

Preserve working native routes and traditional-controls opt-out. A confirmed
menu entry must function; an unshown dialog can use a clearly documented practical
adaptation. Conversion/export and profile-naming forms have incomplete original
references. Do not invent a fidelity claim for them or assume subtitle text
editing/export from an ambiguous reference. Record unresolved reference questions.
Installer work, package-size optimization and optional Code Connect integration
are secondary to the tested beta. Designer contact and publication are separate
user-authorized actions.

The home candidate targets Windows 11 validation. The original Windows 10
AUDIOSES comparison is an explicitly deferred external check. Retain the kit in
`build-windows/beta-native-cache-home-20261018`; run it when that machine is
available. Windows 11 results cannot close the original exception or establish
Windows 10 compatibility. A release claiming both platforms requires that result
or an explicit user decision changing the advertised support scope.

## Next milestone: graphical alpha

Owner decision, 30 October 2026: complete the graphical implementation before
the remaining nonblocking functional depth. Begin with a focused code review,
record its findings, then pause for the owner's choice of the next work session.
The review is preparatory work; it does not itself accept graphical alpha or G4.

The owner resumed on 30 October with the bounded next goal: repair the review's
two saved-state issues, inventory all 32 surfaces, and use that inventory to
choose visual work. Preserve the review as historical evidence; qualify repairs
separately. Completing this inventory does not complete graphical alpha or beta.

The 30 October inventory below maps all 32 surfaces against cached Figma exports,
the original video, a targeted Figma MCP shortcut-editor check and ordinary app
captures. Continue with the selected visual work below. Maintain these assessments
here, separating the following facts rather than collapsing them into one score:

| Measure | Evidence required |
| --- | --- |
| Graphical coverage | The surface and confirmed variants exist and are reachable in the identified application or explicitly identified development preview. |
| Reference fidelity | Matched app/reference captures, named assessed criteria, remaining mismatches and explicit reference gaps or functional adaptations. |
| Functional completion | Which actions use real services, which remain unavailable or mocked, and their outstanding native/ordinary journeys. |
| Beta verification | Current-tuple interaction, stability, accessibility, OS-DPI and package evidence; the existing surface checkbox retains this full meaning. |

Graphical alpha requires all of the following; only the coverage audit is complete:

- [x] **GA1 Coverage audit:** all S01-S32 have an identified reference or explicit
  reference gap, current graphical/functional assessment and linked evidence.
- [ ] **GA2 Reachable presentation:** every confirmed screen, panel, dialog and
  important variant can be inspected in a coherent walkthrough. Complete missing
  layouts while retaining already working services and traditional-controls opt-out.
- [ ] **GA3 Visual comparison:** ordinary rendered screens are compared at matched
  sizes with original references; meet the shared canonical/variant visual targets
  for assessed states and disclose unassessed criteria. Include representative
  empty, populated, selected, disabled, loading, error and retry presentations,
  normal/narrow/short windows and Dark/Light/System variants where applicable.
- [ ] **GA4 Honest previews:** any necessary mock data or unfinished action is
  identifiable in a separate development preview and recorded against its surface.
  Never replace a working consumer with a mock or simulate a successful save,
  conversion or other real-world action in the ordinary candidate.
- [ ] **GA5 Coherent interaction:** navigation, opening/closing, scrolling, focus
  return and basic resizing permit the walkthrough without crashes, state loss or
  blocking layout defects. Existing playback remains usable during UI inspection.
- [ ] **GA6 Reviewable checkpoint:** retain the identified source/runtime, captures,
  walkthrough and outstanding functional/verification list; let the owner assess
  the complete presentation before returning to deeper functional completion.

Fix crashes, data-loss defects and failures that prevent this walkthrough before
accepting it. Queue other native/format work explicitly for W1/W2 rather than
letting one backend area indefinitely postpone whole-interface coverage. Preserve
all completed regression evidence and every unfinished beta requirement.

Graphical alpha is an internal development checkpoint. Broader Windows DPI,
accessibility, monitor/media combinations and clean-package qualification remain
mandatory beta work; no public beta readiness follows from a visual walkthrough.

## Ordered work packages

Keep moving through these packages during an authorized beta session. Select
work using the latest evidence; repair a new failure before accepting its path.
Preserve already working implementations instead of repeating completed work
because an archived TODO says otherwise.

| Order | Package | Completion evidence |
| --- | --- | --- |
| R0 (completed) | Focused code review and saved-state follow-up | [Frozen review](../juxtopposed-references/reviews/code-review-20261030.md); [R1/R2 repairs](../juxtopposed-references/native/saved-state-20261030/README.md) pass scoped native and ordinary recovery checks. R3 remains open. Final independent review is still required. |
| GA (next implementation milestone) | Complete graphical alpha | GA1 is recorded below; finish GA2-GA6, starting with shared theme/contrast and walkthrough warnings, then the missing source/effects/shortcut presentations. Preserve real consumers and retain other functional requirements. |
| W1 | Finish subtitle timing and immediate regressions | Qualify ordinary cross-editor FPS Reset and seek/playing/default/restart behavior, secondary/per-track timing and estimators; resolve startup failures found by their regressions. Preserve the caption-selection, paused-frame, delay and accessibility repairs. |
| W2 | Complete native consumers and designed workflows | Resolve the [settings audit](SETTINGS.md) and remaining surface actions below: customizer/shortcuts, library/detail, queue/bookmarks/chapters/tracks, subtitle scope/presets/timeline and conversion. Exercise live/default/restart timing, reset, cancel, failure/retry and persistence. |
| W3 | Finish interaction, design and accessibility | Compare actual app captures with original assets/frames, cover all 32 surfaces and risky states, close focus/scroll/drop/modal/busy-state defects, verify theme and Windows DPI/monitor behavior. True top-level popup windows must be movable. |
| W4 | Consolidate media and instance regressions | Replay representative real/generated media, paused/playing independence, source changes, restart, missing media, failure/retry, background-worker shutdown and the multi-instance scenarios below on an identified current tuple. Preserve strict output/PCM checks. |
| W5 | Review and qualify the clean candidate | Independently review the final changes and actual design captures; assemble a matched ordinary portable package from fixed source, then verify fresh startup, relocation, coexistence, features/limitations, licenses, attribution, reproducible build notes and checksums. |
| Separate lane | Windows 10 comparison | Run the prepared old/new comparison on the original guest environment, recording OS/audio versions and exception context if it recurs. Keep this external deferral visible. |

The W identifiers retain their existing scope; GA now precedes the remaining
nonblocking W1/W2 work. W3 supplies the broader interaction/accessibility/OS-DPI
acceptance after graphical alpha and functional integration. W5's final independent
review remains required even after the preparatory R0 review.

These are checkpoints within the same beta objective. A session can stop at any
saved checkpoint when the user asks or its requested time ends. The next resume
continues the first applicable unfinished work; it does not restart the project.

## The 32 designed surfaces

Every checkbox means complete beta acceptance for that surface. They remain open
while full journeys, design/state variants or final-package checks are missing,
even where substantial scoped implementation and test evidence already exists.
Maintain implementation, native/desktop verification, visual acceptance and
final-package verification as separate facts in the linked evidence/audit.

The [original coverage ledger](../juxtopposed-references/coverage-ledger.md)
preserves node/time/dimension provenance. Its early progress labels are historical.
Cached Figma exports, the original video and exact-time manifests are the design
sources; editorial zooms and classic-VLC comparison shots do not define new UI.

| ID | Complete surface | Required behavior |
| --- | --- | --- |
| S01 | [ ] Player transport and volume | Previous/play/pause/next, mute/volume and duration/remaining time; correct availability, pointer/keyboard/wheel actions and native feedback. |
| S02 | [ ] Timeline and preview | Correct clock/seeking for paused/playing/unknown/long media; drag, precise seek, real thumbnails and chapter/bookmark marks without stale-source results. |
| S03 | [ ] Queue | Real play/filter/select/reorder/remove/drop/add, jump to playing, repeat/shuffle/sort and selected save/stream actions; retention in attached/detached/library routes. |
| S04 | [ ] Bookmarks | Add/seek/rename/remove/sort for the correct media; seek/library availability, rapid source changes, restart and accurate accessible state. |
| S05 | [ ] Chapters | Real chapter selection/current feedback, timeline marks, keyboard actions and clear no-chapter state. |
| S06 | [ ] Track panels | Audio and primary/secondary subtitle selection, external open, disabled/empty states, focus/dismissal and source-change safety. |
| S07 | [ ] Subtitle appearance and synchronization | Confirmed style/size/color/outline/background/offset/duration/delay/FPS controls, default track/language and native feedback across supported embedded/external formats. |
| S08 | [ ] Playback settings | Complete four-tab navigation and confirmed rate, sleep, metadata and other designed actions; correct native state and persistence semantics. Clarify reverse where referenced. |
| S09 | [ ] Video defaults and effects | Flip/rotation/zoom/aspect/crop/deinterlace and confirmed HDR/resolution/effects fields; actual output, media changes, hardware/format limits and reset. |
| S10 | [ ] Audio equalizer and effects | Native stereo/mix/delay, preamp/bands/presets/two-pass and confirmed effects; real PCM, current/default ownership and desktop interaction. |
| S11 | [ ] Shared navigation | Correct original shell, routes, icons, expanded/collapsed selection, scrolling, library/history policy and retained paused/playing state. |
| S12 | [ ] Shared title/window strip | Home/menu/title/window controls, panel coexistence, minimize/maximize/fullscreen/close, focus and native window behavior. |
| S13 | [ ] Library header | Back/Forward, breadcrumbs/count, sort/view/search/menu and correct selection under filtering in every category. |
| S14 | [ ] Home and Add Folder | Retain the confirmed sections; make Continue Watching prominent for unfinished videos resuming their saved position, with a separate secondary Watch Again section for completed videos starting from the beginning (owner decision, 28 October). Both lists need Clear all items and individual Remove from this list actions on Home and full-list routes; clear the selected history section across search/paging, preserve the other section and media files, and prevent stale active-input writes from restoring removed entries. Qualify completion classification, counts/order/search/events, picker/indexing/history, missing/moved/new roots, privacy/reset/opt-out, restart and retained player. |
| S15 | [ ] Sidebar pins | Ordered stable media/folder/playlist/artist/source descriptors; activate/reorder/unpin, keyboard/context actions, errors and restart. Preserve completed pin work. |
| S16 | [ ] Shared playlists and contents | Mixed-media collections, genuine metadata, creation/edit/order/removal/open, search/sort/selection and player independence. |
| S17 | [ ] Video library | Referenced grid/list/folder/group/player variants with native thumbnails, metadata, filtering, selection/drag, play/return/resume. |
| S18 | [ ] Music category landing | Add/Songs/Albums/Artists/Genres routes, real counts, sort/view/search and correct assets/navigation. |
| S19 | [ ] Songs/Albums/Artists/Genres details | Complete referenced collection/detail views, native drill-down/selection and playback/return; an initial compact Songs view is partial coverage. |
| S20 | [ ] Browse and local sources | Real source/folder traversal and activation, history/return, unavailable/moved sources and stable identity. |
| S21 | [ ] Open/source menu | Confirmed entries/icons/shortcuts and working native file/multiple/folder/disc/network/capture/clipboard/recent, playlist, conversion/stream, customization/settings/about routes. Unshown vanilla-only forms stay outside beta design scope. |
| S22 | [ ] Open Network Stream | Ordered URL validation/append/play, optional named mixed playlist, useful failure/retry/cancel/focus and actual advertised protocol behavior. |
| S23 | [ ] New Playlist | Original layout, picker/drop, ordered/repeated mixed files, name/optional copied thumbnail, busy/partial-save/error/cancel, no duplicates and native cover/content restart. |
| S24 | [ ] Conversion/export | Confirmed source/profile/destination/progress/cancel/error actions, playable complete output, safe destination publication, worker teardown and primary-player independence. Label the functional form adaptation. |
| S25 | [ ] Subtitle attachment scope | This File/Folder/Playlist persisted association, precedence/default selection, invalid/moved sources and restart. Appearance presets alone do not establish subtitle-file scope. |
| S26 | [ ] Subtitle presets | New Custom/Default/saved/edited state, real save/apply/retry/restart and correct track/timing restoration. Clarify unshown naming UI. |
| S27 | [ ] Subtitle cue timeline | Real cues and supported timing interaction, guarded seek/source revisions and confirmed zoom/navigation; distinguish parsed SRT snapshots from decoder/renderer truth and resolve advertised format limits. |
| S28 | [ ] Music player and waveform | Real artwork/metadata/source folder, blur, waveform seek/elapsed/future bars, queue fallback and confirmed lyrics/metadata/settings actions. |
| S29 | [ ] Global Settings shell | Searchable App/Media/System navigation, shared controls/help, checked save/reset/failure/retry and Advanced Options for all thirteen categories. |
| S30 | [ ] Appearance/interface settings | Language, Dark/Light/System/accent, actual system-theme propagation, shared typography/assets, persistence and restart behavior. |
| S31 | [ ] First-run setup | Complete confirmed steps, theme cards/preview/accent/Back/Next and genuine fresh-profile completion/persistence. |
| S32 | [ ] Profiles/customization/shortcuts | Movies/Shows/Music/Work/New, real profile scope and creation/naming, toolbar/tile/shortcut actions and conflicts, save/switch/reset/restart/error handling. |

## Graphical inventory — 30 October 2026

The [dated receipt and captures](../juxtopposed-references/reviews/graphical-inventory-20261030/README.md)
and [source/binary inventory](../juxtopposed-references/reviews/graphical-inventory-20261030/inventory.json)
support the assessments below. **All 32 surface families are mapped; the complete
graphical implementation is not finished.** Forty-three ordinary observations
cover representative states. Some confirmed designs still use traditional native
forms, and complete collection/track layouts have explicit reference gaps.

“Present” means a reachable presentation exists. It does not accept every variant,
the 9.5/9 visual targets, all functions, accessibility, OS-DPI or a release package.
No whole-app percentage or numeric fidelity score is inferred. GA2-GA6 and G1-G6
remain open. The existing S01-S32 checkboxes retain their full beta meaning.

| ID | Reference / gap | Current graphical coverage and fidelity | Real functions and remaining behavior | Current evidence; beta verification |
| --- | --- | --- | --- | --- |
| S01 | [Player 187:328; video around 01:30](../juxtopposed-references/figma/20261014/manifest.json) | **Present.** Authored controls/assets; full matched score and narrow states open. | Native player wired; this run held generated video/music; full transport matrix pending. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/s01-s02-s12-video-light-0.png); all beta acceptance remains open. |
| S02 | [3744:12361; thumbnail frame 20261022](../juxtopposed-references/figma/20261014/timeline.png) | **Present, variants unassessed.** Timeline rendered; hover preview and marker fidelity not requalified here. | Real clock/seek/thumbnail workers; long/unknown input and source-change journeys remain. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/s01-s02-s12-video-light-0.png); all beta acceptance remains open. |
| S03 | [463:831, 400x946](../juxtopposed-references/queue/queue-463-831.png) | **Present, theme gap.** Light tab strip over fixed dark body; inconsistent text contrast. | Real five-item mixed queue shown; full edit/drop/detach/sort journeys not rerun. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/s03-queue-light-0.png); all beta acceptance remains open. |
| S04 | [6320:13784](../juxtopposed-references/queue/bookmarks-6320-13784.png) | **Present, theme gap.** Light empty label and bookmark name are low contrast on dark body. | Owned bookmark added at 21 ms; full rename/remove/seek/restart matrix remains. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/s04-bookmarks-populated-light-0.png); all beta acceptance remains open. |
| S05 | [6321:14045](../juxtopposed-references/queue/chapters-6321-14045.png) | **Present, theme gap.** Three real named chapters shown; shared Light body inconsistency. | Native chapter list/current feedback present; selection/keyboard matrix remains. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/s05-chapters-light-0.png); all beta acceptance remains open. |
| S06 | [Track-selector exact geometry unresolved; 06:06 is attachment scope](../juxtopposed-references/advanced/0366-subtitle-scope.png) | **Native presentation; reference gap.** Existing full-width track popup; do not invent an exact design match from attachment frames. | Two embedded audio/subtitle tracks visible; secondary/default/source-change matrix remains. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/s06-track-panels-light-0.png); all beta acceptance remains open. |
| S07 | [07:50 editorial reference](../juxtopposed-references/settings/subtitle-reference-07m50.png) | **Present, fallback tools.** Style/timing form exists; synchronization tools still open the traditional dialog. | Real models/consumers retained; format/role/FPS/default cases keep their scoped receipts. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/s07-subtitle-controls-light-0.png); all beta acceptance remains open. |
| S08 | [09:10 speed-only tab](../juxtopposed-references/settings/playback-reference-09m10.png) | **Present.** Four tabs/speed agree with inspected reference; sleep/reverse placement remains unresolved. | Real rate control; current inventory does not qualify every narrated feature. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/s08-playback-panel-light-0.png); all beta acceptance remains open. |
| S09 | [536:1855 Essential; 543:990 Crop; 614:1430 Colors](../juxtopposed-references/figma/20261014/manifest.json) | **Partial.** Basic panel exists; dedicated redesigned effects missing. More video effects opens traditional Audio tab. | Native transform/default services retained; complete effects/hardware journeys remain. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/s09-s10-more-effects-traditional-audio-0.png); all beta acceptance remains open. |
| S10 | [6114:13610 EQ; 536:1513 Compressor; 536:1703 Spatializer; 612:1223 Stereo Widener](../juxtopposed-references/settings/equalizer-reference-08m50.png) | **Partial.** EQ/sync panel exists; more effects route remains traditional. | Native EQ/preamp/bands/presets retained; unavailable mix shown honestly; remaining effects/PCM scope open. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/s10-audio-controls-light-0.png); all beta acceptance remains open. |
| S11 | [6261:15078; 217px expanded sidebar](../juxtopposed-references/figma/20261014/home.png) | **Present.** Authored rail/icons/routes/pins present; complete collapsed/focus/scroll variants unscored. | Actual category/pin routes used with retained paused media; storage conflict/Retry/cold persistence passes. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/saved-pins-cold-home-0.png); all beta acceptance remains open. |
| S12 | [Player/Home frames; 40px title strip](../juxtopposed-references/figma/20261014/manifest.json) | **Present.** Authored Home/menu/source/window controls; broad fullscreen/DPI/topology still open. | Home/maximize/normal closes used; source context for video still returns library instead of folder pane. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/s01-s02-s12-video-light-0.png); all beta acceptance remains open. |
| S13 | [6261:15078; video 11:45/12:10](../juxtopposed-references/figma/20261014/home.png) | **Present, mixed consumers.** Shared breadcrumbs/count/view/search present; local browser still has traditional header. | Real counts and grid/list switch observed; all-category filtering/selection matrix open. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/s17-video-list-0.png); all beta acceptance remains open. |
| S14 | [13:56; Home 6261:15078](../juxtopposed-references/home/836.png) | **Present, owner adaptation.** Six sections include owner-requested secondary Watch Again; keep that intentional addition. | Generated root indexed: two videos/four music tracks; history content observed. Full clear/failure/privacy/close-load matrix remains. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/s14-home-history-light-0.png); all beta acceptance remains open. |
| S15 | [13:42; heterogeneous ordered pins](../juxtopposed-references/home/822.png) | **Present.** Three folder rows, conflict, Retry and cold persistence captured; remaining kinds/long names unscored. | R1 scoped repair passes native before/after plus ordinary stale writer/Retry/restart. Full heterogeneous actions remain. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/saved-pins-retry-three-retained-0.png); all beta acceptance remains open. |
| S16 | [New Playlist confirmed; complete collection/detail layout reference gap](../juxtopposed-references/advanced/0808-new-playlist.png) | **Partial; reference gap.** Mixed playlist wrapper reuses traditional collection/content components; empty state captured. | Native mixed collection backend exists; populated edit/selection/order/restart acceptance remains. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/s16-playlists-empty-light-0.png); all beta acceptance remains open. |
| S17 | [11:45/11:55 folder/player states; exact full-library variants unresolved](../juxtopposed-references/coverage-ledger.md) | **Partial.** Compact authored rows plus traditional list; original video-folder/player composition is not complete. | Two indexed real fixtures shown and activated; groups/drag/return/resume matrix remains. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/s17-video-library-0.png); all beta acceptance remains open. |
| S18 | [12:10 Add/Songs/Albums/Artists/Genres](../juxtopposed-references/figma/sidebar-icons-20261018/music-reference.png) | **Present.** Authored category icons/list; outer full-library vs compact source-pane contexts need comparison. | Four native songs indexed; category/album route used; other category journeys remain. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/s18-music-landing-0.png); all beta acceptance remains open. |
| S19 | [12:10 confirms categories; complete detail layouts need original references](../juxtopposed-references/coverage-ledger.md) | **Partial; reference gap.** Albums/artists/genres still use traditional collection/detail components; do not invent unseen layouts. | Real two-album/two-artist metadata/covers; album activation plays collection. Full drill-down/selection remains. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/s19-albums-0.png); all beta acceptance remains open. |
| S20 | [11:45 original video-folder pane; exact extracts retained with this receipt](../juxtopposed-references/coverage-ledger.md) | **Partial.** Music has authored source pane; local/video browsing uses traditional pages, without original video folder pane beside player. | Owned folder pin traverses real browser; music source list shown. Missing/moved/back/activation matrix remains. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/s20-local-browser-empty-light-0.png); all beta acceptance remains open. |
| S21 | [6062:13354, 375x585; 13:07](../juxtopposed-references/advanced/0787-source-menu.png) | **Redesign missing.** Ordinary three-dot menu is nested Media/Playback/etc, instead of the referenced flat source menu. | Real legacy providers and redesigned dialogs remain reachable; preserve those routes during presentation work. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/s21-media-menu-0.png); all beta acceptance remains open. |
| S22 | [6216:13614; ordered URLs and optional playlist name](../juxtopposed-references/advanced/0792-network-dialog.png) | **Present.** Designed form exists; matched geometry, errors and responsive variants remain unscored. | Native ordered URLs/playlist service retained; no live network request submitted during this inventory. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/s22-network-dialog-0.png); all beta acceptance remains open. |
| S23 | [6259:14954, 713x589](../juxtopposed-references/figma/20261014/new-playlist.png) | **Present, theme gap.** Authored form; R3 fixed dark translucent body conflicts with Light foreground tokens. | Native picker/drop/mixed saves retained from prior scope; form opened/cancelled only here. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/s23-new-playlist-dark-0.png); all beta acceptance remains open. |
| S24 | [Convert menu confirmed; original form/profile editor absent in scoped references](../juxtopposed-references/advanced/0787-source-menu.png) | **Functional adaptation; theme gap.** Light text nearly disappears on fixed dark body; label form as an adaptation. | Ordinary adapted lane is bounded audio-to-FLAC; All formats/profiles opens legacy workflow. Broad conversion completion remains open. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/r3-conversion-light-0.png); all beta acceptance remains open. |
| S25 | [1911:7545 / 3336:11929 / 1919:8707 / 1919:9278](../juxtopposed-references/figma/20261026/subtitle-attachments/file-online.png) | **Partial.** Local/scoped layouts exist; integrated online query/language/results layout is missing. | Local association service retained; online currently hands off to separate service. No search/save submitted here. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/s25-online-attachment-gap-0.png); all beta acceptance remains open. |
| S26 | [08:00; unshown naming form is an adaptation](../juxtopposed-references/advanced/0480-subtitle-presets.png) | **Present; naming adaptation.** Default/saved/New Custom menu present; exact naming geometry unprovided. | Real prior saved preset visible; no preset changed in inventory. Broader apply/retry/restart stays scoped. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/s26-presets-light-0.png); all beta acceptance remains open. |
| S27 | [3744:12361; 06:59 editorial cue strip](../juxtopposed-references/advanced/0419-subtitle-timeline.png) | **Present, format-limited.** Strip/range/zoom/status present; populated SRT/short/long fidelity remains to compare. | Real source-cue model supports selected external SRT snapshots; embedded/image source explicitly unavailable. No text editing/export claim. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/s27-cue-timeline-embedded-unavailable-0.png); all beta acceptance remains open. |
| S28 | [12:18 / 12:20; 4767:23726 / 4778:25123](../juxtopposed-references/advanced/0738-music-player.png) | **Present.** Authored cover/source/waveform layout shown with actual synthetic cover; full matched scoring and lyrics variants open. | Real metadata/folder/decoded waveform used; tagged-lyrics action conditional, not fully tested here. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/s28-music-player-0.png); all beta acceptance remains open. |
| S29 | [Thirteen category frames in 20261014 and 20261017 manifests](../juxtopposed-references/settings/global-settings-reference-17m50.png) | **Present.** All thirteen category pages have authored presentation; General/Interface inspected here, not every field/state. | Real category models retained; SETTINGS.md owns per-field unsupported consumers and timing/verification gaps. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/s29-global-settings-dark-0.png); all beta acceptance remains open. |
| S30 | [999:5047 General; 3046:10057 Interface; 2425:13624 Light player](../juxtopposed-references/figma/20261014/manifest.json) | **Present, cross-surface theme gaps.** Light General/Interface work, but conversion/New Playlist and queue/bookmarks are inconsistent. | Actual Light save/application exercised in isolated profile; System events/OS DPI/global combinations remain. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/s30-global-settings-light-0.png); all beta acceptance remains open. |
| S31 | [1902:3670 / 1902:6770, 1090x759](../juxtopposed-references/figma/20261014/manifest.json) | **Present.** Both pages observed; short-window appearance page requires scrolling. No short-layout failure inferred without that check. | Appearance-only marker reset in consented isolated profile; Next and cancel observed; Finish persistence retains older scoped receipt. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/s31-setup-shortcuts-light-0.png); all beta acceptance remains open. |
| S32 | [2437:14832 Shortcuts; 6648:20666 Profiles; customization frames in manifest](../juxtopposed-references/figma/20261030/inventory-shortcuts/keyboard-shortcuts.png) | **Partial; naming adaptation.** Profile/new form and tile customizer present. Shortcut editor is QWidget/KeySelector, missing confirmed redesigned grouped-row form. | Native profile/layout/shortcut services retained; this inventory opened/cancelled profile/customizer only. No full shortcut journey claim. | [Ordinary capture](../juxtopposed-references/reviews/graphical-inventory-20261030/captures/s32-customizer-light-0.png); all beta acceptance remains open. |

### Selected visual work

This ordering follows the inventory; it does not replace W1-W5 or remove their
unfinished requirements. Complete one bounded visual package and keep moving
when the user authorizes a continuing implementation session.

1. **Shared theme/contrast and walkthrough stability.** Fix R3's Conversion and
   New Playlist backgrounds/text, and the related Queue/Bookmarks/Chapters Light
   mismatch. Preserve authored Dark values. Use paired ordinary Dark/Light/System
   captures and failure/empty/populated states. Reproduce the inventory's single
   recursive-layout warning during music teardown/source replacement; identify its
   actual owner before changing layouts. Keep the absent-filter `subsdelay-factor`
   warning visible until its lookup is qualified. Normal exits do not close GA5.
2. **Original source and profile menus.** Implement the confirmed flat source menu
   and profile presentation using original assets/spacing while retaining working
   native file/folder/network/playlist/conversion/settings routes and the traditional
   controls opt-out. Compare against `6062:13354` and `6648:20666`.
3. **Missing effects and shortcut presentations.** Port the confirmed Compressor,
   Spatializer, Stereo Widener and Essential/Crop/Colors layouts, keeping the current
   EQ/native consumers. Correct the More video effects route, which currently opens
   the traditional Audio tab. Implement the confirmed `2437:14832` grouped shortcut
   editor over the existing checked shortcut service; preserve conflicts/cancel/save
   semantics. Backend limits stay explicit; do not make an ordinary mock save succeed.
4. **Browser/library context and remaining reference gaps.** Complete the video
   folder pane beside the player, align local browsing with the shared header and
   review collection/detail/grid/list states. Music already has an authored source
   pane; reuse established architecture where appropriate. Confirm missing original
   playlist/music-detail/track-selector references before inventing layouts. Preserve
   the owner's Continue Watching/Watch Again behavior and all saved-pin repairs.
5. **Integrated subtitle search and the full visual walkthrough.** Build the
   confirmed online query/language/results states; retain local scope matching.
   A separate, explicitly labeled development preview may show unfinished service
   states until real search is ready. It must not replace working local behavior or
   imply a successful ordinary search/download. Then complete matched-size scoring,
   cue/preset/music/setup/customizer states and the broader GA2-GA6 walkthrough.

All these packages retain empty/loading/selected/disabled/error/retry, scrolling,
focus, movable top-level dialogs, resize and theme checks. Conversion and profile/
preset naming forms remain labeled adaptations where the reference is absent.
An approved development preview is evidence of presentation only; beta still
requires real, tested behavior. No additional dependency installation is authorized.

## Shared acceptance rules

- Verify applicable empty/loading/available/selected/disabled/error/retry states,
  long names, unknown duration and source changes with actual data. Exercise
  pointer, keyboard, wheel/drop and accessibility actions where offered.
- Preserve primary input, queue, tracks and clock during independent navigation,
  dialogs and background work. Check paused and playing paths separately, along
  with cold/warm startup, rapid A-B-A changes, blocked writes and normal shutdown.
- Cover real/generated video, audio, chapters, multiple audio tracks, embedded/
  external/multiple subtitles, tagged/missing artwork, live/unknown-length inputs
  and conversion output. Use `D:\Media` read-only; keep private media out of Git.
- For conversion, verify actual decoded samples, EOS/tail, error/cancel, destination
  races and teardown. Retain the strict 88,200-sample FLAC regression; a duration
  tolerance alone cannot establish a complete tail.
- Review actual VLC captures in normal/narrow/short windows, Dark/Light/System,
  nondefault accent/font, 100/150/200% Windows DPI and cross-monitor movement,
  plus applicable pinned/floating/fullscreen/detached panels. Component scale
  simulations do not establish Windows DPI behavior. Cover risky intersections
  and representative combinations rather than an unbounded Cartesian product.
- Keep the agreed visual targets: assessed canonical screens at least 9.5/10,
  assessed variants at least 9/10, no critical defects. Use named criteria:
  layout 35%, typography 20%, surfaces 15%, artwork 15%, state/responsive fidelity
  15%. Record unassessed categories; test counts cannot supply a visual score.
- Bind evidence to source, binaries, fixtures, platform and normal process exit.
  Retain before failures and intentional negative controls separately. Investigate
  unexpected exceptions/assertions/warnings; document scoped fixture diagnostics.
  A different candidate does not automatically inherit every older GUI's result.
- Reuse original references and cached Figma data first. Spend further authorized
  MCP calls on concrete gaps/validation, recording node, purpose and outcome.
  Follow the applicable skill and access controls. Missing references remain
  explicit questions or labeled functional adaptations.

## Multi-instance and shared state

The movie-plus-music/multiple-monitor scenario is a required regression. Use
[SETTINGS ownership rules](SETTINGS.md#ownership-and-application-timing) for live
per-player state versus shared saved defaults, profiles, privacy and window memory.

| Scenario | Required observation |
| --- | --- |
| Fullscreen movie plus music on another monitor | Media/queue/tracks/rate/seek/mute/volume and focused shortcuts affect the intended player; quitting one leaves the other intact. |
| Two distinct physical audio endpoints | Route A and B independently, switch one and verify the other. Default and its named alias are one endpoint. |
| Device loss/default change | Explicit-versus-Default routing, useful recovery/error feedback and survival of the unaffected player. |
| Shared saves/profiles and both closing orders | Unrelated updates survive; same-key policy, rejected writes/reload/retry and remaining Qt exit writers are qualified. No stale exit may overwrite newer defaults. |
| Monitor topology and restart | Normal bounds/fullscreen survive closing orders; offscreen recovery and DPI changes are qualified. Current window memory has one shared last-closed record. |
| Separate configuration roots/file-manager launches | Intentional storage isolation, open/enqueue/single-instance policy and correct handoff target. |

Scoped shared-save, theme/profile, simultaneous-playback and Quit receipts already
exist. Continue their remaining combinations; do not invent a new per-instance
settings UI or persistent startup identities. Physical endpoint/receiver/topology
limits require honest pending or explicitly approved support statements.

## Beta-ready gates

The local Windows 11 candidate is ready only when every applicable gate below has
actual evidence and any external deferral is explicitly approved and disclosed.
User-requested pauses and elapsed session time leave these gates unfinished.

- [ ] **G1 Complete behavior:** every confirmed surface/action and thirteen-category
  consumer contract is implemented, reachable and qualified for its advertised scope.
- [ ] **G2 Interaction/design:** full ordinary journeys, appearance, focus,
  accessibility, responsive layout and OS-DPI/monitor gates pass; reference gaps
  and functional adaptations are explicit.
- [ ] **G3 Integrated stability:** the selected current tuple passes representative
  media, persistence, failure/retry, multi-instance and normal-shutdown regressions;
  no unresolved normal-flow crash or material state-loss defect remains.
- [ ] **G4 Independent review:** final code and native design captures are reviewed,
  material findings are resolved and validation is attached to the reviewed revision.
- [ ] **G5 Clean package:** fixed-source, matched ordinary runtime/resources/plugins;
  no private hooks, test profiles, personal media or dumps. Fresh first run,
  Unicode/space relocation and installed-VLC coexistence pass on Windows 11.
- [ ] **G6 Reviewable delivery:** reproducible source/build notes, feature/limitation
  matrix, designer/font/dependency/software credit and licenses, version/signing
  status, checksums and explicit platform/hardware deferrals accompany the candidate.

Prepare this concrete result before seeking publication approval. Committing,
pushing, tagging or uploading a release follows the user's current authorization;
producing a candidate and publishing it are separate decisions.
