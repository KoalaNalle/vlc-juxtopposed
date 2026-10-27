# Subtitle rate and temporary buffering - 26 October 2026

Core `ccf72399` corrects delay scaling at nondefault rates, separates external
subtitle preroll from the main media seek and removes temporary track offsets
from the network jitter floor when they are reset. The retained before tests
fail on the old behaviors; the same tests pass the repairs.

The clock contract covers 48 combinations and all 13 clock scenarios pass.
Actual WASAPI playback passes the large-delay/reset/resume regression. Both
software and D3D11 video pass all 30 native subtitle functions; these full
suites use dummy audio and retain two and one startup late-picture warnings.
Official A-B/timer tests and eleven format/delay cases separately pass Core224.

The ordinary candidate exits normally with 602 modules, zero error diagnostics
and unchanged guards after independent timing/Reset, seek/input cancellation
and extended caption playback. Its signed estimator checks do not establish
correct calibration at nondefault speed; rate scaling and pause cancellation
remain next. Embedded positive-delay exact-frame coverage, saved-default/cold
restart and the older cue journey also remain open.

[checkpoint.json](checkpoint.json) records exact identities, scopes, fixture
corrections and remaining risks. Generated evidence, source copies and helpers
are compressed with hashes. Raw diagnostic streams stay local. This is a
development checkpoint, not beta or final-package acceptance. Continue from
[STATUS](../../../juxtopposed/STATUS.md).
