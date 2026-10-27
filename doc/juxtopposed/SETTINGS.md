# Settings consumer audit

This is the detailed companion to [BETA](BETA.md), covering all thirteen designed
settings categories. [STATUS](STATUS.md) owns the current defect and next action.
This audit does not duplicate daily reports or establish full-category acceptance
from one stored value, enabled control or historical test count.

## Per-field contract

For each confirmed editable field/action record: original label/reference; native
variable/model and consumer; units/range/default; platform availability; live,
next-media, next-player or restart timing; profile/global ownership; reset/cancel/
failure/retry outcome; and exact component/native/desktop/package evidence.
Distinguish read-only indicators from editable settings. An unavailable confirmed
consumer stays an explicit gap or approved support limitation.

The existing local discovery index is
`build-windows/beta-settings-audit-20261019/consumer-index.json`. It contains field
candidates and source references; automated discovery can omit actions or include
read-only values. Treat it as a starting index, not a completed behavioral schema.
The archived [detailed audit](archive/2026-10-24/juxtopposed-settings-consumer-audit.md)
retains every earlier receipt and exact result; use current evidence to resolve
its outdated descriptions before selecting work.

## Category matrix

All categories have implemented presentation with scoped evidence. Complete
current-candidate desktop, design/accessibility/DPI and package acceptance remains
open. Native model sources are under `modules/gui/qt/maininterface/`.

| Group / category | Model | Preserve and finish |
| --- | --- | --- |
| App / General | `general_preferences_model` | Appearance/error policy, startup language/update/log behavior and first-run choices. Preserve checked persistence and shared-theme repairs; qualify current ordinary behavior. |
| App / Interface | `interface_preferences_model` | Preserve linked fullscreen-bar, volume hotkey boundaries, title, auto-hide/delay, target-screen, geometry and popup-duration/focus receipts. Finish tray/skin/display/recovery journeys, remaining shared Qt exit writers and OS DPI. Detached-video pairing and `playlistTree` consumers remain unavailable. |
| App / Privacy | `privacy_preferences_model` | Library/history opt-out, clear/reset, worker cancellation, database failure/retry and persistence. Verify every editable/destructive action separately; no stale worker may resurrect cleared state. |
| App / Controls | `controls_preferences_model` | Key/mouse/remote consumers, editable-versus-status distinctions, shortcuts/conflicts, reset/retry, focus and current instance ownership. |
| Media / Playback | `playback_preferences_model` | Preserve track/default/start/repeat/shuffle/speed, precise-seek/thumbnail repairs, native live time-shift write/rollover/cleanup and ordinary picker/cold-persistence evidence. Finish continue/resume/privacy, communication ducking, confirmed disc/menu policies, AVI/MKV and remaining field journeys. Keep broader vanilla-only parity outside beta scope. |
| Media / Video | `video_preferences_model` | Preserve rendered pixel/default/software-snapshot checks and the scoped hardware DX11 caption-snapshot repair. Complete picker/reset/effects/current-media journeys and other hardware/format limits. Wallpaper remains gated; a native option alone does not prove desktop-window behavior. |
| Media / Audio | `audio_preferences_model` | Preserve endpoint/fixed-startup/step/delay, independent Night mode, equalizer PCM and Windows Media role. Finish remaining effects/codecs/visualizations and current app flows. Passthrough, Spatial audio, its mode and adaptive visualization size remain gated. Physical endpoint loss/receiver/routing need separate evidence. |
| Media / Subtitles | `subtitles_preferences_model` | Preserve style/duration/selection/paused-refresh/accessibility, scoped local attachments, asynchronous load recovery and before-cue preset coordination. Finish current-tuple ordinary FPS, secondary/per-track timing, estimators, embedded/external defaults/language, narrow error feedback, online search and cue timeline. `ignoreAssStyling` and `visualizationSize` remain unavailable; another style control does not supply them. |
| Media / Library | `library_preferences_model` | Preserve opening/metadata/root and Library-OFF behavior. Complete current folder/category/detail actions, missing/moved roots, restart/reset/opt-out and playback independence without changing source media. |
| System / Network | `network_preferences_model` | Preserve owned transport/proxy/TLS/artwork results. Trace buffering/default policies, editable/read-only indicators, credentials/global ownership and current app failure/retry behavior. |
| System / Performance | `performance_preferences_model` | Preserve hardware/software/priority/encoder/cache evidence. Verify each confirmed consumer's application timing and visible save-failure/retry on the selected candidate. |
| System / Recording | `recording_preferences_model` | Preserve native recording/discovery/output policy, including the native preference semantics when recording is Off. Finish actual device/picker/lifecycle behavior where supported; an unavailable receiver is not simulated acceptance. |
| System / Extensions | `extensions_preferences_model` | Preserve guest native installation/catalog/cancel/error/retry/shutdown evidence. Complete matching home-candidate discovery/startup/install/remove/lifecycle and visual/platform gates. |

## Ownership and application timing

