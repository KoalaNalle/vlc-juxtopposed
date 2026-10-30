# Native history actions: 28 October 2026

The complete native Home suite passes **16 tests, zero failures/skips** in
[r2](runs/r2/result.txt). This receipt qualifies the shared history-edit helpers
and real primary-player Stop/Close behavior on Windows 11. It does not qualify
the QObject/QThread model worker, QML menus, ordinary application interaction,
Windows 10, visual fidelity or the final distributable.

| Run | Result | Process exit | Forced cleanup | Six preservation guards |
| --- | --- | --- | --- | --- |
| [r1](runs/r1/verification.json) | 14 pass, 2 test failures | 2 | No | All true |
| [r2](runs/r2/verification.json) | 16 pass, 0 fail/skip | 0 | No | All true |

Both runs are terminal. The r1 recorder's `normalExit` is false because its test
process returned 2; it was not killed or silently accepted. The guards cover the
recorded source inventory, generated fixtures, runtime inventory, baseline
runtime, test executable and producer. They do not prove unlisted inputs.
Complete JSON, QtTest text and XML are preserved under `runs/`; no runtime
binaries, raw runtime logs, personal media or settings databases are included.

## Tested behavior

- A real indexed video plus 270 synthetic unfinished and 270 synthetic completed
  videos initially produce 271 Continue and 270 Watch Again entries. Individual
  removal affects exactly one selected item. Whole-section clears remove all
  **270 unfinished / 269 completed** entries remaining after the individual and
  stale-action checks, crossing the 256-row native page boundary in each section.
- A stale action is harmless after an item moves to the other section, including
  a simulated change between the outer read and final player-locked scope check.
  The helper accepts no dashboard limit, search or view-page query. It collects
  IDs before mutation, uses insertion-date ordering, deduplicates IDs and orders
  its edit batches without holding the player lock over pagination/the batch.
- The opposite section, audio history, merely opened video, media count, type,
  subtype, year, duration, title, favorite, saved speed, file references and the
  generated video fixture's actual bytes remain intact. Removal retains indexed
  media and files; it clears native recent/resume/play-count state.
- Four real paused-input cases cover `continue-item-stop`,
  `continue-clear-close`, `watch-again-item-stop` and `watch-again-clear-close`.
  Each row first resets the saved speed to 1.0, changes the actual input to 1.5,
  clears the selected history, then verifies empty history and saved speed 1.5
  after Stop/Close. The opposite entry retains its original progress/count/date.
  A fresh input can save progress again; clearing the opposite section while an
  unrelated input remains paused does not suppress that input's later write.
- A deliberate callback failure among three removals reports two successes and
  one failure, retains the failed item, and succeeds on retry. This is an
  **injected removal failure**, not a real database/disk failure. Missing-ID and
  unavailable-backend paths are also checked.

The original fourteen Home checks also pass in both runs, including the earlier
native history pagination and real EOF/seek/recording boundary checks.

## Corrections between r1 and r2

Only the test source/executable changed. The production Core, Qt plugin and
media-library plugin are identical in both runs.

1. Native progress uses float precision. A saved `.3` reads back as
   `0.300000011921`; the test now accepts a difference below `0.000001` while
   retaining the exact section-membership requirements.
2. A synthetic external video has no known duration. Native
   `Media::setLastPosition()` therefore uses `PositionTypes::Any`: progress 1
   remains progress 1 and does not infer completion. The tests now seed synthetic
   completed state through native Remove From History followed by Mark As Played
   (or Mark As Played immediately after global clearing). Pre-clear membership
   assertions and raw progress/count/date diagnostics verify the fixtures before
   testing preservation. Actual primary-player EOF remains independently tested.

The [test-only correction patch](snapshots/test-corrections.patch) and both exact
test sources are preserved. The r1 source was reconstructed from its recorded
commit plus the saved unified-diff file section; its full SHA-256 matches the
original run inventory. See [recovery proof](source-recovery.json).

## Reviewed implementation and limits

The common snapshots match both runs' recorded source hashes. They include the
public `vlc_player_RemoveMediaFromHistory` API/export, scoped Core marker and
initializer, shared helper, model worker and its borrowed-backend accessors.
Scoped removal sets `input->history_removed`, clears cached delayed resume state,
and suppresses that input's recent/progress/title-position writes. It leaves
history epochs and existing global/privacy checks intact, so ordinary pending
playback preferences still save. Live speed saving is tested; changing live track
selections is not separately qualified by these single-video fixtures.

The model-owned worker captures the requested mode, exposes busy/completion/error
signals, drains during model destruction, and reports partial failures. Those
QObject lifecycle and UI paths were statically reviewed; this native suite calls
the shared helper/Core directly. Ordinary-window evidence belongs separately.
There is no database-wide snapshot transaction: concurrently introduced history
is handled on a best-effort basis, with membership rechecked before removal.

## Identities and reproducibility

| Artifact | SHA-256 |
| --- | --- |
| Core, both runs | `f09c45fe4075a02836807f00cab03163cdba1ec79e37ba35a0328f7cb6d5462d` |
| Qt plugin, both runs | `005fbe744643aa9fea7a45ffa84a11313b32ea454197c118984af413a6844484` |
| Media-library plugin, both runs | `e834a41a700bbc6ffeaa83bfde895f5b60f35c2e11f08cd6526d5122afe5af3f` |
| Test r1 | `e5a5338928947fb571d5e1736a57324280332c9378b085e62e5f3594a5173d0b` |
| Test r2 | `999bc9d641dd48f9e34c754664ec626098bff8cfb766959902de82cdd1fa131a` |

[Summary](summary.json) provides concise run results and identities. `builds/`
contains original exit records, producer/input/binary checksums and the r1 source
patch. That patch includes preceding branch changes and is not a feature-only
patch. R2's test-only build verifies all r1 inputs except the corrected test and
reuses the same Core/Qt binaries. `producers/` preserves the exact guarded build
and native drivers plus their guarded case inventory. Their original one-use
paths are evidence, not instructions to rerun them unchanged.

[Copy verification](copy-verification.json) records byte-identical copies.
`artifact-hashes.json` inventories every receipt file except itself, including
the reconstructed test source and generated summary/patch. The collector creates
documentation only; it does not run a build, test or application.
