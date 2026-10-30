# Diagnostic scope of the 29 October caption regressions

The dated caption/role/format receipts classify captured error messages. That
does not claim an empty warning stream or complete audio-content acceptance.
The broader warning review of their generated-fixture logs found:

- MP4 `elst`, STTS and CTTS table messages. The demuxer emits these warnings
  while reading those ordinary metadata boxes; their presence is not itself a
  failed sample or timing assertion. See [mp4.c](../../../modules/demux/mp4/mp4.c).
- The avcodec subtitle opener declines a codec outside its validated list and
  the generic subtitle demuxer declines an unrecognized input before another
  module opens it. The actual caption renders pass separately. See the
  [codec opener](../../../modules/codec/avcodec/subtitle.c) and
  [generic demuxer](../../../modules/demux/subtitle.c). These are module-probe
  outcomes; they are not evidence that every advertised format works.
- `window size missing` in windowless/native test paths, where the display uses
  its default-size fallback; and intentional already-unselected track requests.
- AAC `decoded zero sample` and dropped non-dated audio buffers occur in the
  generated seek/speed fixtures. The [FAAD decoder](../../../modules/codec/faad.c)
  consumes a zero-sample frame; the [Core decoder](../../../src/input/decoder.c)
  explicitly drops a buffer with an invalid PTS. These messages are retained;
  caption/state tests do not qualify their PCM/sample accuracy. The same warning
  families occur in the preceding Core4a role controls, so their presence alone
  is not a newly introduced Coreccb6 failure. Audio-content qualification remains
  a separate beta requirement.
- Four MP4 sample-read warnings in the new role/speed logs occur after the
  explicit `libvlc: exiting` / interface-removal sequence and immediately before
  decoder destruction. Those instances are classified as interrupted teardown
  reads, not proof of corrupted source files during the caption assertions.

No late-picture or avcodec-late warnings were found in the reviewed role,
estimator, lifecycle and new-Core WebVTT/TTML streams. Known test-window-close
errors and the missing-QML runner setup failure remain explicitly classified
in their own receipts. Raw local streams are preserved outside Git. This note
does not close ordinary audio playback, recording tails, physical endpoints,
Windows 10 comparison or final-package stability gates.
