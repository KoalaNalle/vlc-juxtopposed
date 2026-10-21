# Windows input cancellation regression

This opt-in test uses a staged Windows x64 VLC build and existing Python. It
does not install dependencies or touch user media/configuration. Run from the
repository root, using an unused output folder under `build-windows`:

```powershell
python -X utf8 test/libvlc/native_input_cancellation.py `
  --package build-windows/beta-input-cancellation-20261021/runtime-r1 `
  --source-inputs build-windows/beta-input-cancellation-20261021/inputs-r1.json `
  --output-dir build-windows/input-cancellation-replay
```

The package must contain matching Core, cache filter and Windows plugins. The
optional source manifest maps repository-relative files to SHA256. Each child
gets an isolated config, plugin cache, logs and generated silent fixtures. The
parent removes `VLC_PLUGIN_PATH` before launching Python: removing it only after
the child C runtime starts can leave a duplicate search path visible to VLC.

Four separate native LibVLC processes cover:

1. Eighteen Start/Stop cycles at delays 0, 1, 4, 10, 25 and 60 ms, repeated three
   times. A fresh callback acknowledgment is required for each Stop; polling an
   initially STOPPED player is insufficient. The log must show both empty and
   nonempty prebuffer outcomes. The same player then plays a fresh input, seeks
   to five seconds, and advances time and played buffers afterward.
2. An empty file must reach Error and preserve the real prebuffer error.
3. Invalid nonempty input must reach Error and retain native diagnostics.
4. Valid WAV bytes named `.mp3` must select the WAV demuxer and play/seek cleanly.

The callback structure includes every field in public version 0 and checks its
order against the matching header. Callback functions and the structure live
through player release. Callbacks only record state; they do not call back into
the player. VLC 4 native time and seek arguments are **microseconds**. Playback
must advance beyond initial clock samples and increase decoded/played counters.

Successful playback cases require real MMDevice/WASAPI, no native errors,
assertions, Python traceback or callback exceptions, one cache load, unchanged
runtime/fixture/source hashes and normal exit. Empty/invalid errors are expected
only in their own cases. A 120-second child timeout records forced cleanup and
fails the test; it is not accepted as normal shutdown.

The accepted 21 October run has four normal processes and 18 cancellations,
including before and after data arrived. It does not qualify GUI behavior,
audible quality, arbitrary misnamed extensions, every protocol, separate physical
outputs or the complete beta. The `.flac` hint is a known distinct diagnostic
case; do not weaken error checks to include it as a clean result. Frozen source,
runner history and receipts are in
`doc/juxtopposed-references/native/home-input-cancellation-20261021`.
