# VLC Juxtopposed: first publishable beta plan

15 October 2026. **Target: a feature-complete Windows beta of Juxtopposed's design, committed in Git with a reproducible runnable package. Status: incomplete; this document is a plan, not beta acceptance.**

The user's latest request supersedes the earlier goal of one useful action per surface. Preserve the [milestones 0–8](juxtopposed-vertical-slice-plan.md) and their completed evidence, but use the beta gates below as the completion criterion. Every confirmed design surface and action must be reachable and functional. A legacy dialog link, supplied controller, placeholder data or a single successful example does not finish a redesigned surface. Supported-format limitations must be explicit and consistent with the advertised feature.

The [15 October General checkpoint](juxtopposed-references/native/general-settings-20261015/README.md)
adds the real native General page, shared modal ownership and live theme/accent
propagation. Ordinary native backend tests pass 29/0/1; the actual app restores
Light/Purple and Unicode Errors-only logging after restart, and three normal quits
preserve the checked existing user configuration. Maintained General/shared-host
tests and the actual GNU-make resource-remake oracle pass locally. General is the
only enabled category; Interface/Privacy and the other ten bodies, backdrop blur,
Light volume, Reset, broader design acceptance and final packaging remain open.

The following [Light volume checkpoint](juxtopposed-references/native/light-volume-20261015/README.md)
closes the original Light background/full-fill selection gap with a rebuilt native
pointer/keyboard/mute/wheel/theme-switch journey and seven maintained component
groups. Complete Light states, System events and full design acceptance stay open.

The [settings backdrop checkpoint](juxtopposed-references/native/settings-backdrop-20261015/README.md)
accepts native Home blur/shadow and preserves translucent tint over separate native
video. The original shader oracle and maintained 12/24/23 General/host/software
checks pass. A real GNU-make pair reproduces and repairs stale incremental QML
metadata; the final ordinary build opens its actual settings types and quits
normally. Hardware-video blur, performance/DPI/full fidelity, geometry drift and
the remaining beta gates are unfinished.

The [independent settings shadow follow-up](juxtopposed-references/native/settings-shadow-20261015/README.md)
keeps the original shadow over real native video in Light/Dark and preserves Home
blur. Its first ordinary build includes the new QML type, original 39 native
shader assertions and maintained 12/24/23 checks pass, and the app quits normally
with checked human settings unchanged. All four preceding backdrop-head CI jobs
pass; this new checkpoint's CI and full video blur remain separate open gates.

The [Windows geometry follow-up](juxtopposed-references/native/window-restore-20261015/README.md)
fixes the reproduced restart shrink by restoring the saved client rectangle after
native frame initialization. Exact original/fixed restart oracles, both title-bar
modes, native maximize/fullscreen restoration and two ordinary app launches pass.
Other DPI/missing geometry/startup states and this new head's CI remain separate;
all four preceding shadow-head CI jobs pass. This does not finish the beta.

The user authorizes Figma inspection and asks that MCP calls be conserved. MCP access was restored on 14 October with a Professional Full seat: page inventory and high-fidelity Home, New Playlist, subtitle-timeline and its original settings-toggle context/screenshots now read successfully. The [saved MCP inventory](juxtopposed-references/figma/20261014/README.md) identifies the single Page 1 and canonical nodes; metadata alone does not establish visual acceptance. The separate saved browser-site block remains unresolved and must not be circumvented. Root owns MCP access checks and shared integration/build/Git/package work; reuse cached references before requesting more context.

