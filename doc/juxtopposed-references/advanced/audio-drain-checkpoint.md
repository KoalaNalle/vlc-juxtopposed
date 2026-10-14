# Complete native audio export and waveform groundwork

14 October 2026. This checkpoint repairs two actual EOF losses in the packaged native audio pipeline. It provides decoded peak data for the confirmed music waveform reference; it does not implement the music waveform GUI or conversion flow.

The copy packetizer kept one lookahead packet to infer duration from the next PTS, but discarded that final packet at EOF. An explicit NULL drain now transfers it once, retaining its own duration and codec parsing; a consumed input pointer retains lookahead, and an explicit flush still discards it. The native module test exercises those ownership contracts against the actual loaded copy module and is registered in Autotools and Meson.

Audio transcoding now drains the filters/resampler before the encoder. Each returned tail block is encoded and released once, then the encoder drains normally. No input sample, filter, video output or playback feature is disabled to obtain these results.

## Native evidence

The generated mono S16 WAV contains 1,323,000 samples at 44,100 Hz: exactly 30 seconds. Its amplitude-modulated 220 Hz source is defined in `test/libvlc/generate_slice_fixtures.sh`; SHA-256 is `661d2ee20c2ea5952c89f7ec0e295e27cfc29b5ebfb054b49c9ce0a90153921c`. FFmpeg generates development fixtures only; native LibVLC and its packaged modules perform the export.

The same strict [LibVLC diagnostic](../../../test/libvlc/native_audio_drain.py) fails against a separate package containing the preceding copy/transcode modules: [239,362 samples rather than 240,000](audio-drain/before-strict-export.txt) at 8 kHz. The isolated before package shares the other exact DLLs. Neither run changes the user's settings or library.

With both repairs, the diagnostic exits 0:

| Actual output | Verified result |
| --- | --- |
| Raw S16 at 8 kHz | 240,000 samples; complete 30 seconds |
| Raw S16 at 44.1 kHz | 1,323,000 samples; every PCM byte equals the source, including the final packet |
| WAV S16 at 8 kHz | Valid mono header/rate; 240,000 decoded samples; complete 30 seconds |
| 300 waveform peaks | Derived from exported samples; span the full track; mean absolute error against the independent amplitude formula 0.006221 (limit 0.015) |

The full result and peaks are [saved as JSON](audio-drain/native-result.json). The [peak chart](audio-drain/decoded-peaks.svg) shows actual decoded data and is a diagnostic, not an app capture or a visual fidelity claim. The harness waits past the initial queued Stopped state and releases the player only after completion, then checks the fully closed output; an earlier test-start race is not counted as a native EOF failure.

The [copy-module contract test](audio-drain/packetizer-contract.txt) exits 0. Empty/final/repeated/single-packet draining, lookahead timing and flush ownership pass on raw audio. The video submodule also classifies a final WMV3 P-frame during drain, clearing its previous frame-type flag. Other codec/format export variants are not yet separately exercised.

The affected Windows plugins and native test build exit 0. [Plugin build tail](audio-drain/audio-drain-build-tail.txt) and [test build tail](audio-drain/audio-drain-test-build-tail.txt) retain compiler evidence. Subsequent [runtime regressions](audio-drain/regression-results.txt) pass ES-out, input clock, clock startup, concurrent Windows pipes, packaged paused seek/time/resume, [equalizer (5 checks)](audio-drain/equalizer.txt) and [video transforms (11 checks)](audio-drain/video-transform.txt).

## Staged package and reproducibility

Build: existing WSL Ubuntu 24.04 `/root/vlc-juxtopposed-build/win64`, MinGW x64 POSIX/MSVCRT, Qt 6.8.3. One writer holds the existing build lock. The two repaired plugins are in `build-windows/slice-package`; `build-windows/package` retains the user's working build. Each preceding plugin is retained with `.before-audio-drain-20261014`.

| Staged module | SHA-256 |
| --- | --- |
| `plugins/packetizer/libpacketizer_copy_plugin.dll` | `52ebd86e1f2d519492f37b7ccfdde0cde3b3089aba945a4109f2ea1023df585b` |
| `plugins/stream_out/libstream_out_transcode_plugin.dll` | `b6b841ea079d4121fc288bfe43f4436f7bd632042ad8c8cff46e83a67559c984` |

Core/Qt/subtitle modules retain the preceding duration checkpoint. This change has no new Qt presentation; the nine component suites' preceding exact-head CI result remains evidence for that revision, not automatically this new revision.

After generating fixtures and building/staging the native plugins:

```powershell
python -X utf8 test/libvlc/native_audio_drain.py --package build-windows/slice-package --media build-windows/duration-fixtures-20261014/waveform-audio.wav --output-dir build-windows/audio-drain-new-run
```

The output directory must be fresh. Run `test_modules_packetizer_copy.exe` with the packaged plugin/DLL paths and an external timeout for the native packetizer contract.

## Remaining acceptance

Implement a cancellable worker that analyzes real local audio through native VLC, discards obsolete results during media changes, and exposes the waveform to the actual player time/seek controls. Verify silence/short/failed files, bounded resources, shutdown and caching. The redesigned music cover/list/waveform presentation, GUI seek/focus and original visual comparison remain open. Conversion still needs its own source/preset/destination/progress/cancel/failure journey and playable output beyond this isolated export diagnostic.

The existing desktop Firewall prompt still requires user dismissal; no permission choices were made and no replacement GUI was launched to bypass it. Native backend checks above are separate from desktop acceptance. The full milestones 0–8 Goal remains active.
