# Native automatic encoder preference

`native_encoder_preference.py` runs seven serial, bounded Windows VLC processes
against an explicitly selected portable package. Each encodes a generated
two-second mono WAV to FLAC, records both native encoder selections, and decodes
the complete result through the built-in FLAC decoder. Acceptance requires all
88,200 signed 16-bit samples to match, including the nonzero tail.

The cases cover automatic selection, preferred FLAC/avcodec, an explicit FLAC
profile overriding preferred avcodec, an incompatible Vorbis encoder, a missing
encoder, and the automatic `none` sentinel. The incompatible case uses an actual
audio encoder that must reject FLAC without being forced to change its format.

```powershell
python test/libvlc/native_encoder_preference.py --package $package --output-dir $freshOutput
```

The default scope puts `preferred-encoder` in the transcode chain. Use
`--scope global` to exercise the native
`--sout-transcode-preferred-encoder` option in a package containing the new
producer. An older package can be checked with chain scope and
`--expected-missing-preference`; it must ignore the new chain option, still
produce exact audio, and specifically fail to select preferred avcodec.
Per-media options are unsuitable here: the shared stream-output resource is
parented to the player rather than the individual input.

Optional `--core` and `--transcode` arguments replace only the corresponding
files in the runner's fresh owned runtime. Original binaries stay unchanged;
other immutable files are hard linked on the same volume. Plugin caches and
profiles are isolated. A timeout, forced cleanup, abnormal exit, missing
selection, wrong FLAC format, or incomplete PCM fails acceptance.

For the Qt conversion controller, build the existing
`test_qt_conversion_controller` target, or use its opt-in native recipe after
the configured Qt Makefile:

```sh
make -f Makefile -f "$repo/modules/gui/qt/tests/native_library/conversion.mk" \
  -j1 PROPOSAL_OUT="$freshBuildOutput" conversion-controller-native
```

Run the actual `.libs/test_qt_conversion_controller.exe` through
`native_conversion_controller.py`. Its twelve ordered cases include changing
the parent preference synchronously at `Starting`: the first job must retain
avcodec, the subsequent job must use FLAC, and both must validate through the
automatic raw PCM encoder. It also retains actual output playback, cancellation,
publication races, inherited-option isolation and primary-player independence.
The native log callback preserves the complete log for a separate module audit.
The fresh output's generated MOC directory must precede the configured build's
test include directory.

Run `native_flac_drain.py` separately for all nine strict FLAC tail and block
boundary regressions. These checks accept the recorded native audio paths;
they do not establish video/subtitle encoder coverage, the Performance page,
visual fidelity, all media formats or final beta packaging.
