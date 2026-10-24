# Subtitle duration checkpoint

Recorded 14 October 2026. Original visual design and artwork: [Juxtopposed](https://www.youtube.com/@juxtopposed). This advances milestone 5 and the runtime foundation of the complete active milestones 0–8 plan.

The [07:50 reference](subtitle-reference-07m50.png) shows a duration-factor field with three decimals and a slider. The Subtitles tab now exposes that control using VLC's real `subsdelay` filter. [Top](subtitle-duration-top.png) and [bottom](subtitle-duration-bottom.png) are production-popup renders with supplied state, not native desktop screenshots. The reference's final dimensions and panel placement remain provisional.

## Behavior and engine repairs

The field reads the active filter and its native calculation mode. Mode 1 multiplies source duration; mode 0 adds seconds and displays the `s` unit; mode 2 estimates duration from the text. The range is 0–20; **0 removes the adjustment**, matching VLC's existing synchronization controls. Help/accessibility text explains the current mode and that filter-chain changes apply as cues load. Non-finite/out-of-range values and unavailable output/track states cannot write engine settings.

`SubtitleAppearance` observes the actual output's filter chain, factor and mode. Existing inline factor/mode are normalized to live variables on an edit; the overlap option is retained. Other filters retain their original quoted option text and order, including colons within values. `VideoTransform` reuses the extracted parser. Construction and model updates do not change the filter.

The native checks exposed defects that prevented the control from working reliably:

- Initialize the duration filter's heap before parsing inline options, which can invoke its callbacks. Remove output proxy callbacks before clearing either an empty or replacement chain.
- Transfer intrusive region lists between source and clone instead of copying their list heads. Retain cached regions when a source updater does not regenerate them; propagate original picture dimensions.
- Carry requested stop times in the subtitle source clock domain and reconvert them through the SPU clock on rendering. A private synchronized source/display mapping lets the wrapper update timed source content and overlap calculations in the correct clock domain. New exports leave public structure layouts unchanged.
- Rank all text segments together, including styled cues with an empty leading segment, rather than using only the first segment. Removal restores the original stop time and updater duration.
- Let video frame requests drive further buffering. Subtitle decoders retain access to already buffered cues, without driving paused input to EOF on their own. Avoid discarding newly queued paused subtitles according to elapsed wall time.
- When constructing a paused clock reference, preserve its PCR/input delay instead of adding an output-start margin. At 4× speed, the former 80ms margin put the active cue 30ms beyond the frozen rendering date after a seek to 4.2 seconds. Normal playing outputs still use their existing dejitter behavior.

Temporary tracing has been removed. Native GUI interaction and final compositor/DPI checks remain separate gates.

## Verification

[Final native results](subtitle-duration-results.txt): **11 passed, 0 failed, 0 skipped**, including setup/cleanup. The Windows QtTest diagnostic uses the same GUI class, actual LibVLC/Freetype/duration filter and final 320 × 180 BGRA frames. The [fixture recipe](../../../test/libvlc/generate_slice_fixtures.sh) generates a grey video and two styled green SRT cues at 1–2 and 4–6 seconds.

At about 7 seconds, the unadjusted second cue has [disappeared](subtitle-duration-disabled.png), while the [2× multiplier](subtitle-duration-multiplier.png), [two extra seconds](subtitle-duration-seconds.png) and [text-based factor](subtitle-duration-content.png) keep it visible. Adjusted cues then expire at the expected later times; [expired multiplier frame](subtitle-duration-expired.png). Each mode passes three disable/enable cycles with repeated paused seeks at 4× speed, invalid-value guards and inline-option/quoted-chain preservation. Appearance changes also repaint the same paused cue with the duration filter active. Existing font/colour/outline/background/offset/scale and OSD checks still pass.

Preserved failures show the timing problem [before source-clock handling](subtitle-duration-timing-before.txt) and the repeated-seek failure [before the paused-reference repair](subtitle-duration-lifecycle-before.txt). Full intermediate/final traces and captures are under ignored `build-windows/duration-*` directories.

All nine Qt 6.8.3 component suites pass locally. The production settings suite checks duration values, mode-dependent units/help, user actions, engine-only updates and availability. [Native regressions](subtitle-duration-regressions.txt) pass ES-out, input-clock, clock-start (including repeated paused references with nonzero dejitter at 1×/4×), concurrent Windows pipes, equalizer (5 checks), shared-parser/video transforms (11 checks), and packaged target-pixel/repeated-seek/frozen-time/resume checks. The tracer-dependent full clock executable remains an earlier configuration limitation and is not counted as a pass.

The final core, duration filter, diagnostic and Qt plugin builds exit 0. Matching artifacts are staged in `build-windows/slice-package`; preceding core/filter/Qt DLLs are retained as `.before-duration-20261014`. The user's working `build-windows/package` is preserved.

| Artifact | SHA-256 |
| --- | --- |
| Core | `61329b19cc79cf6a310762a3b95d9120d7dae762ee6f6380b2e5db2c395f1c61` |
| Duration filter | `91ee0add5e75bd5d08e949a150e1a1fec5fa6d78a48e475f1dce213f790c04d5` |
| Qt plugin | `dd3b1362de7d837a6070abb0ee66c845cf9c496d3694c80368855de4d5421136` |
| Subtitle diagnostic | `937dea7193a3b1d761694258a3e6cc58b97d72cad36bf754641d16c08cb150c4` |

Freetype is unchanged from the [appearance checkpoint](subtitle-appearance-checkpoint.md). Code Review supplies CI diagnostics; no independent AI code review is claimed. Local review covered clock domains, observer/proxy removal, list ownership, unrelated filter preservation, validation and both build-system registrations.

## Open gates

The existing native GUI still awaits the Firewall prompt's user dismissal. No desktop interaction, complete native visual score or whole-slice completion is claimed. ASS/image/other text formats, overlapping/multilingual cues, non-default timing rules, hardware rendering, GUI media switches and restart persistence need further coverage. A pre-existing inline filter configuration remains the displayed source until a UI edit normalizes it; synchronizing later external commands on such inline configurations still needs integration. Presets/timeline, remaining effects, Global Settings/theme setup and all other outstanding milestones stay active in the [complete plan](../../juxtopposed-vertical-slice-plan.md).