| State | Contract |
| --- | --- |
| Current media/queue/transport/tracks/rate/volume/mute/fullscreen/live output | Per running player. Another player's saved defaults must not overwrite active state. |
| Saved native defaults | Shared file; merge intentionally changed keys, last successful same-key commit wins. Preserve rollback/retry and both closing orders. Check each setting's actual consumer timing. |
| Media profiles | Playback, audio, video, subtitles and player layout follow the selected Movies/Shows/Music/Work profile. Keep complete application/switch/reset/restart behavior qualified. |
| Profile catalogue | Shared checked catalogue; reject stale edits, retain useful error/draft state and recover by reload/retry. Profiles are not separate executable instances or isolated configuration roots. |
| Theme | Shared saved choice committed explicitly. Existing windows retain local appearance; an older exit must not recommit stale theme. |
| Window restoration | One shared last-closed normal rectangle/window/fullscreen record. Separate persistent identities per instance are not implemented. |
| Privacy, network access, library locations and credentials | App-wide, excluded from media profiles; retain each policy's documented application timing. |

The live device picker belongs to the current audio output; the Audio preferences
device is a startup policy. Default and its named endpoint alias do not establish
two-device isolation. Native config concurrency, shared Qt theme and shared profile
conflicts have separate evidence; do not generalize one repair to all Qt settings.

## Established field semantics to preserve

These are carried-forward scoped native results. Broader ordinary/current-package
journeys stay open unless their specific later receipt establishes them.

| Field | Established application behavior |
| --- | --- |
| Default audio/subtitle track | Native selection, explicit-ID override clearing, paused independence, next input and reopened native instance were tested; caption appearance is separate. |
| Preferred quality | Local adaptive rendition respects the saved cap; broader HTTP/service and desktop behavior remains open. |
| Start playback | Off starts paused. Saving On preserves the current paused item and affects the next input. |
| Saved default speed | Editing the default preserves the current and same-player next rate; a new player uses the new default, with truthful restart feedback. |
| Repeat / shuffle / stop after each | Native finite-input repeat/wrap, random policy with complete nonduplicated traversal, and stopping before the next queue item have bounded tests. |
| Finish action | Native queue completion/manual Stop sends the owner's Quit notification under that policy; desktop process closure still needs its own journey. |
| Time-shift folder/chunk | Next-input constructor consumes saved values; current paused clock is preserved. Native fallback is TEMP/50 MiB. An owned continuous HTTP/raw-PCM source verifies actual writes, 1/2 MiB rollover, buffered sample continuation and cleanup. Ordinary picker Cancel, accented/spaced path, 1 MiB save, cold restore and reopened directory pass separately; other live formats remain open. |
| Preferred audio language | Current and same-player next selection are preserved after a saved default edit; a new player with automatic track policy uses the new language. |
| Audio On/Off | Saving Off preserves current/same-player next audio; a new player can decode video with no selected audio. |
| Saved audio delay | Current offset stays unchanged; the next input/reopen uses the saved delay, with matching help rather than an unnecessary restart warning. |
| Current subtitle FPS | Native 0 means normal external-subtitle timing. Native held/playing/EOF retiming and ordinary cross-editor Reset, fractional feedback, next source and cold saved-default restart have scoped evidence. The latest role/estimator tuple and remaining combinations are identified in STATUS. |
| Secondary subtitle timing | Actual secondary selection gates both current delay editors. Unselection cancels unfinished marks; an unavailable secondary setter preserves the primary. The opening-frame clock/selection repair has separate native and ordinary evidence; broader media/preset/package journeys remain distinct. |
| Music folder / Track order | Songs uses indexed audio-containing folders with an All songs fallback. Folder queries exclude video. Track uses native media TrackId ordering (album/track groups); the existing artist/album track-count criterion remains separate. Disc is metadata only; unsupported independent Disc sorting is disabled. Scoped query, command-lifetime and ordinary cold/keyboard checks do not complete all library categories or design/DPI gates. |
| Artist / genre details | One selected artist opens details without starting playback in both grid and table; explicit Play and multi-selection playback remain separate. Redesigned artist details share history/search/sort/view; traditional details keep their custom header. Native album identity is preserved across equal display titles. Scoped GUI290/native-query evidence does not close all category or package gates. |

Retain signed microseconds, inactive style/duration values, unknown lengths and
category inheritance. Ordinary Save must not replay unrelated presets or clear
activity. Reset must affect its intended scope without stale callbacks restoring
cleared state. Settings search must reveal real fields and preserve edit/focus
behavior in short windows without leaking input to playback.

## Evidence entry points

- [Music detail navigation, playback independence and native album/artist/genre queries](../juxtopposed-references/native/home-music-details-20261027/README.md).
- [Native Playback/Audio consumers](../juxtopposed-references/native/home-playback-consumers-20261019/README.md)
  and [settings activation](../juxtopposed-references/native/home-settings-activation-20261019/README.md).
- [Audio device/startup activation](../juxtopposed-references/native/home-audio-device-activation-20261019/README.md),
  [Night mode](../juxtopposed-references/native/home-audio-night-mode-20261022/README.md)
  and [Windows Media role](../juxtopposed-references/native/home-audio-media-role-20261022/README.md).
