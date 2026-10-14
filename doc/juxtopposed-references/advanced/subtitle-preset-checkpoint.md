# Subtitle preset checkpoint

14 October 2026, recorded at 08:53:31 UTC. The confirmed [08:00 preset reference](0480-subtitle-presets.png) now has a compiled persistent subtitle preset path and native verification. Original design: [Juxtopposed](https://www.youtube.com/@juxtopposed). This checkpoint records backend and component results; desktop GUI and Figma visual acceptance remain pending.

Evidence is stored in [subtitle-presets-verification-20261014T085331Z](subtitle-presets-verification-20261014T085331Z/checkpoint-verification.json). Text records use LF. Caption images come from actual VLC/Freetype memory-video output, and the separately labelled component previews use supplied controller state.

## Implemented scope

The Subtitles settings footer offers New Custom Preset, Default Preset, saved names, the selected check, edited state, a name form with Save/Cancel/error retry, and the real advanced preferences route. Empty names cannot save; Enter saves and Escape cancels with focus return. The shared inline name form is provisional because its geometry is not shown in the reference. No unconfirmed preset ellipsis action was added.

MainCtx owns `SubtitlePresetModel`, borrowing the native player, PlayerController's SubtitleAppearance and shared Qt settings. It explicitly deletes the service and unregisters its native listener before settings and PlayerController teardown. Ordinary preferences saves refresh configured Default values without replaying a stored association over live Custom edits. Reset Preferences reloads cleared storage and removes owned live styling; the destructor never writes preset data back. Home's existing reset-only visit clearing remains intact.

Named records save the current native style values: enabled/embedded formatting, font, text and outline colors, background opacity, margin, both relative and pixel size/outline values, subtitle scale, and duration enabled/mode/factor/overlap including inactive values. They also save exact category delay, associated FPS and up to two selected text-track policies with primary/secondary order, stable native ID, language, ordinal fallback and exact own delay or category inheritance. Delay values remain microsecond ticks; no terminated ES pointer persists. Saving a disabled custom style freezes its effective defaults in the record without changing playback.

Persistence uses bounded, validated schema-1 JSON in `Juxtopposed/SubtitlePresets/v1`: at most 64 named presets, 1,024 media associations and 2 MiB. Names are trimmed and unique without case sensitivity. Association identities hash the native MRL. Failed saves retain the previous document and current output; an apply may succeed while reporting that its media association could not be saved. Corrupt or future-version documents are preserved and block further saves until Reset Preferences clears them. Default retains selected tracks and clears all per-ES delay overrides as well as restoring configured category/style values.

Media generation and request serial checks protect output changes and stop/restart. Attachment and snapshot mutation batch notifications and emit after unlocking. Duration snapshots retain initial inline configuration, while subsequent native commands become authoritative. The controller displays category inheritance only for a current secondary role, reads actual native roles rather than stale queued event values, and applies relative secondary nudges from the effective own-or-category delay. Role refreshes preserve each ES's genuine native override.

## Native verification

All final suites report zero failures and zero skipped cases, including setup and cleanup:

| Suite | Passed | Elapsed | Record |
| --- | ---: | ---: | --- |
| Persistent presets, native player/Freetype/QSettings | 10 | 3,324 ms | [Native preset results](subtitle-presets-verification-20261014T085331Z/native-preset-results.txt) |
| Actual PlayerController and main-playlist normal lock | 8 | 879 ms | [Native controller results](subtitle-presets-verification-20261014T085331Z/native-controller-results.txt) |
| Existing appearance and duration regression | 11 | 24,161 ms | [Native appearance results](subtitle-presets-verification-20261014T085331Z/native-appearance-results.txt) |

The preset suite covers save/apply/Default pixels, exact category and own delay, one final snapshot notification, a temporary confirmation failure with exactly one category/track/FPS submission, restart with service creation before output attachment, real track selection, reversed primary/secondary roles, live duration commands, unsupported output, invalid/future storage, failed writes, obsolete media/generation and Reset Preferences without resurrection. [Native runner verification](subtitle-presets-verification-20261014T085331Z/native-preset-verification.json) records **zero input-control FIFO overflows**, fixture/test/core hashes and bounded control counts: 14 category-delay, 18 per-ES-delay and 14 subtitle-FPS controls across the complete journey.

The controller suite uses the production controller and the actual main-playlist player with a normal mutex, real video/two-SRT inputs and the real preset Default path. Direct notification observers re-lock that player; a signal under its lock would deadlock the finite runner. It verifies own 650,000 ticks, inheritance of category 350,000, category updates, a +50,000 nudge producing native 400,000/UI 400 ms, exact negative microsecond timing, native role swaps before queued GUI cache refresh, Default clearing both overrides, unselection and media reset. This is a native QObject test with existing unrelated test support, not a desktop window journey.

The renderer regression retains actual font/unit/scale, color, embedded formatting, outline/background/margin and OSD checks. It also exercises disabled, multiplier, extra-seconds and text-content duration modes through repeated enable/disable/seek cycles. Mode 0 adds its factor in seconds to the source duration; it does not replace the entire cue duration.

These are **native caption captures only**, each a 320 × 180 memory-video frame with no desktop window or preset menu:

- [Saved preset: actual red caption](subtitle-presets-verification-20261014T085331Z/native-caption-saved-red.png).
- [Default: actual embedded green caption](subtitle-presets-verification-20261014T085331Z/native-caption-default-green.png).
- [Fresh player/settings restart: actual restored red caption](subtitle-presets-verification-20261014T085331Z/native-caption-restart-restored-red.png).
- [Extra-seconds duration: actual visible caption](subtitle-presets-verification-20261014T085331Z/native-caption-duration-extra-seconds.png) and [disabled duration: actual expired caption](subtitle-presets-verification-20261014T085331Z/native-caption-duration-disabled-expired.png).

## Initial failures and repairs

The [initial native record](subtitle-presets-verification-20261014T085331Z/native-initial-failures.txt) passed setup, save/apply/Default pixels and notification batching, then failed a later restart capture, track-selection assumptions, reversed roles and an unavailable Default application. The [runner timed out after 55 seconds](subtitle-presets-verification-20261014T085331Z/native-initial-timeout.txt); its [FIFO excerpt](subtitle-presets-verification-20261014T085331Z/native-initial-fifo-excerpt.txt) shows overflowing category-delay/per-ES-delay/FPS commands.

The production retry path wrote settings, failed its final live capture, kept the pending request and queued another refresh. Those writes also generated native callbacks, allowing another complete write cycle. The repair gates mutation on actual selected outputs/roles, submits writes once per request, and makes subsequent callbacks confirm the resolved native snapshot through reads. Generation/serial timeouts are bounded; unsuccessful confirmation cannot replay submitted writes. A native counter test interrupts first confirmation, restores readiness and performs repeated refreshes while asserting exactly one submission of each control.

Two track assumptions were also corrected. `GetSelectedTrack` returns the first selected enumeration entry rather than the primary role, and forced subtitle attachment can retain more than one selected track. The fixture now explicitly selects a sole second ES and checks actual output roles. In addition, the native `EsOutSelectList` skips already-selected ESs, so merely reversing the same two IDs does not reverse their roles. Genuine role changes now queue bounded deselection before the desired ordered selection, then wait for actual callbacks, retaining each track's delay.

The first restart had already restored the saved values and produced red pixels before its later capture became empty. The repaired fixture pauses and seeks into the cue before capture or reselection, avoiding assumptions about cached subtitle timing while playback continues. That initial pixel failure does not establish that mode 0 shortened the source cue to 1.5 seconds. The final restart capture and complete native suites above are the accepted evidence.

## Component evidence and remaining gates

All **13 component suites passed locally**, as reported by the root run. The included [preset component verification](subtitle-presets-verification-20261014T085331Z/component-verification.json) exercises the production QML with supplied controller state: named/Default actions, empty-name guard, retained-name retry, Enter/Escape/focus, selected/edited/unavailable states, advanced route and three sizes/scales, with no QML messages. [Menu preview](subtitle-presets-verification-20261014T085331Z/subtitle-presets-520x700-1.png) and [short name-form preview](subtitle-presets-verification-20261014T085331Z/subtitle-preset-name-320x480-1.png) are **component-only previews**.

The initial preset Qt plugin built successfully at `build-windows/staged-presets-libqt_plugin.dll` and was staged in `build-windows/slice-presets-package/plugins/gui/libqt_plugin.dll`, both with SHA-256 `d5bd60835b5e1ba30d95aefa5268fbcb2a9941f65da2a74923b9a8a05c5a073b`. This hash identifies the initial preset build, not acceptance of the active desktop package.

After the user manually dismissed both Windows Firewall prompts, the root desktop check found an `MLBaseModel::setMl` assertion in Home's Continue Watching → See All route: an inactive History model was explicitly bound to `ml: null`. Both History models now retain `ml: MediaLib`. The rebuilt Qt plugin SHA-256 is `364bfe40d5af74435b08421a6fe1032fa8338158c8f208cda3652e1b3a500abe`; it is staged separately and runs in the Home development package. Its real video/audio history routes, search/sort/view switching and retained audio return pass the [Home desktop checks](../home/desktop-checkpoint.md). Preset/controller production behavior is unchanged. No security UI automation is recorded.

The [separate Windows caption evidence](subtitle-presets-desktop-20261014/verification.json) uses the actual native settings popup with pinned video controls and private portable preferences. Disabling embedded formatting and entering `#FF3333` changes the real paused caption from green to red; the native service reports Custom settings. This proves the integrated subtitle tab/style path. It does not establish the footer menu Save/apply/restart journey: automation attempts to scroll/navigate the transient panel dismissed it and are not counted as successful menu checks. The native service save/restart results above remain separate evidence.

The integrated Windows menu still needs a Save → edit → apply → quit/restart → Default journey, secondary timing/focus/error interaction and fresh output/media-switch checks. Exact Figma geometry, light/scaled/native window presentation and unshown name/ellipsis behavior remain unaccepted. Language/ordinal fallback across different input layouts, nonstandard global Freetype defaults, other supported text codecs, bitmap-only subtitles, external subtitle loading failures and alternate renderer/hardware combinations are not established by these fixtures.

The separate [06:06 file/folder/playlist subtitle attachment scope](0366-subtitle-scope.png), [06:59 cue timeline](0419-subtitle-timeline.png) and [08:35 default-track/language fields](0515-subtitle-defaults.png) are not completed by this preset checkpoint. Per-media preset association is distinct from attaching a subtitle file to a file/folder/playlist. This checkpoint does not establish the complete milestones 0–8 slice or whole-app visual fidelity.