Original design and artwork belong to [Juxtopposed](https://www.youtube.com/@juxtopposed). Source references are the [community design](https://www.figma.com/community/file/1691465999134370980/vlc-redesign), [accessible design copy](https://www.figma.com/design/zdNUYRVOH9lEtIxMYvh8DV/VLC-Redesign--Community-), unchanged manual exports and the supplied local walkthrough. Preserve designer artwork bytes, provenance and the existing CC BY 4.0/Inter/VLC license materials.

## Evidence baseline and status rules

The [coverage ledger](juxtopposed-references/coverage-ledger.md) contains 32 surface rows. The [advanced manifest](juxtopposed-references/advanced/manifest.json) preserves 13 exact-time original frames and the video SHA-256 `70b0ccfa8276acd8fbdf876b96233ebde13aab922acb4f618ef7c31c422aff08`. [Home](juxtopposed-references/home/manifest.json), [library](juxtopposed-references/library/reference-notes.md), [settings](juxtopposed-references/settings/implementation-checkpoint.md) and [queue](juxtopposed-references/queue/manifest.json) provide additional references. Walkthrough screens are reference media, not implemented application chrome. Editorial zooms do not establish canonical dimensions.

The latest specific checkpoints take precedence over older summary status:

- [Runtime controls](juxtopposed-references/runtime/player-controls-verification.md): actual floating controls, paused seeking, Back/retained player and fullscreen pass within the recorded fixture scope.
- [Home desktop](juxtopposed-references/home/desktop-checkpoint.md): real video/audio history, See All, search/clear, sorting, view switching and retained audio return. The [latest separate Home native helper suite](juxtopposed-references/advanced/new-playlist-checkpoint.md) passes all 11 checks, including real audio-only roots/events/restart and blocked-store/clear handling. The [later actual audio-only folder journey](juxtopposed-references/home/audio-folder-desktop-20261014/README.md) passes native Add Folder, two-track folder/player routing, play/pause/Back and card/contents after normal quit/restart in the existing profile. Fresh-user first-folder setup and complete visits/reset/opt-out/error journeys remain open.
- [Music](juxtopposed-references/advanced/music-player-checkpoint.md): real source-folder model, metadata, waveform, timer and desktop seek/return/media replacement; actual artwork blur, lyrics and native DPI/other formats remain open. A separate dark VLC fallback is prepared; native acceptance is still needed.
- [Subtitle presets](juxtopposed-references/advanced/subtitle-preset-checkpoint.md): native persistence/selection/rendered output, actual normal-lock controller and appearance/duration regressions pass. Integrated footer Save/apply/restart is unproved; a settings popup navigation/input problem is under investigation.
- [New Playlist](juxtopposed-references/advanced/new-playlist-checkpoint.md): original 9 native passes preserved; latest suite has 10 native passes, alongside 14 earlier broader component/source suite passes. Eight actual desktop captures establish title/image/media pickers, reordered repeated audio/video/video save, copied Home cover/route after restart, and repaired individual/all-unknown `--:--` durations on Qt build `88452491…ed99c6`. Native Other/`UNKNOWN` classification is preserved. Active-primary preservation, desktop drop/empty/cancel/errors and DPI/light/full fidelity remain open.
- [Network](juxtopposed-references/advanced/network-dialog-checkpoint.md), [waveform](juxtopposed-references/advanced/waveform-checkpoint.md), [video transform](juxtopposed-references/settings/video-transform-checkpoint.md), [subtitle appearance](juxtopposed-references/settings/subtitle-appearance-checkpoint.md), [subtitle duration](juxtopposed-references/settings/subtitle-duration-checkpoint.md) and [audio drain](juxtopposed-references/advanced/audio-drain-checkpoint.md) retain their separate native/component limits.
- Folder-bootstrap and unknown-duration corrections are integrated/built with the specific helper and desktop evidence above; those passes do not finish all Home or playlist journeys. The selected-local-UTF-8-SRT cue service is mounted in the native controls and passes twelve native controller cases; embedded cues, renderer expiry and the complete desktop timeline journey remain open. The FLAC conversion controller passes eleven native cases, including literal percent-character paths. Its separate [desktop proof](juxtopposed-references/native/conversion-desktop-20261014/README.md) records actual pickers, completion and an independent full PCM match on the preceding GUI build. The form is a bounded shared-style adaptation without a confirmed original Figma form; all profiles and complete Convert/Save remain open.
- The [leaf-route proof](juxtopposed-references/native/navigation-leaf-20261014/README.md) and [mounted navigation journey](juxtopposed-references/native/navigation-desktop-20261014/README.md) repair nested property forwarding and verify Home/Videos/Playlists, real empty/mixed playlist contents and restored second-playlist selection. All seventeen local component/source suites pass. Broader Music/Network history, rapid generations, native pointer restoration, reference fidelity and final packaging remain separate gates.

For each row record four independent statuses: **functional implementation**, **native app verification**, **reference/visual acceptance**, and **final package verification**. Keep supplied-state QML, injected failures, native QObject/core tests and actual Windows journeys labelled separately. A newer source or DLL does not automatically inherit the older package's results.

## Reference work first

- [ ] Reconcile the successfully read single-page MCP inventory's actual frames/variants with every unique screen, overlay, modal, menu and reusable control in the 32 existing rows. High-fidelity reads now succeed; keep complete surface/state reconciliation open rather than treating metadata inventory as design acceptance.
- [ ] Prioritize complete Global Settings/categories, profile/customizer/shortcuts, conversion/progress/profile selection, subtitle scope/timing interactions, first-run sequence, music category content, sidebar pins and light states. These gaps affect beta scope or backend choices.
- [ ] Export original icons/assets and record exact node, dimensions, tokens, state, source date and hashes. Keep new exports separate from old reference bytes. Do not guess node IDs.
- [ ] Resolve Home's cut-off lower grid, full player/library coexistence, detached queue, narrow/fullscreen and missing-art/error states. If Figma does not specify a necessary state, document a consistent functional adaptation and its provisional fidelity.
- [ ] Confirm whether timeline dragging changes overall delay or individual cues, what its zoom/navigation controls do, and whether text editing/export is actually designed. The preserved strip/narration confirms timing interaction; text editing/export is still unconfirmed.
- [ ] Confirm all conversion/streaming controls and profiles before specifying the final form. The 13:10 material previously labelled conversion is the network dialog. A menu entry is not a form reference.

Preserved authoritative nodes include Queue `463:831`, Bookmarks `6320:13784` and Chapters `6321:14045`, each 400 × 946. The new MCP inventory additionally establishes Home `6261:15078` (1920 × 1080), New Playlist `6259:14954` (713 × 589), subtitle control-bar variant `3744:12361` (1920 × 54), its original “Show Subtitle Timeline on Seekbar” toggle `5187:24487`, and the 13 Global Settings category frames. These IDs are grounded in successful reads, not inferred from the video. Context and screenshots remain required for each implementation.

## Every known design surface

All rows below remain unchecked for **complete beta acceptance**, even where existing checkpoints prove substantial behavior. Each needs the shared state/visual/package gates later in this document.

| Beta surface | Grounded reference | Required complete behavior and current gap |
| --- | --- | --- |
| [ ] Player transport and volume | Walkthrough around 01:30; preserved playback assets | Real previous/play/pause/next, mute/volume, duration/remaining time and available-state feedback; pointer, keyboard, wheel and engine-originated updates. Existing controls are built; complete media variants/focus/visual gates remain. |
| [ ] Main timeline and seek preview | Around 03:30; existing slider/assets | Accurate current/unknown/long duration, held drag/precise seek, preview, chapter/bookmark marks and paused/resumed clock. Core regressions pass; marks, preview fidelity and variants remain. |
| [ ] Queue | Node `463:831`; queue manifest | Native selection/filter mapping, play/remove/reorder/add/drop, jump to playing, sort/repeat/shuffle and save/stream selected entries; retain state across library/player/attached/detached routes. Native lifecycle, blur and final variants remain. |
| [ ] Bookmarks | Node `6320:13784` | Real add/seek/rename/remove/sort and media association; unavailable library/seek states and no stale actions after rapid media switches. Components/source review exist; full native lifecycle remains. |
| [ ] Chapters | Node `6321:14045` | Actual chapter selection/current feedback and timeline marks; keyboard and no-chapter states. Generated chapter fixture exists; native action acceptance remains. |
| [ ] Audio/subtitle track panels | Around 06:10; exact frame/node still needed | Actual selected tracks, primary/secondary roles, external subtitle open, empty/unavailable states, dismiss/focus. Final panel structure and track/media-switch acceptance remain. |
| [ ] Subtitle appearance and synchronization | Settings 07:50; advanced 08:35 | Every confirmed font/color/outline/background/offset/size-mode/scale/embedded-format/duration/delay/FPS field, default track and language. Native style/duration/controller paths pass; defaults/language/persistence/full menu variants remain. |
| [ ] Playback settings | Settings 09:10; advanced 09:08 | Four-tab navigation and native speed/numeric/slider synchronization; inspect reverse, sleep timer, metadata and any additional designed options. Their placement/complete behavior is not established by existing speed controls. |
| [ ] Video defaults and effects | Settings 09:00 | Flip/rotation/zoom/aspect/crop/deinterlace and every additional confirmed HDR/resolution/effects field. Existing rendered transforms pass; remaining fields and hardware/non-square/media-switch/persistence remain. |
| [ ] Audio equalizer and effects | Settings 08:50 | Native stereo/mix/delay, preamp, all bands, preset/two-pass state and other designed audio effects. Real PCM check passes; full desktop/persistence/effects layout remains. |
| [ ] Shared navigation | Library 11:45/11:55 | All labelled destinations, expanded/collapsed rail, native history, scanning/unavailable state, keyboard and stable route/selection. Inferred widths need original measurements/assets and final native variants. |
| [ ] Shared title/window strip | Library 11:45/11:55 | Home/menu/title/window controls, panel coexistence, maximize/fullscreen/minimize/close and focus. Current Home entry works; full original shell presentation remains. |
| [ ] Library header | Library 11:45/11:55 | Native Back/Forward, breadcrumbs/count, sort/view/search/main menu and correct selection under filtering. Built adapters and limited desktop passes; cross-category and variant acceptance remains. |
| [ ] Home and Add Folder | Home 13:47/13:49.3/13:56 | All five real sections: Continue Watching, Listen Again, frequent folders/playlists/artists; See All, activation/resume, valid visits, clear/reset, opt-out, deletion/unavailable IDs and restart. Audio-only bootstrap is integrated/built with 11 native helper/event passes; verify full service/picker/first-folder playback and remaining visit journeys. |
| [ ] Sidebar pins | Home 13:42; narration 13:37–13:43 | Ordered stable media/folder/playlist/artist/source descriptors, pin/activate/reorder/unpin and restart. Native favorites alone do not supply the designed heterogeneous order. |
| [ ] Shared playlists and contents | Library navigation; detailed content reference needed | Real mixed-media collection/content layouts, selection/sort/search, creation/edit/order/removal/open and playback. Creation backend is built; complete content layouts/actions and active-player independence remain. |
| [ ] Video library | Library 11:45/11:55 | Referenced grid/list/folder/group/player variants, genuine metadata/thumbnails, filtering/selection/drag and play/return/resume. Index/search/activation passes; full layouts/groups/native variants remain. |
| [ ] Music category landing | Library 12:10 | Add/Songs/Albums/Artists/Genres routes, counts, sorting/view/search and proper icons. Components/native entry exist; every category transition needs final native/visual acceptance. |
| [ ] Songs, Albums, Artists and Genres content | Library 12:10 plus Figma content inspection | Complete referenced collection/detail views with real audio/album/artist/genre models, selection, drill-down and playback/return. Compact Songs adapter is partial coverage; category/detail layouts remain. |
| [ ] Browse/local sources | Around 13:30; detailed reference needed | Actual source/folder traversal, unavailable sources, history, file activation and return; retain source identity. Existing browser infrastructure is not final redesign acceptance. |
| [ ] Open/source menu | Advanced 13:07 | Original menu structure/icons/shortcuts and real file/multiple-file/directory/disc/network/capture/clipboard/recent routes, save playlist, Convert/Save, Stream, Customize, Plugins/Extensions, Settings and About. Native providers exist; redesigned menu/dialog coverage remains. |
| [ ] Open Network Stream | Advanced 13:12 | Ordered URL validation/append/play and optional named mixed playlist, errors/retry/Cancel/focus; designed menu variants and supported protocol states. Backend/limited native modal passes exist; actual Save/playback and external protocol journey remain. |
| [ ] New Playlist | Figma `6259:14954`, 713 × 589; Advanced 13:28 | Actual picker/drop, ordered/repeated mixed files, title/optional copied thumbnail, busy/error/partial-save routes, no duplicate creation, native cover refresh and restart. Current compiled 670 × 554 layout needs alignment with the newly confirmed reference. Latest 10 native passes and observed picker/save/copied-cover restart pass their recorded scope; active primary, desktop drop/empty/cancel/errors and scaled/light/full visual gates remain. |
| [ ] Conversion and export | Convert/Save at 13:07; form not yet identified | Every confirmed source/profile/output/progress/cancel/error action, playable complete output and source/primary-playback preservation. Pending local audio→FLAC is only one bounded profile; final designed profiles and native journey remain. Do not use the classic route as the independent controller. |
| [ ] Subtitle attachment scope | Advanced 06:06 | This File/This Folder/This Playlist with real persisted subtitle association/default selection and precedence, invalid/moved-source behavior and restart. Current-file loading and per-media style presets do not implement folder/playlist subtitle-file scope. |
| [ ] Subtitle presets | Advanced 08:00 | New Custom/Default/saved selection/edited state, meaningful save/apply/error/restart and actual track/timing restoration. Service tests pass; native popup focus/input/Save/apply/restart remains a blocker. Unshown naming form remains provisional. |
| [ ] Subtitle cue timeline | Advanced 06:59; nearby local strip frames | Mount actual cue service, seek with revision/source guards, real timing-shift interaction and confirmed zoom/navigation controls; retain playback text/identity and supported-state explanation. Source parser is currently bounded UTF-8 external SRT and fails closed for embedded/image/multiple/FPS/duration effects. Inspect advertised design scope; extend native support where required rather than calling the file snapshot a rendered cue bridge. |
| [ ] Music player and waveform | Advanced 12:18/12:20 | Canonical cover/source-folder list/blur, actual artwork/title/artist, real waveform seek/elapsed/future bars, explicit queue fallback and designed lyrics/metadata/settings actions. Core/folder/clock journey passes; native artwork/lyrics/DPI/codec and visual gates remain. |
| [ ] Global Settings shell | Settings 17:50; advanced 18:48 | Real searchable App/Media/System category navigation, shared controls/help, persistent save/reset/error and Advanced Options. All 13 routes require their complete confirmed settings, not empty categories. |
| [ ] Appearance/interface settings | Global General and nearby frames | Language, Dark/Light/System and accent choices, true theme propagation through every surface, meaningful system-theme response and restart. Current dark literals do not establish complete theme support. |
| [ ] First-run theme setup | Settings 19:50 | Distinct Light/Dark/System cards, real preview/accent selection and Back/Next; inspect remaining setup steps and persist completion/selections. Fresh portable launch currently needs separate first-run acceptance. |
| [ ] Profiles/customization/shortcuts | Advanced 19:35 | Movies/Shows/Music/Work/New profile actions, original Customize Player and Keyboard Shortcuts screens; actual layout plus all designed preference scope, save/switch/restart/reset/conflict handling. Existing toolbar profiles are partial support. |

The source menu additionally requires coverage records for its dialogs/submenus even if the final inspection finds no redesigned body: Open Disc, Capture Device, Recent, Save Playlist to File, Stream, Plugins/Extensions and About. Real native actions need explicit exercised or hardware-unavailable outcomes. A comparison shot of classic VLC or its classic Toolbar Editor is not evidence of a redesigned dialog.

## Complete Global Settings routes

The preserved audit confirms **13 category routes**. Inspect each original page, list every visible field/action and map it to native persistent state. All are open beta gates:

| Group | Required categories | Current specification limits |
| --- | --- | --- |
| App | [ ] General; [ ] Interface; [ ] Privacy; [ ] Controls | General visibly contains interface language, Dark/Light/System, accent swatches, confirmation/error dialogs, automatic updates, Developer mode, Save a log, Log file/Browse, Log details and Advanced Options. Other category contents need Figma inspection. |
| Media | [ ] Playback; [ ] Video; [ ] Audio; [ ] Subtitles; [ ] Library | Playback popup fields are partial reusable support; they do not finish global persistent category pages. Default subtitle track/language and library/history/scanning/reset policies need complete mappings. |
| System | [ ] Network; [ ] Performance; [ ] Recording; [ ] Extensions | Routes are confirmed; original fields, validation, persistence and unavailable states remain unspecified. Do not invent settings to fill the pages. |

- [ ] Maintain a per-field mapping: original label/state, native variable/model, units/range/default, configured-versus-live semantics, restart requirement and reset/error outcome.
- [ ] Verify ordinary Save does not unexpectedly replay a media preset or clear activity; Reset must clear the intended settings and live caches without queued-worker resurrection.
- [ ] Preserve signed microsecond synchronization values, inactive style/duration values, unknown media lengths and category inheritance; do not coerce them to unrelated defaults.
- [ ] Settings search must reach the actual fields/categories, retain edits correctly, and support short-window scrolling and keyboard access without leaking actions to playback.

## Milestones 0–8 retained, beta gates expanded

| Milestone | Beta finish criterion |
| --- | --- |
| 0 Reference/state inventory | Browser/Figma/video reconciliation accounts for every original surface/action/variant; unknowns and adaptations are explicit with exact provenance. |
| 1 Runtime foundation | Fresh/warm startup and ordinary launch, real audio/video/paused seek/resume, helper/plugin/resource availability and bounded shutdown pass on the final package. Retain repaired clock/drain regressions. |
| 2 Shared shell/Home | Full original shell, all Home sections and sidebar pins work with real data, visits/reset/opt-out/restart and retained playback. |
| 3 Library/discovery | Complete video/music/category/playlist/source layouts, metadata, search/sort/view/drill-down and all designed opening routes work. |
| 4 Player/queue | Full transport/timeline/tracks/chapters/bookmarks/queue, pinned/floating/detached/fullscreen states and stale-media/focus behavior pass. |
| 5 Settings/effects | Every confirmed popup/effect/synchronization action reaches native state and reflects native changes; persistent/live semantics and native popup usability pass. |
| 6 Advanced surfaces | Complete subtitle scope/presets/timeline, music/waveform/lyrics and confirmed conversion/export workflows pass with actual data and output. |
| 7 Preferences/appearance | All 13 settings categories, profiles/customizer/shortcuts, first-run flow and Dark/Light/System/accent behavior are complete and persistent. |
| 8 Beta integration/package | Final exact Git revision, native/component/visual journeys, fresh relocatable package, credits/licenses/build/reproduction notes and documented beta limits pass together. |

Recommended sequence: inventory and persistent/theme/profile model decisions first; finish native preset popup and full folder-bootstrap desktop journeys while cue/conversion backend tests proceed; port the complete categories/source menu/library details/scope/customizer; integrate cue and conversion presentation using confirmed references; then run the full final package/visual matrix. Keep one writer for the native build and integrate shared glue sequentially. Preserve known-good source/package snapshots between reviewable commits.

## Native, state and visual acceptance

- [ ] Every surface handles its applicable empty, loading, available, selected, disabled, error/retry, long-text, unknown-duration, keyboard-focus, short/narrow and media/source-change states using genuine data. An unshown visual state gets a documented adaptation, not a fidelity claim.
- [ ] Run actual video, audio, chaptered, embedded/external/multiple-subtitle, tagged artwork, missing-art, unknown-length and network fixtures. Verify pointer, keyboard, wheel/drop and accessibility actions on the final app.
- [ ] Exercise active playback through Home/library/queue/settings/pickers/New Playlist/conversion and return; preserve primary queue, selected item, track identity and clock where the action promises independence. Test paused state separately.
- [ ] Exercise native presets Save→edit→apply→quit/restart→Default; scoped subtitle attachment and source changes; chapter/bookmark lifecycle; Home audio-only Add Folder→visit→open→play→restart and reset while indexing; profile/theme/category persistence.
- [ ] Verify timeline source/generation/revision guards with actual normal-lock native tests and app actions. Displayed source-file snapshots must not be labelled decoder-rendered expiry; external file Reload does not reopen VLC's cached subtitle text.
- [ ] Require complete conversion samples and amplitude/output decode, real EOS/error/cancel, active-worker teardown, racing-destination preservation and actual primary independence. Retain the strict 88,200-sample FLAC regression; the existing 150ms duration tolerance cannot prove an intact tail. Native encoder selection and built-in drain/header defects need explicit results before conversion acceptance.
- [ ] Test cold/warm startup, quit with indexing/metadata/cue/conversion work, rapid A→B→A changes, missing/moved files, history opt-out/reset, unwritable persistence and stale queued results. No late callback may target a replacement source or resurrect cleared state.
- [ ] Review native captures at normal/narrow/short sizes and 100/150/200% Windows scaling in Dark, Light and System themes, with nondefault accent and pinned/floating/fullscreen/detached panels. Compare original geometry/typography/icons and native alpha/blur/rounding.
- [ ] Preserve the established visual target: canonical assessed surfaces at least 9.5/10 and assessed variants at least 9/10, with no critical defects. Record unassessed categories separately; a normalized component score cannot stand for whole-app/native fidelity.

## Conserved MCP validation budget

Figma MCP reads were restored on 14 October with a Professional Full seat. Reuse
the saved high-fidelity Home, New Playlist, subtitle-timeline and settings-toggle
context/screenshots before requesting more. The saved browser-site block is a
separate unresolved restriction; no browser read is established and it must not
be bypassed. If MCP becomes quota-limited again, continue from saved references
and retain the outstanding comparison gates. Do not repeatedly retry unchanged
metadata, infer new node IDs or retrieve already measured frames without a
concrete new question.

When read access becomes available, use a proposed ceiling of **8 planned validation calls plus 2 reserved correction calls** for this beta pass, subject to the actual remaining account limit. This is a work budget, not a claim of available quota or a task-wide token budget. Count calls conservatively and record tool, exact node, purpose, result and remaining allowance; follow the Figma skills before any applicable tool call.

| Priority | Proposed pair of validations | Reason |
| --- | --- | --- |
| 1 | Design context + canonical screenshot for a discovered Global Settings/appearance frame | Validate shared tokens/category shell/theme measurements affecting many pages. |
| 2 | Design context + screenshot for a discovered complete Home/library frame | Resolve lower-grid/shell/category geometry and original assets. |
| 3 | Design context + screenshot for the highest-risk discovered player/subtitle/music frame | Check timeline/timing/preset/cover layout after real implementation is reachable. |
| 4 | Design context + screenshot for a discovered conversion/profile/first-run frame | Use the remaining pair for the largest unresolved beta surface identified by browser inventory. |

Choose the actual four nodes only after a successful permitted inventory; unknown IDs stay unknown. Reuse saved context/screenshots for implementation and compare native captures locally. The two reserved calls are for a material unresolved mismatch, not routine repeated screenshots. If access or the budget cannot cover all surfaces, preserve manual provenance and pending validation rather than weakening native/fidelity gates.

## Publishable Git and package checklist

- [ ] Commit all necessary source, original/new documented assets, both build-system registrations, tests and reference evidence; no runtime dependency may exist only under ignored `build-windows`.
- [ ] Build the final selected Windows x64 configuration from the exact source revision with matching core/GUI/plugins/resources. Record toolchain/configuration, source revision and binary hashes. Recheck generated QML/JS dependencies and cache/resources from a clean regeneration.
- [ ] Run appropriate component CI and actual native regressions against that revision/package; record build/test results with no unexplained skips. Earlier checkpoint suite counts are not final-beta acceptance.
- [ ] Launch the unpacked package by its ordinary executable in a fresh directory/profile, without development environment paths or mandatory diagnostic flags. Resolve Qt/preparser/cache/plugins/fonts/assets/helper paths and confirm startup/video rendering without a blank window or missing plugins.
- [ ] The `qt-juxtopposed-controls` option already defaults to true in `modules/gui/qt/qt.cpp`. Verify fresh default launch uses the complete redesigned shell and first-run path; verify the explicit opt-out retains functional traditional controls and queue. Do not introduce a redundant default change without evidence.
- [ ] Verify relocation, Unicode/space paths, normal quit/restart and coexistence with installed VLC. Distribute a clean package, excluding private media-library/preferences, developer paths, logs/dumps and test fixtures.
- [ ] Bundle existing VLC/dependency/font/designer licenses and attribution, build/source instructions, exact Git revision, checksums, supported Windows/package requirements, beta feature matrix and honest known limitations. Preserve original artwork and label provisional derived VLC fallbacks separately.
- [ ] Decide and document portable-versus-installer distribution, package size/dependency completeness, version/beta naming and signing status. Test installation/unpacking/launch/cleanup for the selected artifact; an installer alone is not playback acceptance.
- [ ] Complete independent review of the final change and visual evidence, push the reviewable beta source revision, and create the chosen publishable artifact through Root's workflow once its actual gates pass. No publication or release completion is claimed by this plan.

Beta acceptance requires all confirmed behavior above and one coherent verified package. Newly discovered design requirements expand the inventory; unconfirmed text editing/export, unknown conversion geometry or unavailable category references stay explicit until inspection resolves them. Do not silently reduce the user's feature-complete beta request back to the earlier vertical slice.
