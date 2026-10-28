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

## Ordered work packages

Keep moving through these packages during an authorized beta session. Select
work using the latest evidence; repair a new failure before accepting its path.
Preserve already working implementations instead of repeating completed work
because an archived TODO says otherwise.

| Order | Package | Completion evidence |
| --- | --- | --- |
| W1 | Finish subtitle timing and immediate regressions | Qualify ordinary cross-editor FPS Reset and seek/playing/default/restart behavior, secondary/per-track timing and estimators; resolve startup failures found by their regressions. Preserve the caption-selection, paused-frame, delay and accessibility repairs. |
| W2 | Complete native consumers and designed workflows | Resolve the [settings audit](SETTINGS.md) and remaining surface actions below: customizer/shortcuts, library/detail, queue/bookmarks/chapters/tracks, subtitle scope/presets/timeline and conversion. Exercise live/default/restart timing, reset, cancel, failure/retry and persistence. |
| W3 | Finish interaction, design and accessibility | Compare actual app captures with original assets/frames, cover all 32 surfaces and risky states, close focus/scroll/drop/modal/busy-state defects, verify theme and Windows DPI/monitor behavior. True top-level popup windows must be movable. |
| W4 | Consolidate media and instance regressions | Replay representative real/generated media, paused/playing independence, source changes, restart, missing media, failure/retry, background-worker shutdown and the multi-instance scenarios below on an identified current tuple. Preserve strict output/PCM checks. |
| W5 | Review and qualify the clean candidate | Independently review the final changes and actual design captures; assemble a matched ordinary portable package from fixed source, then verify fresh startup, relocation, coexistence, features/limitations, licenses, attribution, reproducible build notes and checksums. |
| Separate lane | Windows 10 comparison | Run the prepared old/new comparison on the original guest environment, recording OS/audio versions and exception context if it recurs. Keep this external deferral visible. |

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
