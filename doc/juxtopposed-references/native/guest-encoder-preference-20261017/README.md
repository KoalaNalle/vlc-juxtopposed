# Automatic encoder preference — Windows 10 guest

The new `sout-transcode-preferred-encoder` option orders automatic encoder
selection without forcing an incompatible encoder to change the output format.
Explicit profile encoders still take precedence. Missing or incompatible
preferences fall back to normal selection. Core's new `module_need_preferred`
entry point retains the ordinary module loader's failed-activation resource
cleanup and restores the caller's force flag; existing `module_need` is unchanged.

ConversionController captures the preference before emitting `Starting` and
launching its worker. A change made during that signal affects the next job.
Its internal PCM validation always uses automatic encoder selection.

The matched native candidate is Core
`602632d1eebdeb3559590a5b69315f5672163877fcc742487032d71a25ca1896`
and transcode
`b1a422d272a9486d1366a02841ce9cb9a97bdab7fe611aff24dd19fac4d5a23f`.
Both were built serially in the verified E:-backed WSL environment. The native
Qt fixture compiles the actual conversion controller and records real module
selections through LibVLC's logger; it is not shipped in the ordinary package.

| Native check | Accepted result |
| --- | --- |
| Older Core6823/transcodecfe7, new chain option | Targeted negative: preferred avcodec is ignored; all seven outputs still contain exact PCM |
| New consumer, chain preference | Seven selections and seven complete, byte-identical 88,200-sample decodes |
| New consumer, global CLI preference | Same seven selection and exact PCM checks |
| Actual Qt conversion controller | Twelve ordered cases, zero failures/skips; captured avcodec, subsequent FLAC, automatic raw PCM validation; loaded-module audit passes |
| Strict built-in FLAC regression | Nine tail, block-boundary, repeat and resampling cases; strict 88,200-sample cases remain exact |
| Ordinary matched runtime | Redesigned/traditional finite playback, Library-on startup, local HLS and a bounded Downloads D3D11VA sample; five normal exits |

The seven selection cases are automatic, FLAC, avcodec, explicit FLAC over
preferred avcodec, incompatible Vorbis, a missing name and the automatic `none`
sentinel. Vorbis is an actual audio encoder in this package. The selected FLAC
format is checked independently of selection: 44,100 Hz, mono, 16-bit, with every
decoded sample matching the generated WAV, including its nonzero tail.

The Qt test also retains publication-race protection, cancellation/reuse,
inherited-option isolation, literal/Unicode paths, real output playback, normal
shutdown and independent primary queue/paused time. Reproduction instructions
and the opt-in build recipe are in
[native_encoder.md](../../../../test/libvlc/native_encoder.md).
The receipts and raw/public hashes are listed in [manifest.json](manifest.json).

The following failed attempts are retained under
`build-windows/beta-performance-20261017` and are not acceptance:

- `encoder-build-r1`: a plugin attempted to call Core-private resource cleanup;
  compilation rejected it. Cleanup now stays inside the exported Core loader.
- `encoder-positive-maintained-r1`: a per-media preference did not reach the
  player-parented shared stream output. All payloads were exact, but the chosen
  encoder was wrong. Both paired checks now use the supported chain scope;
  global scope is checked separately on the new producer.
- `conversion-acceptance-r1`: stale configured-build MOC metadata exposed only
  the previous eleven cases. The exact-count runner rejected the result. The
  fresh generated MOC include directory now takes precedence.
- `conversion-acceptance-r2`: the new case ran, but incorrectly expected
  avcodec for automatic PCM validation; the actual native encoder is `araw`.
  The logger callback also replaced the file logger, leaving the separate module
  audit empty. The fixture now preserves its complete native callback log.
  The accepted `conversion-acceptance-r4` requires all twelve cases and the
  independent module audit to pass.

The ordinary GUI is
`753feb6236a3e6457260ee98dfd45e93da1244f6251b5a59e371bcceddc1346f`.
It was rebuilt with all 2,176 selected source inputs unchanged. Of these,
2,169 maintained inputs are recorded in [source-inputs.json](source-inputs.json);
the other seven are the five preserved unrelated untracked files and two unused
ignored HTTP diagnostic prototypes. The ordinary package contains no private
fixture/provider/test executables. The generated playback cases have no QML,
write, visual-callback or PCR diagnostics. The Downloads source and capture
remain private; no large media file was copied, and its stat stayed unchanged.
Both earlier ordinary runtimes are preserved. Meson syntax parsed, with its
existing duplicate-keyword warning; a native Meson build is not claimed.

Performance's category body, priority, plugin rescan and other fields remain
unfinished. These checks cover the native audio
paths above; video/subtitle encoder selection, all media formats, the full
native regression, themes/DPI, Windows 11 and a clean final beta package remain
separate gates. The original ordinary runtime and five unrelated untracked
files remain preserved; nothing is pushed or published.
