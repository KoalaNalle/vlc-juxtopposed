# Window geometry preference activation — 20 October 2026

The actual Interface editor now has scoped native and ordinary Windows proof.
This is separate from the earlier R3 normal-bounds repair and does not accept
the full beta, offscreen recovery, actual Windows DPI or the final package.

Ordinary GUI `7d50711efded6c0fd79f2e6972a94ca92d4d3809ffdd549d9046f45ccb88708b`;
private GUI `75bbb10ae9edd945a5be890cc492a390b70a97f06528039dfc2dda937e3ed1f5`.
Compiled source matches checkpoint `ed2f5d0155`, frozen `source-r1` under
`build-windows/beta-geometry-activation-20261020`. Protected Core/audio/library
modules are unchanged. No private hooks are present in the ordinary GUI.

- Four accepted native cases /194 checks: initial paused54, cold Off49,
  cold On/reopen48 and isolated playing43. These counts exclude overlapping
  preliminary runs. Actual pointer/Space, strict invalid types, genuine
  locked-save rollback/retry, visible failure and restart feedback, reopen,
  input/queue/tracks/clock/rate/volume/mute preservation all pass.
- Startup policy stays fixed for the current process. An On process saves its
  actual exit bounds even after saving Off for next start. An Off process starts
  at default600x420 and leaves the exact original geometry blob unchanged on
  exit even after saving On. The following On process restores the saved bounds.
- Three ordinary sessions use only Computer Use input and normal Ctrl+Q exits.
  Pointer editing and truthful restart feedback pass; cold Off starts small.
  Windows Maximize then saving On does not overwrite the original bounds while
  the current process's startup policy is Off. Cold On restores1280x900 and the
  editor shows On without stale restart feedback. Independent config/QtINI
  readback follows each normal exit; no native/Qt errors in these sessions.

The original four-case run's playing case passed its assertions and exited
normally, but the strict runner rejected a306ms PCR timing error. An ordinary
test startup overlapped that run; causality is unproven. The error is retained
in `timing-error-playing-r1.json`, not reclassified as an expected error. A fresh
fully isolated playing process passed all43 checks with no native/Qt errors.

Both monitors were authorized by the user for this continuation. No Windows
security/privacy prompt was acted on, and no raw Computer Use capture was saved.
Generated native test captures and local launch commands remain in the ignored
work folder. `verification.json` binds the public receipt files by hash.

Next: popup-duration editor/cold/platform-delivery acceptance. Retain the open
cold-fullscreen lower-edge observation, saved-offscreen recovery, physical screen
unplug, actual OS DPI, multi-instance ownership and final-package checks.