- [Title](../juxtopposed-references/native/home-interface-title-20261019/README.md),
  [auto-hide](../juxtopposed-references/native/home-auto-hide-20261019/README.md),
  [fullscreen screen](../juxtopposed-references/native/home-fullscreen-screen-20261019/README.md),
  [geometry](../juxtopposed-references/native/home-geometry-activation-20261020/README.md)
  and [popup focus](../juxtopposed-references/native/home-popup-duration-activation-20261020/focus-repair-r4/README.md).
- [Shared native settings](../juxtopposed-references/native/home-settings-concurrency-20261021/README.md),
  [shared Qt](../juxtopposed-references/native/home-shared-qt-20261021/README.md)
  and [profile recovery](../juxtopposed-references/native/home-profile-error-20261021/README.md).
- [Precise seek](../juxtopposed-references/native/home-precise-seek-20261021/README.md),
  [timeline thumbnail native checks](../juxtopposed-references/native/home-timeline-thumbnails-20261021/native-r96/checkpoint.json)
  and [the latest FPS checkpoint](../juxtopposed-references/native/home-subtitle-fps-20261024/README.md).
- [Ordinary subtitle timing, actual secondary roles and signed estimators](../juxtopposed-references/native/home-subtitle-roles-estimator-20261025-night/README.md).
- [Native integration, conversion and live time-shift buffering](../juxtopposed-references/native/home-integration-20261025-night/README.md).
- [Timing-context cancellation, Escape and ordinary time-shift picker](../juxtopposed-references/native/home-tracks-context-20261026/README.md).
- [Cross-editor timing Reset repair and speed-radio Escape](../juxtopposed-references/native/home-tracks-reset-20261026/README.md).

- [Ordinary startup-rate repair and exact-output conversion](../juxtopposed-references/native/home-startup-rate-conversion-20261026/README.md).
- [Ordinary conversion failure/retry, short-window result and active-worker shutdown](../juxtopposed-references/native/home-conversion-recovery-20261026/README.md).
- [Hardware subtitle snapshot repair and affected native regressions](../juxtopposed-references/native/home-hardware-snapshot-20261026/README.md).
- [Native Qt/player timing-context cancellation](../juxtopposed-references/native/home-timing-context-20261026/README.md).
- [Local subtitle attachment scope, precedence and restart](../juxtopposed-references/native/home-subtitle-attachments-20261026/README.md).
- [Attachment/preset coordination, cold restore and naming viewport](../juxtopposed-references/native/home-attachment-presets-20261026/README.md).
- [Asynchronous subtitle load recovery and before-cue preset restoration](../juxtopposed-references/native/home-subtitle-load-restore-20261026/README.md).
- [Wrapped notices and ordinary accessible attachment errors](../juxtopposed-references/native/home-subtitle-notices-20261026/README.md).
- [Paused cue clocks, exact caption boundaries and ordinary timeline keyboard ownership](../juxtopposed-references/native/home-subtitle-cue-clock-20261026/README.md).
- [Actual compact notices/recovery, fixed-control caption area and open hardware cold-start regressions](../juxtopposed-references/native/home-compact-subtitles-20261026/README.md).
- [Deferred paused refresh, clock recovery, saved-source warm return and current FPS journeys](../juxtopposed-references/native/home-subtitle-clock-refill-20261026/README.md).
- [Separate snapshot export/display, matched-screen evidence and open secondary-screen idle capture](../juxtopposed-references/native/home-snapshot-display-20261026/README.md).
- [Subtitle delay/rate conversion, per-source preroll and temporary-buffer reset](../juxtopposed-references/native/home-subtitle-rate-buffering-20261026/README.md).
- [Estimator speed, same-object pause cancellation and ordinary saved-default restart](../juxtopposed-references/native/home-estimator-speed-defaults-20261026/README.md).
- [MP4 edit-list frame preservation, inherited delays and open fragmented seeking](../juxtopposed-references/native/home-mp4-held-frame-20261026/README.md).
- [Fragment seek/cue restoration, final native matrix and remaining initial paused-caption observation](../juxtopposed-references/native/home-fragmented-mp4-20261026/README.md).
- [Initial paused caption repair, ordinary before/after exports and affected regressions](../juxtopposed-references/native/home-cold-paused-caption-20261027/README.md).
- [Secondary availability, selection cancellation and first-frame video clock](../juxtopposed-references/native/home-subtitle-selection-clock-20261027/README.md).
- [Ordinary late-cue FPS Reset, preset/default, file return and cold restart](../juxtopposed-references/native/home-late-preset-replay-20261027/README.md).

- [Music folder queries, native Track sort, keyboard Play lifetime and ordinary cold/traditional routes](../juxtopposed-references/native/home-music-library-20261027/README.md).

Each receipt retains its own source/runtime/platform limits and failures. Use
[BUILD](BUILD.md) to stage and test the changed consumer; use BETA's final gates
to decide category and package readiness.
