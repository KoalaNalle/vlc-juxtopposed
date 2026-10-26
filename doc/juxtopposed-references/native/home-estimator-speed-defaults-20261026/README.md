# Estimator speed and saved timing defaults - 26 October 2026

GUI `4d754f60` with Core `ccf72399` scales wall-clock timing marks into native
media time and cancels incomplete pairs across pause/resume. The same corrected
test executable fails the old handlers and passes the new handlers. All 31 native
software subtitle functions pass, including 24 signed speed rows and 12 context
cancellation rows, without late-picture or avcodec-late errors.

The ordinary GUI applies +8433 ms and -7756 ms at 1.5x within independently
bracketed click-time bounds. Reset clears the adjustment. This is UI timing
calibration, not an acoustic synchronization claim. Ordinary same-popup pause
could not be driven by the automation tool's unsupported media keys; the native
same-object cancellation proof remains separate.

An ordinary saved-default/cold-restart journey on the preceding GUI158 and the
same Core224 preserves 23.976 fps defaults while current-media Reset changes the
caption at an identical paused video frame. Three ordinary processes exit 0 with
602 modules, zero error diagnostics and unchanged guards.

Embedded positive-delay coverage now reproduces a separate MP4 held-frame error:
frame 300 becomes 298 at the same reported 12 seconds, then Reset returns 300.
Captions/audio and normal cleanup pass. This remains unfinished; see exact
before evidence, fixture corrections and qualification limits in
[checkpoint.json](checkpoint.json). Raw streams remain local with hashes.
Continue from [STATUS](../../../juxtopposed/STATUS.md); beta gates remain open.
