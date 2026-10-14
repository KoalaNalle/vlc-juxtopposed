# Player settings and native equalizer checkpoint

Recorded 14 October 2026. Original visual design and artwork belong to [Juxtopposed](https://www.youtube.com/@juxtopposed). This advances milestones 0/5; the complete 0–8 Goal remains active.

## Confirmed references

These are whole frames from the supplied 3840 × 2160 walkthrough, **not captures of the implemented app**. Editorial zoom, animation and compression affect measurements. Do not use reference screenshots or their embedded media as product artwork.

| Surface | Exact source moment | Confirmed structure |
| --- | --- | --- |
| [Playback](playback-reference-09m10.png) | 550s / 09:10 | Playback/Video/Audio/Subtitles tabs, speed value, horizontal slider, eight speed labels |
| [Video defaults](video-reference-09m00.png) | 540s / 09:00 | Flip, rotation, zoom, HDR, resolution and aspect ratio; some tab labels are provisional in the source concept |
| [Equalizer](equalizer-reference-08m50.png) | 530s / 08:50 | Preamp and stacked horizontal band controls with numeric dB values |
| [Subtitle appearance](subtitle-reference-07m50.png) | 470s / 07:50 | Synchronization and appearance, embedded styling, font/colors/offset, relative/pixel size, outline and background opacity |
| [Global Settings](global-settings-reference-17m50.png) | 1070s / 17:50 | App/Media/System categories; General shows appearance, startup, diagnostics and advanced options |
| [Theme setup](theme-setup-reference-19m50.png) | 1190s / 19:50 | A separate first-run step: Light/Dark/System cards, player preview, nine accent circles, Back/Next |

The first-run theme step is a distinct confirmed surface, not the ordinary preferences page. Privacy/security labels in the source are references, not instructions to alter those settings. Additional close frames remain under ignored `build-windows/settings-reference-frames`.

## Implemented behavior

The redesign's settings entry now opens the four-tab panel instead of the interim single list. It retains the existing ControlBar auto-hide lock, menu ownership and focus-return connection. Width 400, preferred height 600, 20px content inset, 52px tabs, 14px Inter labels and rounded numeric/slider controls are **provisional implementation dimensions**, pending original exports and native comparison. Header glyphs currently use VLC's font; they are not exact designer exports.

- Playback uses live `Player.rate` and availability, with numeric input and the referenced eight slider labels. The numeric field retains VLC's supported range through 4×; the ordinary slider shows the reference range through 2× and expands to represent higher active values.
- Video connects real zoom, aspect, crop and deinterlace choice models. Checked-model and hotkey changes refresh selection without emitting another user action. The existing effects dialog remains reachable.
- Audio connects stereo/mix choices, audio delay and an actual ten-band equalizer. Enable, presets, preamp, bands and two-pass settings reach VLC's filter/output variables. Frequencies follow VLC/ISO mode. The class observes engine changes and balances its output/variable references as the output changes.
- Subtitles connects primary/secondary delay and associated-subtitle frame rate. Delay fields display seconds and write integer milliseconds. The frame-rate label describes VLC's actual FPS capability, not an invented speed multiplier. Native synchronization tools remain reachable.
- Numeric fields commit only actual user edits. Unchanged blur, model updates, control destruction and tab switches do not write to the engine. Invalid text restores the engine value. Tabs fit narrow windows, content scrolls and focused controls are brought into view.

The Add action in Music categories now explicitly imports `VLC.Dialogs`, where DialogsProvider is registered. Existing original artwork/font bytes are preserved. No demonstration content enters the app.

## Verification

All nine Qt 6.8.3 component suites pass locally. The new suite constructs the **entire production popup** with supplied controller state and actual Qt controls, checks rate/model synchronization, numeric entry and invalid text, choices, delay units, equalizer actions, tab-switch write suppression, unavailable media, Escape and 100/150/200% scenes. It substitutes VLC registration/controller objects and therefore does not prove the Windows VLC GUI journey. The existing settings-choice suite now checks the new production choice component; the legacy speed component remains tested for its still-used track-menu path.

The native MinGW/QtTest equalizer diagnostic uses the actual packaged LibVLC player, generated audio, memory audio callbacks and the same AudioEqualizer class used by the GUI. It passes engine callback synchronization, consecutive band edits without event-loop loss, disabled-filter preset selection, range/non-finite validation and actual filtered PCM. A 24dB preamp reduction lowers RMS, then disabling the filter restores output. Logs/results are in ignored `build-windows/equalizer-native-checks`. The diagnostic requires `VLC_EQUALIZER_FIXTURE`; default runs without a supplied fixture skip this diagnostic rather than inventing data. A small C bridge accesses the actual internal player without interpreting C atomic fields in C++.

The final native run reports five passes (three cases plus setup/cleanup), no failures/skips, and RMS `0.272227 → 0.015011 → 0.180362` for flat/+12dB, −12dB, then disabled. The generated fixture's envelope varies with time, so these are captured output samples rather than an exact gain-ratio measurement. Native test executable SHA-256: `91380c1a0ce6a110163aa67c9b62b2d98a318b54e38888b5a03109a7562cfb65`.

The native Windows Qt plugin build exits 0. Autotools/Meson register the new class, diagnostic and three QML types; generated resource lists contain SettingsPanel, SettingSlider and SettingChoice. AudioEqualizer is registered as a non-creatable Player-module type. Staged Qt DLL SHA-256: `e886f1a396e538e346b6fbf1804c0a79173b14fe25b8a089dea46bed560550a8`. It is in the separate `build-windows/slice-package`; the preceding DLL is `.before-settings-20261014`, and the user's working package is preserved.

[Playback component](playback-component.png) and [Audio component](audio-component.png) are isolated Qt harness captures with supplied state, **not native VLC application captures**. Other variants are under `build-windows/settings-component-checks` and the full-suite outputs under `settings-checkpoint-previews-20261014`. No fidelity score is assigned.

## Remaining work

Native desktop interaction still awaits the existing Windows Firewall prompt's user dismissal. The new GUI paths, focus return, repeated media switches and short-window interaction need native verification. The native audio diagnostic proves this equalizer backend path, not every GUI action.

The subsequent [video transform checkpoint](video-transform-checkpoint.md) implements flip/rotation and passes native rendered-pixel checks. Video HDR/resolution and the remaining filters, subtitle duration factor/appearance/presets/timeline, full audio effects, Global Settings categories and first-run theme flow remain open. Existing native dialogs preserve access while their final presentation is ported; those links do not complete the missing redesigned surfaces. Complete light/accent behavior, original icons and whole-app visual acceptance are also pending. Continue the complete [vertical slice plan](../../juxtopposed-vertical-slice-plan.md).
