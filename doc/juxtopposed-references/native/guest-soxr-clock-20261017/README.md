# Fixed-rate conversion timestamps — 17 October 2026 guest

The previously traced two-second stereo-to-mono export contained every sample,
but its encoded timestamps started 50 ms late and had 27 adjacent gaps/overlaps.
The WAV/PCR repair had corrected input duration; zero PCR errors alone did not
establish a continuous output timeline.

SoXR's fixed converter now timestamps emitted samples with an exact rational
sample clock, retaining fractional ticks across packets and its final drain.
Flush/discontinuity resets that clock. Source-clock jumps remain represented,
and missing timestamps retain PCM: a later timestamp anchors buffered earlier
audio; an entirely unanchored stream keeps unknown timestamps and complete PCM.
A zero timestamp offset is valid and has its own validity flag.

Non-clocked chains previously selected the adaptive resampler even at a constant
conversion rate. They now perform fixed-rate conversion in the normal filter
pipeline. The final resampler remains available and normally passes through at
1:1. Clocked playback keeps its existing adaptive branch. No public API or ABI
changes are added; both production DLL import sets are unchanged.

The **same native executable** goes from 15 failing cases out of 16 to all 16
passing when only core and SoXR change. It loads the actual SoXR DLL and checks
per-block PTS/duration against an independent cumulative sample clock, exact
sample counts, up/downsampling, fractional rates, stereo/short tails, flush,
discontinuity, repeated drain/new streams, forward/backward clock jumps, missing
initial/later/all timestamps and the real generic filter chain. The adaptive
input-clock/ratio-switch contract also passes. See
[loaded module and chain pair](loaded-module-and-chain-pair.json).

The full launcher/transcode/encoder path changes from **27 gaps/overlaps to zero**,
starts at PTS 1 and ends at 2,000,001 with all 16,000 samples. Both exports quit
normally and have zero PCR errors. See [encoded timestamps](encoded-timestamps.json).
This observation uses a retained diagnostic-only transcode DLL; production PCR
source is unchanged and the ordinary runtime contains the clean transcode DLL
with no trace strings. The clean uninstrumented export independently passes:
[clean-export.json](clean-export.json).

The fresh source-compiled lifecycle diagnostic passes 11 native SDK/PCM and
injected ownership/error cases. Audio preferences pass 32 cases and Waveform
passes nine, with zero write/visual-callback/PCR errors. Complete 30-second PCM
exports retain exact 240,000 / 1,323,000 / 240,000 sample counts, source-rate byte
identity and all 300 peaks. All nine FLAC boundary cases have exact frame counts,
including strict/repeated 88,200; all eight source-rate cases are byte-identical.
See [audio regressions](audio-regression.json), [PCM](complete-pcm.json) and
[FLAC](flac-exact-output.json).

Default redesigned/traditional video/audio and local HLS pass with decoded
frames, Direct3D 11/WASAPI, normal exits and zero QML/write/visual/PCR errors:
[ordinary-runtime.json](ordinary-runtime.json). Final-tuple native Library
18/42/9, Subtitle preferences 37 and controller 4/25/8 pass without failures,
errors, skips or forced cleanup. Their core/GUI/SoXR/WAV/file/transcode hashes
match the ordinary runtime: [native-stage-identities.json](native-stage-identities.json).

The incremental one-job core/SoXR builds and maintained native test build pass.
Both build systems register the test; Meson syntax parses, but no native Meson
build was run. See [verification.json](verification.json) and
[source-identities.json](source-identities.json). Test staging reuses immutable
previous runtime bytes, gives changed files separate copies and uses fresh
portable/cache/profile locations. The ordinary baseline and prior accepted
runtimes remain preserved. This is not a clean rebuild of every beta component.

Reproduce the changed components in the configured build:

```sh
make -C <configured-build>/src -j1 libvlccore.la
make -C <configured-build>/modules -j1 libsoxr_plugin.la
make -C <configured-build>/test -j1 test_modules_soxr_timestamps.exe test_modules_soxr_lifecycle.exe
```

Stage the matching DLLs and native executable in an owned portable runtime,
including the compiler's matching test-only libssp DLL where required, then run:

```powershell
python test/libvlc/native_soxr_timestamps.py --package <owned-runtime> --output-dir <fresh-output>
python test/libvlc/native_file_output.py --package <owned-runtime> --output-dir <fresh-export> --require-clean-visual-callbacks --require-clean-pcr
```

Frozen local recipes and raw evidence are under
`build-windows/beta-soxr-clock-20261017`. This closes the specific observed
fixed-rate export timestamp defect. Broader A/V sync, other resamplers/codecs,
SDI hardware, desktop/DPI/multiple-monitor and independent visual review,
remaining beta settings/surfaces, Windows 11 and a final clean licensed package
remain open. No publication is performed.
