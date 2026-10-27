# Empty-block file output — 17 October 2026 guest

The real file stream-output module treated a zero-byte write as an I/O failure.
An empty block therefore emitted `cannot write: No error` and released the rest
of its chain, including any later payload. The regular-file writer now consumes
empty blocks and continues, matching its existing pipe/socket paths. Nonempty
writes, partial-write handling and genuine failures retain their behavior.

The **identical native test executable** loads the actual file module through
its regular-file `fd` shortcut. It fails five of eight checks against the old
plugin and passes all eight against the rebuilt plugin, with normal parent
exits and only that plugin's identity changed. Checks cover NULL/single-empty/
all-empty input, subsequent payload, empty blocks before/between/after payload,
exact complete bytes, descriptor ownership and a real read-only-fd write error.
See [loaded-module.json](loaded-module.json).

An independent two-second stereo WAV is exported through the ordinary launcher,
real transcode/SoXR/raw mux/file chain. The old plugin produces twelve file-write
errors; the repair produces zero. Both retain exactly **16,000 mono samples**
at 8 kHz. S16 silence allows one LSB of SoXR's default TPDF quantization dither;
the sample count is exact. See [original export](export-original.json) and
[repaired export](export-repaired.json). Both processes exit normally. The same
fixture still reports two missing-visual-callback errors and one PCR-match
error: those separate defects are not hidden or accepted by this repair.

The incrementally built production AudioWaveform fixture passes **8/0/0/0**,
including complete peaks/cache, silence/four-sample audio, validation,
cancellation/source changes, concurrent/paused primary audio and teardown.
It reports zero file-write errors and fourteen missing-visual-callback errors;
see [waveform.json](waveform.json). This is native worker acceptance, not GUI
waveform presentation or independent visual/DPI acceptance.

The complete 30-second PCM regression also passes: raw 8 kHz and WAV 8 kHz each
retain **240,000 samples**; raw 44.1 kHz retains **1,323,000 samples**, every byte
identical to its generated source, including the final packet. All 300 waveform
peaks are present, with mean amplitude error 0.006212 (limit 0.015).
See [complete-export.json](complete-export.json). This does not replace the
separate strict 88,200-sample FLAC regression or accept every conversion codec.

The new plugin adds no DLL imports; core and GUI are unchanged. Autotools module,
C test and waveform builds pass using one job. Both build systems register the
C test; Meson syntax parses, but a native Meson build was not run. The new C test
requires the matching compiler's `libssp-0.dll` in this cross-build runtime;
that dependency belongs to the test executable, not the repaired plugin. Its
initial missing-DLL launch never reached the test and is not accepted. An initial
strict-zero resampled-silence assertion was corrected to account for the observed
one-LSB dither while preserving exact lengths; the recorded export pair uses
the identical maintained runner. [verification.json](verification.json) records
source/binary identities and scope. The ordinary package/previous GUI runtimes
are preserved; raw logs and generated fixtures stay outside Git.

Reproduce with the configured cross-build targets:

```sh
make -C win64/test -j1 test_modules_access_output_file.exe
make -C win64/modules -j1 libaccess_output_file_plugin.la
make -C win64/modules/gui/qt -j1 test_qt_audio_waveform.exe
```

Stage the binaries and dependencies in a separate portable Windows runtime,
set its `VLC_PLUGIN_PATH`, `VLC_LIBEXEC_PATH` and owned `VLC_USERDATA_PATH`, and
run the C test from a fresh writable working directory. The maintained full-chain
check is:

```powershell
python test/libvlc/native_file_output.py --package <owned-runtime> --output-dir <fresh-output>
```

Run the waveform fixture with `VLC_WAVEFORM_FIXTURE` pointing to the independently
generated 30-second amplitude-modulated WAV. The existing
`test/libvlc/native_audio_drain.py` checks all three full PCM exports and peaks.
Frozen guest build/launch scripts and logs are under
`build-windows/beta-audio-output-20261017`. Windows 11, full ordinary GUI/codec
matrix, clean final packaging and the complete beta remain open.
