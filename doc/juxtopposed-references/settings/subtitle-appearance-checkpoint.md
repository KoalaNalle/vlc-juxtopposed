# Live subtitle appearance checkpoint

Recorded 14 October 2026. Original visual design: [Juxtopposed](https://www.youtube.com/@juxtopposed). This advances milestone 5 of the active complete 0–8 plan.

The [07:50 reference](subtitle-reference-07m50.png) confirms embedded-format, font, text/outline colour, offset, Relative/Pixels size values, font size, outline thickness and background opacity. Those fields now appear in the Subtitles tab. Numeric/text fields, colour swatches and the two-part size selector reuse the existing local typography and settings styling. The [top](subtitle-appearance-top.png) and [bottom](subtitle-appearance-bottom.png) captures are the production popup in a supplied-state Qt harness, **not native VLC desktop screenshots**. Scroll/focus makes the lower fields reachable within the current popup. Dimensions and the eventual sidebar placement remain provisional.

## Engine implementation

`Player.subtitleAppearance` follows the actual video output, observes its style/margin variables and the player's subtitle scale, and refreshes after track selection. It reads real values, validates user input and makes no engine changes during construction, output attachment or tab switching. The first customization preserves existing font/colour/background defaults before enabling custom appearance. Hex input uses six RGB digits, opacity maps percent to 0–255, and the existing `sub-margin` supplies the additional vertical offset. Positive offset moves the subtitle up.

Relative font size uses VLC's native relative-size convention; pixel size uses the font's pixel em size. Global subtitle scaling is reflected in the displayed size and bounds, and edits account for that scale. Relative outline thickness is a percentage of font size; pixel mode supplies an absolute stroke radius. Both modes retain their own values when switched. These mappings need comparison with exact design exports, particularly portrait/DPI variants.

VLC 4 marks the old Freetype absolute/relative font-size options obsolete. The new supported `sub-text-style-*` options instead feed the active Freetype renderer. Embedded formatting can retain source segment styles or let the user defaults take precedence. Font-family changes refresh the selected face; colour, outline, size and background changes are applied to rendered text. Ordinary playback messages retain their original style.

The core tracks style revisions and subtitle scaling. On a change it pauses background text rendering, invalidates cached subtitle updater formats under the SPU lock, then resumes rendering. Updaters retain the original text, so the **current cue**, including a paused cue, can be regenerated. The renderer is told whether a region is a subtitle or a playback message. This avoids applying subtitle customizations to OSD text.

Availability uses the selected text track and renderer support. Image-based subtitles and ASS's separate renderer need their own appearance integration; these fields are unavailable for those tracks rather than pretending to change their pixels. The native diagnostic currently verifies SRT; other admitted text codecs, font fallback, multilingual content and hardware/DPI variants retain separate gates. Changes currently belong to the output; full app-restart persistence remains part of the preferences milestone.

## Verification

The actual Windows QtTest/LibVLC diagnostic uses the **same SubtitleAppearance class**, rebuilt core/Freetype plugin and final BGRA memory-video frames. The [tracked fixture recipe](../../../test/libvlc/generate_slice_fixtures.sh) generates a grey MPEG-4 video and one long SRT cue with green embedded formatting. Tests explicitly select that subtitle and pause inside the cue before changing appearance; they do not seek to obtain a freshly decoded cue.

[Final results](subtitle-appearance-results.txt): **7 passed, 0 failed, 0 skipped**, including setup/cleanup. They prove:

- Embedded green → user white/red → restored embedded green on the same paused cue.
- Pixel size, relative size, Arial/Courier New family changes and the player's scale update actual glyphs. Captured 28px bounds are Arial `46 × 20`, Courier `128 × 18`; glyph bounds differ from the font's em size.
- Blue outline pixels appear; opaque background increases black pixels from `0` to `3893`.
- A 20px offset moves the observed baseline from `165` to `145`.
- Direct engine-variable colour changes update both the model and picture. Invalid values, output detach/reattach and unselected-track guards pass.
- An OSD message remains white and at its original position despite red subtitle styling.

Captured generated media: [embedded cue](subtitle-native-embedded-original.png), [red text](subtitle-native-red-text.png), [direct engine colour update](subtitle-native-engine-red.png), [Arial](subtitle-native-font-28.png), [Courier New](subtitle-native-font-monospace.png), [blue outline](subtitle-native-blue-outline.png), [offset/background](subtitle-native-offset-20.png), [unaffected playback message](subtitle-native-osd-unaffected.png). These are test-video frames, not full-app captures or designer artwork. Full logs/frames are under ignored `build-windows/subtitle-native-checks`. Set `VLC_SUBTITLE_FIXTURE` and `VLC_SUBTITLE_OUTPUT` to run the diagnostic; without a supplied fixture it reports a skip.

All nine component suites pass locally. The production popup suite now covers the appearance fields, typed hex validation, engine-only updates, units/actions, unavailable state, tab-switch write suppression, scrolling, focus and scaled scenes. Earlier equalizer/video diagnostics also pass on the rebuilt core (5 and 11 checks). ES-out, input-clock, clock-start and concurrent Windows pipe tests exit 0. The packaged paused-seek regression again verifies target pixels, repeated seeks, frozen time and resume. The tracer-dependent full clock test remains an earlier configuration limitation and is not counted as a pass.

The Windows core, Freetype renderer, native diagnostic and Qt plugin builds exit 0. Autotools/Meson register the class and diagnostic; the class is exposed in VLC.Player. Tested SHA-256 values:

| Artifact | SHA-256 |
| --- | --- |
| Core | `8197085d47a3f108db8d57b2781a41ea158d6dee8509aae8249b6d52fb72b53d` |
| Freetype plugin | `e903fd321265935350d63fdcdc7bff7acf983a0c7662fd655205dc897049c6d9` |
| Qt plugin | `31bb38a896373626f5386f8d8a83cf5ccff097b4b5a84435551ae8d067a8a562` |
| Subtitle diagnostic | `0e94d58a88600db9a62a9ba11ff697b97b4b85367d9c2e231dc11575e056fc50` |

The matching runtime/plugins are staged in `build-windows/slice-package`; each preceding DLL is retained with `.before-subtitle-20261014`. The user's working package is preserved.

## Open gates

Native GUI interaction still awaits the existing Firewall prompt's user dismissal. Actual popup/track/media-switch/focus journeys, complete startup/shutdown, app-restart persistence and final visual review remain unverified. Subtitle duration factor, presets/timeline, other settings/effects, full preferences/theme setup and the remainder of the [complete plan](../../juxtopposed-vertical-slice-plan.md) remain active work. No whole-app fidelity score or completion is claimed.
