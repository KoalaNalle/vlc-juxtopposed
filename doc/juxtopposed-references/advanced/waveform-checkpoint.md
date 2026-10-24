# Native waveform worker and player seek bars

14 October 2026. The confirmed [12:18 music reference](0738-music-player.png) and [12:20 waveform detail](0740-music-waveform-detail.png) now have a compiled waveform checkpoint. Original design: [Juxtopposed](https://www.youtube.com/@juxtopposed). The large cover/music-folder layout remains a separate port.

## Working behavior

`Player.waveform` runs an independent native VLC decode on one worker, exports mono S16 at 8 kHz to a private temporary directory, and computes up to 300 peak values across the complete output. The preceding [audio drain repairs](audio-drain-checkpoint.md) retain both the final input packet and resampler tail. No external decoding executable is needed by the app.

The audio player enables analysis for a local file with one audio track and no video tracks. Changing source or disabling analysis immediately clears the presentation and cancels obsolete work. Only the latest queued request remains; generation checks discard late callbacks. Successful peaks are cached for up to eight file/size/mtime identities. Temporary PCM is removed after the worker finishes; reads use 64 KiB chunks. The worker monitors 128 MiB PCM and a 120-second analysis deadline, reports limits/errors, and releases its native player off the UI thread. Destruction cancels and joins the worker before its native parent is released.

The production seek background draws rounded bars from these actual peaks, normalizes their display height, and colours elapsed/future portions using the native slider's position. VLC's existing slider retains seeking, chapter handling, keyboard and accessibility actions. Loading, unavailable or failed analysis leaves ordinary seeking available; the tooltip explains loading/errors. Silence has a minimal visible baseline. The traditional opt-out does not start this analysis.

This first working path deliberately supports one audio track. A multiple-track file retains ordinary seeking with an explanatory tooltip; selected-track analysis and changes remain open work. Network/UNC sources are not analyzed. Very long/slow files can reach the analysis limit. The bars show source audio, not the equalizer's output. Codec priming/discontinuities, source edits during playback and general compressed/multiple-track formats still need broader acceptance.

## Verification

The [native QtTest log](waveform/native-tests.txt) reports **8 passed, 0 failed, 0 skipped**, including setup/cleanup. It uses the same production AudioWaveform class and actual packaged LibVLC/core/packetizer/transcode modules; only unrelated Qt test infrastructure has stubs.

- A 30-second file with a quoted/comma-containing path yields 300 peaks, complete duration and mean amplitude error 0.006220 against the independent generator formula; cache return is synchronous.
- One-second silence yields 300 zero peaks. A four-sample WAV retains all four samples, the exact duration, the full negative peak and half-amplitude sample.
- Missing/network/corrupt sources produce unavailable/error states. Immediate cancellation, ten rapid source changes and a changed file size invalidate stale work/data.
- The independent decode completes while actual primary memory-audio buffers continue, and also while primary playback is paused without changing its frozen time.
- Pending worker destruction completes within the test deadline and processing queued events does not target freed storage.

All **nine Qt 6.8.3 component suites pass locally**. The expanded production playback-bar suite parses all ten playback QML files and checks real saved peak values, variable bar heights, elapsed/future colours, seek actions, scaled/narrow bounds, silence and fallback. The [component preview](waveform/component-preview.png) is supplied-state Qt rendering using preserved native peak evidence; it is not a native VLC window or a fidelity score. [Component verification details](waveform/component-verification.json) retain geometry and messages.

The native test and Qt plugin builds exit 0. `AudioWaveform` is registered in VLC.Player and the generated PlayerController metaobject exposes its property. Both build systems include the new sources, metaobject and native test. The plugin is staged in the separate slice package; the user's working package and preceding Qt plugin remain recoverable.

| Artifact | SHA-256 |
| --- | --- |
| `slice-package/plugins/gui/libqt_plugin.dll` | `8099c1e03e25210a6749160ccd931ac998b993de781740ba00cbfc150c815b31` |
| `core-tests/test_qt_audio_waveform.exe` | `d299cc5f3e908cea85bb0bbef588ac988a4d25516a1ade1a6d9096917f1c6ded` |

Build environment remains WSL Ubuntu 24.04, MinGW x64 POSIX/MSVCRT and Qt 6.8.3. The old plugin is `libqt_plugin.dll.before-waveform-20261014`. Core/subtitle/copy/transcode retain the preceding verified modules. Libtool's cross-host executable-path warnings are unchanged; the copied native test executes successfully on Windows.

For a new native diagnostic run, set `VLC_PLUGIN_PATH` and `VLC_LIBEXEC_PATH` to the slice package, isolate `VLC_USERDATA_PATH`, set `VLC_WAVEFORM_FIXTURE` to the generated 30-second WAV, and run `test_qt_audio_waveform.exe` with an external timeout. The fixture recipe is in `test/libvlc/generate_slice_fixtures.sh`.

## Pending desktop and design gates

The existing owned GUI's Firewall prompt remains open, confirmed again on 14 October. No security permission choices or replacement GUI launches were made. The newest plugin has **not** received desktop startup, music transition, seek/focus/media-switch or whole-window visual acceptance. The original music list/cover/blur geometry, exact bars/assets, light appearance, track variants and the full 0–8 slice remain open. The actual worker/component passes above remain separate from those gates.

The preceding reference/audio-drain commit `994d43bcf22c2dfab664921ac690670560d2aaa3` passed its exact-head Windows workflow through Code Review; its logs show all nine component suites executing. Check this waveform revision separately after pushing it. Next, port the confirmed music cover/list layout and subtitle preset action while completing missing Home/conversion/preferences references.
