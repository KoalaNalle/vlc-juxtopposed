# Home video-history actions

28 October 2026. This supplements the [Home section split](../home-video-sections-20261028/README.md).
The owner requested Clear all items for Continue Watching and Watch Again, plus
individual removal. Implementation and scoped checks pass. After the owner's
explicit approval, ordinary individual removal in both sections, Home/full-list
clearing and a fresh-process persistence check also pass, retaining all three
generated library entries. This is not full beta acceptance.

## Implemented behavior

- Home's section options and each full-list header offer **Clear all items**.
  A confirmation names the selected section, includes hidden search results and
  explains that media files are kept. Cancelling changes no history.
- Video context menus offer **Remove from this list**, including missing files.
  Playback remains blocked for unavailable media. Home retains Pin to sidebar.
- Each operation captures its selected video-history mode, snapshots matching
  IDs before deleting, ignores view search/preview limits/paging and rechecks
  membership before removal. Other video/audio history, indexed media, files,
  favorites, metadata and playback preferences are preserved. Errors include
  partial removal; work has busy feedback and an owned/joined worker.
- Successful removal suppresses history/resume writes only for matching open or
  retiring inputs. Stop/Close cannot immediately restore those entries. A fresh
  input can record history normally. Pending playback speed/track/video state
  still saves, with existing privacy/global history guards unchanged.

The active-preference preservation uses a separate per-input marker. Reusing the
global history epoch would discard pending preferences; that review finding was
repaired before the first build. Four native Stop/Close cases independently seed
speed 1.0, change it to 1.5 and verify the saved change with history still empty.
Only speed is directly exercised by that single-video fixture, not live track
changes. Concurrent external history is best effort, not a database-wide snapshot.

## Evidence and boundaries

| Evidence | Result and scope |
| --- | --- |
| [Component](component/NOTES.md) | Both offscreen suites exit 0, no QML warnings, all source/producer guards. Real production pages/delegates with explicit native menu/dialog/model adapters. Includes busy/error/cancel, keyboard/pointer, missing files and section targeting. New page-routing suite is included in the QML CI workflow; it has not been pushed/run remotely in this work. |
| [Native](native/README.md) | Full Home suite: 16 pass, no failures/skips/forced cleanup, all six guards. Real indexed video plus 270 synthetic entries per section, both clears cross the 256-entry boundary, stale actions, opposite/audio preservation, actual open-input Stop/Close and fresh playback. Original r1 failures and corrected fixtures are preserved. QObject worker/UI mutation is outside this suite. |
| [Actual model worker](worker-lifecycle/README.md) | Six real QObject/QThread lifecycle data rows plus setup/cleanup pass: busy/reentrant rejection and captured scope, logical missing-ID error/retry, and owner-thread deletion joining a held worker. A live history-mode cache defect is reproduced on old source and repaired; both failed controls remain frozen. This is separate from an ordinary OS window close or disk I/O error. |
| [Ordinary menus](ordinary-r1/NOTES.md) | Actual app: section/full-list menus, named confirmations, cancellation, search-hidden entries and keyboard Menu/reopen. Exit 0, 603 modules, no checked diagnostics, all seven guards. No UI deletion was performed while action-time approval was pending. Modifier-chord delivery remains unqualified. |
| [Ordinary mutations](ordinary-r2/NOTES.md) | Owner-approved actual individual Continue removal preserves Watch Again; Watch Again's full-list Clear all removes its search-hidden entry. Both sections remain empty after model reload; library count remains 3. Actual QObject worker success path; exit 0, 603 modules, no checked diagnostics, all seven guards. Ordinary storage failure/retry and close-under-load remain separate. |
| [Cold persistence](restart-r3/NOTES.md) | A fresh ordinary process opens the saved post-mutation profile: both sections stay empty, header retains 3 items, and Videos lists all three generated entries after maximizing/navigation. Normal title-bar Close; exit 0, 603 modules, no checked diagnostics, all seven guards. |
| [Ordinary reverse actions](ordinary-r4/NOTES.md) | Fresh original generated profile: actual Watch Again item removal retains Continue; Home Continue's named/default-No Clear all confirmation is then accepted with owner approval. Both histories empty and Videos retains all three entries. Normal Close; exit 0, 603 modules, no checked diagnostics, all seven guards. No additional cold restart in this run. |
| [Library lock](media-library-lock/README.md) | A shared-condition-variable wake defect is reproduced in deterministic and actual Windows controls. Candidate passes 100/500 trials; dependency and plugin are incrementally rebuilt and source/archive/plugin identities retained. The original VLC hang had matching paths, but its exact counters were not captured. |

The ordinary runtime is
`build-windows/beta-home-sections-20261028/history-actions-ui-r4/runtime`.
The approved mutation and cold-restart VLC processes/launchers closed normally;
the separate r3 receipt establishes cold persistence.
Earlier ordinary runtimes and
the alpha fallback remain preserved. No commit, push or release was authorized
or performed in this follow-up.

| Artifact | SHA-256 |
| --- | --- |
| Core | `f09c45fe4075a02836807f00cab03163cdba1ec79e37ba35a0328f7cb6d5462d` |
| Qt plugin | `005fbe744643aa9fea7a45ffa84a11313b32ea454197c118984af413a6844484` |
| Media-library plugin | `e834a41a700bbc6ffeaa83bfde895f5b60f35c2e11f08cd6526d5122afe5af3f` |
| Corrected native test | `999bc9d641dd48f9e34c754664ec626098bff8cfb766959902de82cdd1fa131a` |

Current beta scope remains in [BETA.md](../../../juxtopposed/BETA.md), and the
live next action belongs in [STATUS.md](../../../juxtopposed/STATUS.md).
