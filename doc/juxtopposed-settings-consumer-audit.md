# Settings consumer audit — 19 October 2026

**In progress. All thirteen category components load and pass their scoped
presentation checks. The complete per-field native/app/package audit is open.**

Keep implementation, native behavior, actual app interaction/visual review and
final-package acceptance separate. A stored option, enabled mock control or
passing component count does not establish that its consumer works.

## Newly verified native behavior

The [Playback/Audio console receipt](juxtopposed-references/native/home-playback-consumers-20261019/README.md)
uses the real production models and Core main-playlist initialization, owned
media and dummy sinks. Its latest accepted fixture is `source-r18`: eighteen
cases, including setup/cleanup, normal exit0 and no Qt diagnostics. All28 Core
logs verify `wdummy`/`vdummy`/`adummy`, no hardware-decoder selection, no native
errors and no unexpected warnings. Earlier R16 logs exposed an obsolete decoder
flag and normal window-provider selection; earlier windowless/software claims
are withdrawn in the linked scope review. R17 repairs the fixture; R18 strengthens
runner checks using that executable. Counts overlap earlier revisions.

| Page and field | Consumer/timing established | Actual desktop/package status |
| --- | --- | --- |
| Playback: default audio track | Actual native stream selection, ID-override clearing, paused independence, same-player next input, native-instance reopen | Open |
| Playback: default subtitle track | Same checks with two embedded SRT tracks; cue appearance is separate | Open |
| Playback: preferred quality | Actual local adaptive rendition240 under cap240; next input/reopen can select the available360 rendition under cap720. Highest logic isolates bandwidth. | HTTP/service and desktop checks open |
| Playback: Start playback | Saved Off starts paused; saving On preserves the paused item and starts the next input; reopen agrees | Open |
| Playback: saved default speed | Current and same-player next input retain1.0 after saving1.5; new player uses1.5; restart feedback agrees | Open |
| Playback: repeat | Actual finite-input current repeat and whole-queue wrap | Open |
| Playback: shuffle | Native RANDOM policy and complete, nonduplicated finite-input traversal; no prescribed random permutation | Open |
| Playback: stop after each | Stops after the first finite input without starting another queued item | Open |
| Playback: finish action | Natural queue completion and manual Stop send the owned instance's Quit notification; changing the policy while paused preserves input/clock. Returning to Stop restores the saved stop-after-item policy on reopen without an exit notification. | Open; desktop process closure not tested |
| Playback: time-shift folder and chunk | Actual next-input constructor reads saved folder/chunk; paused input and clock remain unchanged. Reopen agrees; empty folder/-1 use owned TEMP and native50MiB defaults. | Constructor consumption only; live-source disk writes, rollover and cleanup remain open |
| Audio: preferred language | Explicit English baseline remains on current/same-player next input after saving Finnish; new player selects Finnish with automatic track index | Open |
| Audio: Audio on/off | Current/same-player next audio selection remains after saving Off; new player decodes video with no selected audio | Open |
| Audio: delay | Saving125ms preserves the paused0ms offset; same-player next input and reopen use125ms. Paired before/after testing repairs the unnecessary restart warning and help timing. | Native/component feedback passes; desktop open |

The [Windows Audio receipt](juxtopposed-references/native/home-audio-consumers-20261019/README.md)
separately verifies real MMDevice/WASAPI endpoint selection, fixed startup volume
and native live volume-step actions. Its UI acceptance gates remain disabled.
The live-step restart warning, Audio on/off wording and next-input-delay restart
warning/help are distinct repairs. The current component replay passes20 groups.
The repaired delay model also passes the unchanged Audio defaults/PCM32 and real
Windows4 suites. Ordinary Audio R7 and private origin UI R4 both compile against
matching production source; startup/playback/UI remain unexecuted. These results
remain scoped evidence. Later fullscreen-consumer source has its own candidate.

Paused-clock assertions now establish a stable native value and a no-write
control before requiring exact post-write equality. This does not accept
instantaneous final-position publication immediately after Pause. The rejected
R8 replay and its incorrect seamless-transition observer are preserved.

## Coverage that must still be resolved

