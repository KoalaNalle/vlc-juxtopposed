# Subtitle synchronization checkpoint - 25 October 2026

Paused at the owner's request at 19:07 UTC. No owned test/build process remains.
The latest native candidate is Core 9edcbb10, GUI ecaeab25, WASAPI e5560794 and
MKV f46d8d15. It has an unresolved startup failure and incomplete ordinary-app
qualification. [checkpoint.json](checkpoint.json) binds sources and artifacts.

## Established results

- The legacy Synchronization widget now permits normal FPS 0 and blocks model
  feedback from writing rounded/clamped values back. An ordinary GUI-only run
  verifies paused Reset, fractional feedback and seek at 50. Its older Core still
  exposes stale playing captions; this is not latest-tuple GUI acceptance.
- Core flushes only the changed associated subtitle source's old cues and revives
  its EOF source. The same native executable fails playing caption checks on the
  baseline and passes all twelve functions, including setup/cleanup, on the latest
  tuple. Snapshots prove held/playing cues and EOF revival with advancing clocks.
- Mouse movement without Qt/hotkeys reproduces the missing intf-input-blocked
  assertion. Core now owns this temporary flag's lifetime. The same new bare-player
  executable fails on the preceding Core and passes thirteen checks on the new
  Core: decoded frames, mouse events, blocking/teardown, pause/resume and exit.
  The maintained Controls runner exposes the new bare case.
- A WASAPI resume assertion exposed queued audio starting while paused. The new
  module tracks pause state, buffers without starting and cancels stale timers.
  The latest tuple passes eleven timing cases and eight startup cases, including
  the previously failing delayed-video startup. Three repeated audio startups pass.

## Open failure and interrupted work

The single-frame, audio-disabled video ends before its paused snapshot arrives,
with buffer deadlock prevented. Its original failure and all four fresh repeats
fail. Investigate first-picture/EOF/drain ordering and pending frame requests.
No cause or repair is claimed. Fifteen remaining subtitle/track regressions were
prepared but not launched before the pause.

Ctrl+C interrupted the fourth repeated WASAPI startup. No owned worker remains,
but its saved native receipt has teardownComplete=false: normal teardown is
unconfirmed. Its batch JSON retains the last written running status and three
completed passes, not a live job. The ten-case repetition and ordinary current
FPS/default/restart/Quit workflow remain unfinished.

## Evidence corrections and limits

The interim diagnostic-attempts note predates the confirmed input-flag repair.
The r5/r6 FPS setup wrongly compared a raw paused timer after a seek; r7 verifies
the actual frame marker and resumed clock. Later role failures in r5 followed
that early setup return. Four debugger setup attempts stopped before media and
do not qualify normal teardown. The first bare-player staging attempt copied a
libtool launcher; the corrected pair uses the actual .libs executable.

All 2,755 enumerated inputs, their actual frozen copies and the Controls extras
matched before checkpoint documentation changed. Sources before/after the base
commit, structured results, selected generated snapshots and producers are
retained here. Gzip preserves report/XML bytes; the checkpoint records hashes.
Raw logs and binaries remain ignored locally. Diagnostic excerpts preserve the
missing flag/call stack and audio assertion with raw-log hashes. Build warnings
remain recorded. Refresh old inventories before adapting one-use producers.

This runtime includes a private test executable and is not a distributable.
No Windows 10 AUDIOSES, audibility, distinct physical endpoints, final design/DPI
or clean-package acceptance is established. No install, commit, push or release
was performed. Resume through [STATUS](../../../juxtopposed/STATUS.md) and
[BETA](../../../juxtopposed/BETA.md).
