# Track resets and Escape - 26 October 2026

Regular Core `883f7e51` / GUI `979fa7b5` passes the repaired ordinary reset
journey and sixteen native subtitle test functions. This is a scoped checkpoint;
the beta session continues.

The preceding ordinary run exposed a primary Reset assertion after the older
synchronization editor had been opened and closed. A relative subtitle edit
emitted its delta while holding the player lock. The hidden older editor then
updated its widget and tried to query the same player. The captured native stack
confirms that path. Audio used the same notification pattern. Relative edits now
ignore empty estimates/missing or stale items and let the queued native callback
publish the final delay after unlocking.

The exact ordinary sequence passes after repair at paused 12 seconds and main
rate 1.5x: external primary plus Finnish secondary, FPS 50, primary +5000 ms and
secondary -9000 ms. Primary Reset reaches zero, preserves the secondary override
and redraws the external caption. Secondary and automatic-FPS resets also pass.
Primary/secondary estimates apply -5517/+3621 ms independently. Audio applies
+4338 ms then resets to zero. Reopened legacy controls display matching zero
values. These marker clicks verify edits and feedback, not perceptual AV sync.
Normal Quit exits zero with unchanged inputs/binaries/fixtures/cache, 602 modules
and no error diagnostics. The native regression separately checks signed exact
units, final-value notifications and observers that reacquire the normal lock.

The prior failed run is preserved honestly, including its assertion image and
stack. Its noninvasive debugger stalled on the last thread and was stopped.
The observed Abort click did not close VLC; only the verified failed test PID
was then stopped. This run has forced-cleanup and failed-exit flags and is never
accepted as a complete journey.

A separate radio-delegate parent binding repairs Escape from selected playback
speed. The native before control fails that new case; the after control passes
all eleven data rows. Actual Escape after selecting 1.5x returns to the track
list with the rate retained. That ordinary observation preceded the unrelated
reset failure, so it does not turn the earlier whole run into a pass.

[checkpoint.json](checkpoint.json) records exact identities, artifacts and
remaining limits. Gzip retains original bytes; only generated media was used.
Continue from [STATUS](../../../juxtopposed/STATUS.md).
