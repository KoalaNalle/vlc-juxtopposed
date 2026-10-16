# WAV packet durations and PCR accounting — 17 October 2026 guest

The final WAV input packet contained 50 ms of real audio but reached transcode's
PCR helper with zero duration. The helper accounted for only 1.95 seconds of
input while the correct encoder/resampler output contained two seconds. It
exhausted its queued input before the final 194-sample tail and disabled PCR
forwarding with an input/output mismatch. No samples were missing in that
particular export; its input timing metadata was wrong.

The WAV demuxer now publishes each packet's duration before sending it, including
the final packet. Sample counts come from the actual block bytes. This also
corrects partial-packet arithmetic that used the unfilled portion of a packet
rather than the bytes read. The packetizer and PCR matching/failure logic are
unchanged. Real mismatches remain errors.

The **same native executable** loads the real WAV module and fails all nine
cases before repair, then passes all nine after repair. It checks PCM stereo,
partial/short tails, a fractional packet duration at 11025 Hz, 48 kHz, one sample,
float stereo, odd-length mu-law payload and a seek before EOF. Every packet's
PTS/DTS/PCR/duration, cumulative sample clock and payload bytes are checked.
See [wav-demux.json](wav-demux.json). Other compressed WAV variants are not
accepted by these cases.

The **identical clean export runner** changes only `libwav_plugin.dll` and goes
from one PCR-match error to zero, retaining exactly 16,000 samples and normal
launcher exits. Both runs observe the actual WAV demux; no trace instrumentation
is present. See [clean-export-pair.json](clean-export-pair.json). The trace-only
comparison independently records the last input duration changing from zero to
50,000 ticks and total input time from 1,950,000 to 2,000,000 ticks. Encoded output
metadata is unchanged: [diagnostic-timing.json](diagnostic-timing.json).

The existing PCR synchronization/helper tests pass, including genuine mismatch
rejection, as does the loaded copy-packetizer contract. Native AudioWaveform
passes 9/0/0/0. Complete 30-second PCM exports preserve exact 240,000 / 1,323,000 /
240,000 sample counts, source-rate byte equality and all 300 peaks. All nine FLAC
boundary cases pass with exact frame counts, including strict and repeated
88,200 samples; all eight source-rate cases are byte-identical to their sources.
The resampled case is also exactly 88,200 frames, checked more strictly than the
runner's historical tolerance. See [audio regression](audio-regression.json),
[PCM](complete-pcm.json), [FLAC](flac-exact-output.json) and
[PCR/copy contracts](pcr-and-copy-units.json).

The ordinary runtime passes default redesigned/traditional video/audio and local
HLS with normal exits, decoded frames, Direct3D 11/WASAPI and zero QML, file-write,
missing-visual-callback or PCR-match errors in these journeys. Native 18/42/9/37
and 4/25/8 controller regressions pass with zero failures/errors/skips and normal
exits. See [ordinary runtime](ordinary-runtime.json),
[Library](library-regression.json), [Subtitle preferences](subtitle-preferences.json)
and [controller](controller-regression.json). The changed plugins in every fresh
native runtime match the ordinary tuple:
[native-stage-identities.json](native-stage-identities.json).

Temporary diagnostic logging was removed byte for byte from the PCR helper;
that production source has no change. The transcode plugin was rebuilt clean
from it, and the shipped candidate contains no trace strings. Its hash differs
from the older retained binary, so the clean comparison uses the same rebuilt
plugin on both sides. Core, GUI, launcher and file writer retain their accepted
identities. WAV module/test and clean transcode builds pass with one job; WAV DLL
imports are unchanged. Both build systems register the new native demux test.
Meson syntax parses; no native Meson build was run. Source and tuple identities
are in [verification.json](verification.json). The ordinary baseline package and
previous accepted runtimes remain preserved; raw logs, profiles and generated
fixtures remain outside Git.

The trace also exposes a **separate open resampler timestamp issue**: nonempty
output totals 16,000 samples/two seconds, but starts at PTS 50,001, ends at
2,044,876 and has 27 adjacency gaps/overlaps. Those timestamps did not change
with the WAV fix. [resampler-timestamps-open.json](resampler-timestamps-open.json)
is a diagnostic, not A/V-sync acceptance. Do not interpret removal of this
specific PCR error as acceptance of all resampler/codec/stream timing.

Reproduce with an owned portable runtime, the matching plugins and fresh output:

```sh
make -C <configured-build>/win64/test -j1 test_modules_demux_wav_timing.exe
make -C <configured-build>/win64/modules -j1 libwav_plugin.la
```

Run the C executable with the owned runtime's `VLC_PLUGIN_PATH`, matching DLLs
and a bounded process timeout. This cross-built C test also needs the compiler's
matching `libssp-0.dll`; the production WAV plugin adds no imports. The maintained
full-chain regression is:

```powershell
python test/libvlc/native_file_output.py --package <owned-runtime> --output-dir <fresh-output> --require-clean-visual-callbacks --require-clean-pcr
```

Local frozen recipes/evidence are under `build-windows/beta-pcr-20261017`.
Actual desktop/DPI/multiple-monitor review, broader media/codec/A-V sync,
remaining settings/surfaces, independent visual review, Windows 11 and a final
clean licensed beta package remain open. Nothing is published by this checkpoint.
