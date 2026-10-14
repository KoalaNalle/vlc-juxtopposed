# VLC Juxtopposed vertical slice implementation plan

Updated: 14 October 2026. Status: active Goal; implementation covers milestones 0 through 8.

The user activated this plan as a Goal and authorized committing and pushing the existing progress, then pushing each built and verified runnable checkpoint. Keep the pull request in draft while the integrated acceptance gates remain open.

Build a Windows desktop version in which a user can move through every confirmed area of Juxtopposed's design and complete one useful action in each. Use real local media and VLC's native playback, playlist and library models. Start with the dark interface and support the complete journey in one portable build.

The original design and artwork belong to [Juxtopposed](https://www.youtube.com/@juxtopposed). Preserve the existing attribution, original SVGs and bundled font licenses. The [project handoff](../PROJECT_HANDOFF.md) records the current build and native validation results.

## What the first slice must deliver

The central journey is: open Home, add a local folder, browse video or music, search for a file, play it, use the queue and playback panels, return to the library and resume it. Additional short journeys exercise subtitles, effects, waveform, conversion and preferences as those surfaces are confirmed in the reference inventory.

Each confirmed surface must have a reachable entry, its original visual structure, one real action and a way back. Persistent controls must restore their actual state after restart. Empty, unavailable, loading, error and keyboard-focus states must be understandable. Sample data belongs in isolated development fixtures; the runnable app must use the actual models.

The first slice covers the breadth of the design. Subsequent passes bring each canonical surface to the existing visual target of at least 9.5/10, with assessed variants at least 9/10. Functional coverage and visual fidelity have separate acceptance records.

## Current baseline

The Windows x64 Qt 6.8.3 debug package builds and its baseline GUI loads. The redesigned playback bar, queue tabs, library navigation/header/compact rows and four-tab settings panel are implemented. Nine component suites pass. Native backend checks verify library indexing/history, actual equalizer PCM, rendered flip/rotation, current-paused-cue subtitle appearance and duration/expiry/filter cycles with repeated paused seeks at 4× speed; the latest complete GUI journeys await the existing firewall prompt's user dismissal.

Home retains VLC's presentation with a connected native folder picker. Remaining library/title/folder layouts, settings/effects/subtitle appearance, subtitle timeline and presets, waveform, conversion screens, preferences and full light appearance require further design work.

The reproduced paused-seek/reset/resume-clock faults and Windows helper collisions have been repaired and verified with core/packaged-backend tests. Complete desktop startup/playback, native chapter selection, bookmark lifecycle, track/settings interaction, fullscreen and DPI variants remain open.

## Establish the complete design inventory

Milestone 0 is a reference audit, not a declaration that the inventory is already complete. Inspect the supplied walkthrough video and accessible Figma design, then reconcile every unique screen, panel, dialog and reusable control with the repository's preserved exports. Screens displayed inside the walkthrough are reference media, not implemented application UI.

Create a coverage ledger under `doc/juxtopposed-references` with one entry per surface. Record its name, Figma node or video timestamp, reference dimensions, assets, entry route, native model/action, required states and current implementation status. Use these statuses: reference needed, specified, implementing, component verified, native verified and visually accepted. Record any deliberately deferred state separately.

Keep the following surface families as the initial checklist. Split or add entries when the original references reveal more surfaces; do not invent missing screen geometry.

| Design area | Minimum working path for the slice | Current starting point |
| --- | --- | --- |
| Shared shell and appearance | Navigate with the sidebar/header; retain window and panel state; use shared typography, icons, colors, focus and sizing rules. | Existing shell; original artwork and Inter available for playback/queue. |
| Home and library | Add a folder, show real thumbnails/metadata, open an item, return and resume it. | Existing native library models and Home. |
| Video, music and playlists | Browse real items in the referenced layouts, change grid/list presentation, create or open one playlist and play its items. | Native library and playlist infrastructure. |
| Search, browse and source opening | Find a real library item, clear search, open a local file and open a supplied stream address. | Existing filters, browsers and file/network dialogs. |
| Player and fullscreen | Play/pause, seek, change volume, choose tracks and enter/leave fullscreen with pinned or floating controls. | Redesigned playback bar; native runtime checks incomplete. |
| Queue, bookmarks and chapters | Add/reorder/filter queue items, add and seek a bookmark, select a real chapter, then retain the queue when navigating. | Implemented components and adapters; lifecycle/native acceptance incomplete. |
| Audio, subtitles and playback settings | Open the referenced panels, select real tracks, change speed or aspect and return focus to the player. | Existing track picker and interim settings menu. |
| Effects and synchronization | Apply one supported effect and adjust one synchronization value through the referenced layout. | Existing VLC effects and synchronization behavior. |
| Subtitle presets and timeline | Load real subtitle cues, seek to a cue and apply one supported presentation preset. Cue editing/export enters the slice if the confirmed design requires it. | Requires reference and backend capability mapping. |
| Waveform and audio presentation | Render a waveform from actual audio data and keep its position synchronized with one local audio file. | Requires reference and backend capability mapping. |
| Conversion and export | Select a source and one preset, produce a playable output in a chosen location, report progress/failure and support cancellation. | Existing VLC conversion/stream-output capabilities; new presentation required. |
| Preferences, dialogs and themes | Change and persist one setting, return to playback, and verify shared controls in dark/light appearances. | Existing settings; complete design port required. |

If a concept needs backend behavior that VLC does not currently expose, make the smallest real implementation necessary for its agreed path. A mock or disabled button does not complete that surface. Identify the dependency in the ledger and continue independent surfaces while resolving it.

## Implementation order and acceptance

| Milestone | Work | Evidence required to finish |
| --- | --- | --- |
| 0 Reference and state inventory | Complete the screen/control ledger, identify original assets, map native actions and define representative video, audio, chaptered and subtitle fixtures. | Every confirmed reference surface is accounted for; unresolved references are explicit. |
| 1 Stable runtime foundation | Preserve the current working changes, reproduce paused seeking with traditional and redesigned controls, repair the confirmed cause and resolve startup blockers in the used paths. | Native cold/warm startup; repeated pause/seek/resume without reset loops; relevant core/component checks; preserved reproduction logs. |
| 2 Shared shell and Home | Establish shared design tokens/components, port navigation/header/Home, connect real library data and keep the existing player/queue reachable. | Add folder, open media, return and resume; native window captures at normal and narrow sizes; passing incremental build. |
| 3 Library and discovery | Port the confirmed video/music/playlist layouts, search/filter and source-opening routes. Reuse the shared shell. | Real item selection, filtering, playlist operations and empty/loading states; source-index mapping stays correct. |
| 4 Player and queue completion | Complete timeline chapter/bookmark marks, track panels, queue behavior and fullscreen/pinned/floating states. Assess transparency/blur using the actual compositor. | Real chapter and bookmark lifecycle checks; keyboard/focus paths; no stale actions during media switches; native captures. |
| 5 Settings and effects | Replace the interim menu with the confirmed settings/effects/synchronization layouts while preserving native choices and actions. | Setting changes reach the engine; engine/hotkey changes refresh controls; unavailable choices remain correct; focus returns after dismissal. |
| 6 Advanced media surfaces | Build the confirmed subtitle preset/timeline, waveform and conversion paths, one working scenario per surface. | Actual subtitle/audio data; playable conversion output; progress/cancel/failure behavior; backend limitations recorded. |
| 7 Preferences and appearance | Complete the confirmed preference/dialog layouts, persistent settings and light appearance across all implemented surfaces. | Restart persistence; correct contrast/assets; keyboard access; dialogs fit short/narrow windows. |
| 8 Integrated slice and visual acceptance | Run the complete user journeys, close coverage gaps, perform the visual review and package a reviewable portable build. | Every ledger entry has a native pass for its agreed path; component/native checks pass; reviewed captures and explicit fidelity limits. |

Milestones are ordered by dependency. Reference collection and isolated shared-component work can progress while a runtime fault is investigated, but a failing runtime path must retain its failure status. Do not turn off a useful playback feature to obtain a passing result.

## Repeatable work cycle

For each surface, establish its reference and real action before implementation. Keep changes small enough to review. Verify the component's behavior, build the affected native target, stage into a separate test package and exercise the actual Windows app. Compare the native capture with the reference, repair material defects and update the ledger.

Tests should cover outcomes, model/lifecycle risks and regressions. They must expose real production dependencies where practical; a presentation stub can hide a missing Qt import, as the first native launch demonstrated. Record component passes separately from native passes.

Use the existing WSL build at `/root/vlc-juxtopposed-build/win64` and reuse compatible contribs. Keep one writer to that build and honor its lock. Prefer incremental targets. Never edit a helper while it is running. Do not replace a DLL loaded by the user's open player; use a separate test package or close only an identified development instance.

Preserve existing uncommitted fixes. Keep reviewable diffs and source snapshots with each milestone. Commit and push the existing verified GUI baseline, then each built and verified runnable checkpoint, as authorized by the active Goal. Use GitHub and Code Review checks to track the exact pushed revision. Keep the known working portable package recoverable. Publishing releases, merging changes or sending messages to the designer require separate authorization.

## Overnight work

The recommended first overnight target is milestones 0 through 2: a complete reference ledger, a verified runtime foundation and a connected shell/Home slice with a passing Windows build. This is a staged target, not a promise that the entire design will be finished in one night.

A Goal in this local coding chat can carry a concrete objective across continuation turns, subject to its stopping conditions and usage limits. A scheduled follow-up in this same local chat can revisit the next incomplete milestone. A web-only scheduled chat does not provide direct access to this Windows project folder. See [Goals in Codex](https://developers.openai.com/cookbook/examples/codex/using_goals_in_codex) and [scheduled tasks](https://learn.chatgpt.com/docs/automations).

Keep the computer awake, online and the desktop app running. Native screen/input checks require an accessible desktop; do not change lock/security settings automatically. If UI checks are unavailable, continue independent code/build/component work and retain the pending native gates. WSL, network and file permissions may still require review, and account limits can stop progress.

Continue from the ledger and latest evidence rather than restarting. At each verified checkpoint, update the handoff with changed surfaces, checks, failures, captures, build output and the next step. Report completion, a material failure or required input; an optional scheduled follow-up should stay quiet while state is unchanged. At the agreed stop or usage limit, leave the current build, reviewable changes and a concise morning report.

The Goal is active without an explicit deadline or usage budget. No follow-up schedule has been created. The recommended first overnight target above is a staging point within the full milestones 0 through 8 objective, not a reduction of its scope.

## Next concrete action

Continue the remaining confirmed effects surfaces and subtitle presets/timeline using saved exports and the video, preserving real backend actions and separate component/native evidence. The duration control now has its own [native checkpoint](juxtopposed-references/settings/subtitle-duration-checkpoint.md). Extend subtitle appearance to other renderers and persistent preferences, and keep completing the reference inventory. When desktop input becomes available, verify the built folder-picker/browse/search/play/return/resume journey and settings/media-switch/focus paths before accepting those milestones. The coverage ledger and latest checkpoint records identify the remaining full-slice work.
