# Beta readiness snapshot — 18 October 2026

Use with the [revised roadmap](../juxtopposed-beta-roadmap.md). This reconciles
the 32 surfaces in the earlier plan with newer checkpoints and selected current
source. It is a planning snapshot, not a new runtime acceptance receipt.

**Implementation** below means code/foundation is present, not that every
designed action is complete. **Native evidence** distinguishes backend fixtures
from actual app journeys. **Visual** is open for complete surface acceptance;
specific original artwork/geometry checks are identified separately.
**Final package verification is open for all 32 rows.** Historical receipts
retain their own source, binary and platform identities.

| ID | Surface | Implementation / native evidence already available | Remaining implementation or verification | Complete visual acceptance |
| --- | --- | --- | --- | --- |
| 01 | Player transport and volume | Native controls and scoped actual video/audio, pause/seek, volume/mute journeys | Current-tuple tracks, media changes, focus, pinned/floating/fullscreen and representative media | Open |
| 02 | Main timeline and preview | Native seek, timer and guest live-clock repairs; ordinary HLS replays | Marks/preview, unknown/long duration, live limitations and source-switch behavior | Open |
| 03 | Queue | Real playlist adapters; component and earlier integration evidence | Full actual filter/selection/reorder/drop/remove, state retention and detach/dock lifecycle | Open |
| 04 | Bookmarks | Native backend persistence/seek fixture included in the nine-case bookmark/chapter proof | Actual panel add/rename/remove/seek, indexing readiness, rapid media changes and unavailable-library behavior | Open |
| 05 | Chapters | Native chapter backend and generated fixture | Actual panel/current chapter/timeline marks, keyboard and empty-state journeys | Open |
| 06 | Audio/subtitle tracks | Existing native track controllers and panel foundation | Multiple tracks, selected state, external subtitles, source changes, dismissal and exact reference mapping | Open |
| 07 | Subtitle appearance/synchronization | Native style/duration/delay and renderer repairs; earlier app journeys | Final-current menu/default/language/persistence, renderer/format variants and source changes | Open |
| 08 | Playback panel | Existing four-tab panel and native speed/controller evidence | Audit every confirmed action; verify numeric/slider/native synchronization and advertised limits | Open |
| 09 | Video defaults/effects | Native preference page, rendered transforms and scoped app default/restart proof | Audit remaining consumers, hardware/non-square/current-media behavior and persistence | Open |
| 10 | Audio equalizer/effects | Native preference page, real PCM/EQ and scoped app restart proof | Remaining confirmed effects/consumers, formats, live changes and complete panel journeys | Open |
| 11 | Shared navigation | Ordered typed routes/pins and Library-OFF Recent; focus R2 native compact video741/audio742, recovery321, Home ON/OFF188 each; keyboard-return rings pass in native captures and direct ordinary Windows input | Remaining whole-app accessibility/OS-DPI and final-package selection/input coverage | Open; original Videos/Music bytes and slot geometry pass |
| 12 | Shared title/window strip | Original 40-pixel strip/assets; actual Windows system drag/double-click, compact and close checks | Actual DPI transitions, multi-monitor reachability, Windows 11 Snap and variants | Open; scoped original geometry/assets pass |
| 13 | Library header | Native history/search/sort/view adapters and scoped app journeys | Cross-category selection/history, long text, rapid filters and compact focus | Open |
| 14 | Home/Add Folder | All five sections have native app content/resume/privacy/restart proof; preview bounds repaired | Moved/new-path/parent-folder identity/history, shell-native picker, broader rapid UI and states | Open |
| 15 | Sidebar pins | Ordered media/folder/playlist/artist/source activation, persistence/retry and identity guards | Remaining presentation, reorder/input/error paths and variants on final tuple | Open |
| 16 | Shared playlists/content | Real mixed collections; native creation/order and app content/cover/restart evidence | Complete selection/search/edit/remove/drop behavior and primary-player independence | Open |
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
| 29 | Global Settings | All 13 native bodies mounted; scoped playing/paused/restart and checked-store proofs | Per-field consumer audit, reset/error/retry semantics and final-current regression | Open |
| 30 | Appearance/interface | Real theme/accent persistence; native Light contrast/accessibility repairs | Actual System notifications, font/accent states, OS DPI, blur and reachability | Open |
| 31 | First-run setup | Confirmed two-page theme/accent/shortcut flow implemented with draft-only preview and checked saving. Private R4 Cancel/Finish/retry/fresh-process reopen pass (49/60/18); ordinary R2 Next/Finish and R3 completed-profile reopen pass through actual Windows input. Three native visual defects repaired; [receipt](native/home-setup-20261019/verification.json) | Actual new-user consent widget, Windows DPI and complete visual/package states | Open |
| 32 | Profiles/customizer/shortcuts | Profiles79/79/15 and customizer111/15/111; layout11, host115, transport persistence71; [customizer receipts](native/home-customizer-20261019/README.md). Favorite183 and component24 now pass, including native accessibility/keyboard/source races, cold persistence and real write failure/retry with independent DB checks. Ordinary GUId944735cbabb passes five playback regressions; [current-media receipts](native/home-current-media-20261019/README.md) | Direct ordinary Favorite input awaits human firewall dismissal. Finish HDR/other tile actions, authoritative playlist-source name, shortcut consumers and full visual/accessibility/OS-DPI/reset/conflict/package journeys | Open |

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
snapshot. Continue independent implementation and Windows 11 validation.
