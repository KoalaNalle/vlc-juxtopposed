# Native audio regression evidence — 14 October 2026

Three native gates passed for the FLAC and SoXR repairs. This bundle records the
actual results, source/runtime identities and prior failures. It establishes
the scopes below; it does not establish completion of the redesigned beta.

| Gate | Actual result | Scope |
| --- | --- | --- |
| SoXR lifecycle | 11 cases, exit 0 | Production filter source compiled into a native harness, actual libSoXR and VLC blocks, exact independent SDK counts/full PCM for rate transitions and true bypass, flush and explicitly injected ownership/allocation/process/Gather failures. |
| FLAC packetizer | 7 cases, exit 0 | Actual loaded packetizer with native encoded one-sample and 4097-sample frames, fragmented input, repeated complete drain, original packet bytes/PTS, truncated/CRC rejection and flush recovery. The shared header reader also faces a protected-page malformed blocksize test. |
| Complete FLAC output | 9 cases, exit 0 | Actual staged built-in encode/decode retains every sample for 1, 17, 4095, 4096, 4097, 8209 and 88200 samples, a repeated 88200 run, and complete 8000→44100 Hz resampling. |

The first eight complete-output cases require exact decoded PCM. The resampling
case requires the complete expected 88200 frames; its result deliberately has
`exactPcm: null`. The independent SoXR transition gate checks every finite PCM
sample within the unchanged tolerance of `1e-5`, with exactly 192048 samples for
the buffered variable-rate sequence and 144000 for the true-bypass sequence.

The SoXR and complete-output gates selected
`build-windows/slice-beta-native-package-20261014`. Packetizer7 selected the
immutable `build-windows/slice-flac-complete-package`. Its recorded core and
packetizer DLL bytes match the staged package. That identity comparison is not
an additional packetizer execution claim. The complete-output resampling log
records the actual loaded `soxr` resampler.

`source-runtime-manifest.json` records exact source, executable and runtime
hashes. `evidence-index.json` records the SHA-256 of each original local evidence
file and its separately hashed publishable copy. Copied JSON values and log
paths use repository-relative or generic build-root labels; original hashes
remain unchanged. The local runs and historical source snapshots remain under
ignored `build-windows` paths. No userdata or personal media is copied here.

The final `soxr-extra-build.log` and `flac-packetizer-extra-build.log` contain
actual C compilation and linking of their accepted targets. The build owner
recorded exit 0, then copied the executables whose hashes appear in the native
results. Libtool also prints host-path diagnostics followed by “Continuing”; the
subsequent actual native runs exited 0. The module build excerpt contains actual
SoXR compilation/linking. The four FLAC/ML targets in that excerpt were already
up to date; this bundle does not relabel them as newly compiled. A source hash
alone is not compilation evidence.

The retained history distinguishes defects from test-fixture corrections:

- Short FLAC EOS frames, malformed two-byte blocksize reads and an EOF demux
  that stopped after its first drained packet caused missing output. The final
  gates require complete data, including the previously lost 4097th sample.
- The bundled variable-rate SoXR delay query reports a placeholder 100 samples.
  Clearing after one 102-sample drain discarded 186 more native samples. The
  repaired filter drains bounded chunks until the actual SDK returns zero,
  checks totals and preserves ownership and initial-allocation retry behavior.
- An initial nominal-count oracle was invalid for instantaneous buffered ratio
  changes: the independent SDK itself produces 192048 rather than nominal
  192000. Its failed run and source remain identified in `history-index.json`.
- The first independent PCM oracle omitted the SDK's explicit initial ratio
  setup. A separate trace found 95848 mismatches with create-only initialization
  and zero mismatches with the documented initialization used by production.
  The final strict oracle retains the same tolerance and 11 cases.
- A loader failure and rejected/stale build attempts remain historical evidence
  and are not counted as native passes.

The SoXR fault cases inject errors at production source API call sites. They do
not reproduce actual system-wide OOM or inject errors into a loaded SoXR DLL.
The ordinary complete-output case does use the loaded staged resampler. These
gates do not verify arbitrary media/rates/hardware, desktop playback, the native
QML panels, Figma fidelity or the new subtitle-cue bridge prototype.

## Reproduction

Use Windows x64 Python 3.11 or later. From the checkout root in PowerShell,
these commands resolve every helper/package/output argument to an absolute
Windows path without embedding a particular developer's checkout location.
Use fresh output directories; the helpers refuse existing ones. The selected
packages and matching test executables must already be built and staged.

```powershell
$audioCheckout = (Resolve-Path .).Path
$env:VLC_LIB_PATH = "$audioCheckout\build-windows\slice-beta-native-package-20261014\plugins"
Remove-Item Env:VLC_PLUGIN_PATH -ErrorAction SilentlyContinue
python "$audioCheckout\test\libvlc\native_flac_drain.py" `
  --package "$audioCheckout\build-windows\slice-beta-native-package-20261014" `
  --output-dir "$audioCheckout\build-windows\audio-proof-full-output-FRESH"
python "$audioCheckout\test\libvlc\native_audio_extra.py" `
  --package "$audioCheckout\build-windows\slice-beta-native-package-20261014" `
  --test-exe "$audioCheckout\build-windows\core-tests\test_modules_soxr_lifecycle.exe" `
  --output-dir "$audioCheckout\build-windows\audio-proof-soxr-FRESH"
python "$audioCheckout\test\libvlc\native_flac_packetizer_extra.py" `
  --package "$audioCheckout\build-windows\slice-flac-complete-package" `
  --test-exe "$audioCheckout\build-windows\core-tests\test_modules_packetizer_flac_eos.exe" `
  --fixtures "$audioCheckout\build-windows\audio-proof-full-output-FRESH" `
  --output-dir "$audioCheckout\build-windows\audio-proof-packetizer-FRESH"
```

The packetizer helper requires a fully passing nine-case FLAC result and its
real one-sample/4097-sample fixtures. Each helper records its newly selected
runtime hashes; a new run with different bytes is separate evidence.

In an already configured MinGW x64 out-of-source build, the explicit Automake
targets are `test_modules_soxr_lifecycle.exe` and
`test_modules_packetizer_flac_eos.exe`:

```sh
make -C /build/vlc/win64/test test_modules_soxr_lifecycle.exe
make -C /build/vlc/win64/test test_modules_packetizer_flac_eos.exe
```

Replace `/build/vlc` with that configured build root. These opt-in targets are
registered in `test/Makefile.am`; they are not part of default `make check` and
this checkpoint does not claim Meson registration. SoXR requires the actual
configured SDK dependency. The Win32 target flags are
`-static-libgcc -no-pthread`, with `-l:libwinpthread.a` after the target's native
library dependencies. Retain successful actual CC/link output, stage the
matching executable/core DLLs, and verify imports. An up-to-date message or a
relink of a stale object does not establish a new source compilation.