| Category | Current audit boundary / important remaining work |
| --- | --- |
| General | Reconcile each live appearance/error policy and startup language/update/log policy with its existing native receipt and current app behavior. First-run settings have separate scoped evidence. |
| Interface | Six earlier wired consumers remain acceptance-gated in MainCtx: geometry, media-name title, fullscreen screen, auto-hide, hide delay and popup duration. The [new embedded fullscreen-bar consumer](juxtopposed-references/native/home-fullscreen-controls-20261019/README.md) passes policy41/native storage15/component10 and actual audio/video On/Off138. Its settings gate activation and ordinary editor check are deferred to the next session. Detached video has no paired controller. `playlistTree` remains unavailable with no tree consumer. Actual tray, skin/picker and display journeys remain separate. |
| Playback | The fields above have scoped native behavior proof. Remaining work includes continue/resume/privacy interaction, communication ducking, disc/menu language, precise seek, thumbnails, optical drive, AVI/MKV, time-shift disk behavior/picker and actual app interactions. Precise seek is gated; timeline thumbnails have no provider. |
| Audio | Native Windows fields and two startup fields are covered in the scopes above. Finish actual UI, other effects/codec/receiver/visualization behavior and remaining field traces. Nine controls remain unavailable in the ordinary app: device, fixed startup volume, step, passthrough, Night mode, Spatial audio/mode, media role and adaptive visualization size. Distinguish pending acceptance gates from absent/platform-specific consumers. |
| Video | Preserve the existing actual pixel/snapshot/defaults checks. Complete the current app picker/reset and remaining field journeys. Wallpaper remains gated; native options do not establish desktop-window behavior. |
| Subtitles | Preserve the renderer/style/shortcut receipts. Complete current embedded/external cue, scope/timing and field interaction checks. `ignoreAssStyling` and `visualizationSize` are explicitly unavailable; do not infer an ASS override from another text-style setting. |
| Controls | Existing native key/mouse/remote policy evidence is bounded. Reconcile editable fields versus status indicators, then replay actual editor/reset/remote interactions with current app ownership. |
| Library | Preserve existing opening, metadata, root and Library-OFF receipts. Complete category/detail and current folder/action journeys without mutating user media. |
| Network | Preserve the owned transport/proxy/TLS/artwork receipts. Reconcile each buffering/default policy, custom/read-only indicators and actual current app interaction. |
| Performance | Preserve hardware/software/priority/encoder/cache evidence. Verify each field's actual live/startup consumer and visible failure/retry on the selected candidate. |
| Recording | Preserve owned recording/discovery/output policy evidence. Actual device/receiver/picker and lifecycle claims need their own scope; unavailable hardware is not simulated acceptance. |
| Privacy | Preserve library/history-off, cancellation, persistence and owned database-failure receipts. Reconcile every editable value and destructive action separately, keeping credentials global and private. |
| Extensions | The current two component checks establish loading/required-property semantics. The earlier [Windows 10 settings/Extensions receipt](juxtopposed-references/native/guest-beta-settings-quality-20261017/README.md) already records five native Extensions journeys/190 checks for its exact guest candidate. Preserve that evidence; current home-candidate discovery/startup/install/remove/retry/lifecycle replay remains open. |

The [current component replay](juxtopposed-references/native/home-settings-components-20261019/README.md)
records all categories' presentation scopes. Earlier native references retain
their exact source/binary/platform bounds; they do not automatically accept a
new package.

## Local source index and execution constraint

The ignored `build-windows/beta-settings-audit-20261019/consumer-index.json`
contains per-page field candidates, direct native declarations and source
consumer references for manual review. Fallback discovery can include read-only
indicators and omit actions; it is not a completed schema or behavioral audit.

The human dismissed the previous firewall prompt with Cancel. Fresh Computer
Use confirmed responsive Home on monitor2 and no permission dialog; the old
isolated app then closed normally. The private runner is gaining an optional
startup hold so each new window can be inspected before test input starts.
Future permission dialogs require human handling. The revised beta roadmap and
its full goal remain authoritative; this ledger neither shrinks nor completes them.
