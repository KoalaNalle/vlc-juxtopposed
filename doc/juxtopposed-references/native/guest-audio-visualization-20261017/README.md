# Audio filter visualizations — 17 October 2026 guest

Generic transcoding and normal audio output share the core audio-filter API.
The generic object has no `visual` variable, but the filter previously tried to
register and remove its selection callback. The core now checks the local
variable's string type, records registration and pairs cleanup in both paths.
It also creates visualization filters only when an output source clock exists.
Generic export/waveform chains have no such clock and must not acquire a
visualization video output; normal playback keeps its visualization behavior.

The identical maintained two-second PCM runner fails against the old core with
two missing-variable callback errors, then passes against the repaired core
with zero. Only `libvlccore.dll` changes. Both launcher processes exit normally
and preserve exactly 16,000 mono samples, with one-LSB S16 dither allowed.
See [core-pair.json](core-pair.json).

A callback-only intermediate candidate still attempted to open an inherited
spectrum visualization in a generic export. That owned probe stalled for 30
seconds and required forced cleanup after its executable path and command line
were checked. It is a failed diagnostic, not normal-exit acceptance; see
[configured-export-original-diagnostic.json](configured-export-original-diagnostic.json).
The completed clock guard makes the configured-visualizer export finish
normally, preserve all 16,000 samples and emit no missing-variable/write error:
[configured-export.json](configured-export.json). The existing PCR-match error
still occurs once in this short stereo-to-mono export and remains open.

The native Audio preferences fixture passes **32/0/0/0**, including a new real
playing-output case that selects spectrum, disables it, restarts filters and
selects scope. Its actual `audio-visual`/`effect-list` mapping and continuing PCM
verify that callback wiring survives restarts. Visualization display uses the
dummy video output; this does not accept desktop visualization rendering.
The actual AudioWaveform fixture passes **9/0/0/0**, including a fresh worker with
global spectrum configured. All 300 peaks and the complete 30-second source are
available, with zero missing-visual/write/PCR errors in this suite. The previous
eight-case worker suite logged fourteen missing-variable errors; the new suite
adds the configured-visualizer case. See [audio-regression.json](audio-regression.json)
and [waveform-diagnostics.json](waveform-diagnostics.json).

The eight loaded-file-module checks remain green. Complete 30-second exports
retain 240,000 / 1,323,000 / 240,000 samples for raw 8 kHz / raw 44.1 kHz / WAV
8 kHz, source-rate byte equality and all 300 peaks. See
[complete-export.json](complete-export.json). Exact FLAC drain/codec and full
conversion acceptance remain separate gates.

The final incremental tuple passes native **18/42/9/37** Library/Subtitle
preferences cases, four NORMAL-lock timeline cases, 25 timer cases and eight
subtitle-controller cases, all zero failures/errors/skips and normal exits.
See [Library](library-regression.json), [Subtitle preferences](subtitle-preferences.json)
and [controller](controller-regression.json). A new ordinary runtime combines
the repaired core, latest popup GUI, repaired file writer and startup launcher.
Default redesigned/traditional video/audio, decoded frames, Direct3D 11/WASAPI
and local HLS pass with normal exits, zero QML diagnostics, zero file-write
errors and zero missing-visual callbacks. Redesigned silent-audio analysis still
logs the independent PCR-match error. [ordinary-runtime.json](ordinary-runtime.json)
records the exact tuple; the baseline package and preceding runtimes are preserved.

The one-job Autotools core and affected native test builds pass. All 450 frozen
core/header/test inputs still match after the build; DLL imports are unchanged.
The first build caught an incorrect assumption about the void callback API and
was corrected before accepted builds. An intermediate full Audio preferences
stage omitted Qt's option provider and failed its maximum-volume check; staging
the provider made the unchanged model/test pass. Neither intermediate attempt
is relabeled as acceptance. [verification.json](verification.json) contains
source/binary identities. Existing test targets remain registered in both build
systems; no native Meson build was run. Raw logs, profiles, generated media and
host identifiers remain outside Git.

Reproduce using the maintained `test_qt_audio_preferences.exe` and
`test_qt_audio_waveform.exe` targets with an owned portable runtime, matching
plugins and the generated 30-second WAV. Set `VLC_AUDIO_DEFAULTS_FIXTURE` and a
fresh `VLC_AUDIO_DEFAULTS_OUTPUT` for preferences, or `VLC_WAVEFORM_FIXTURE` for
the worker. The two full-chain checks are:

```powershell
python test/libvlc/native_file_output.py --package <owned-runtime> --output-dir <fresh-output> --require-clean-visual-callbacks
python test/libvlc/native_file_output.py --package <owned-runtime> --output-dir <another-fresh-output> --require-clean-visual-callbacks --configured-visualizer
```

Frozen guest scripts and raw evidence are under
`build-windows/beta-visual-callback-20261017`. Native desktop pointer/DPI/multiple
monitors, sustained media performance, remaining settings/surfaces, Windows 11,
independent visual review and a final clean licensed beta package remain open.
This is a local incremental checkpoint; no publication is authorized.
