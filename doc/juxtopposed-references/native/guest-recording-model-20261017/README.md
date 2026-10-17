# Recording model foundation — 17 October 2026

The typed model passes **13 ordered native Qt cases** with normal exit, no
failures/skips/forced cleanup or temporary-directory cleanup warnings. This is
a controller foundation. Recording12 is **not mounted or accepted**; media
consumers, native window/chooser, visual/platform and full beta gates remain open.

Actual native config and QSettings cover opening without writes, disabled
consumer gates, strict types/ranges/choices, Unicode recording-folder save and
restart, track/native-recording/x264 preference persistence, all four cast
quality values, custom-value preservation, real Windows config/INI file locks,
rollback/retry, discovery persistence and accepted-OFF notification, CLI
precedence, headless chooser rejection and Advanced dispatch. The source runtime
and cache stay unchanged. This test does not change process scheduling.

Automatic cast quality delegates to the actual VLC platform default. Other
native choices/custom values remain explicit. This adds no adaptive bitrate
feature. The discovery helper defaults ON but is not yet wired to RendererManager;
stopping scans after accepted OFF and preserving an active cast remain pending.
The Stream wizard also needs to consume its saved local-output default.

VLC's input-record-native preference selects original-stream recording when
possible; its fallback can copy/remux too. OFF does not force re-encoding. The
user was asked about that ambiguous original label; this backend retains the
native preference and adds no new encoder mode.

Twelve scoped native-tested inputs match after the build. Autotools resolves the
registered target and three Meson files parse, with the existing src/meson.build
duplicate link_args warning. Two build-registration files bring the source
receipt to fourteen inputs. A native Meson or ordinary GUI rebuild is not claimed.
The first two compile attempts are retained as rejected; corrected R3 compiles.
Production construction and all default-false consumer gates remain separate.

Core is b283af8e99a35562ef790318abd9dd4fdbbd8048fc344f0f436b749659212f12.
The test executable is recorded in [verification.json](verification.json).
Public text is UTF-8/LF; raw evidence hashes and normalized public hashes are
recorded separately. See [results.txt](results.txt), [source-inputs.json](source-inputs.json),
[manifest.json](manifest.json) and the
[reproduction guide](../../../../test/libvlc/native_recording_preferences.md).

Performance14 remains committed and focused-verified as 62d7be5b83. Continue the
active redesign beta with Recording's actual consumers/page, Extensions7 and
the remaining designed behaviors and quality/package gates. Vanilla-only parity
belongs to V1.0, with new integrations tentative for V1.5.
