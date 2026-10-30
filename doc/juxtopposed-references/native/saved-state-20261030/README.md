# Saved-state repairs — 30 October 2026

The frozen [review](../../reviews/code-review-20261030.md) identified two shared
settings defects. A stale sidebar catalog could discard newer pins or restore a
removed pin (R1). A post-save readback/rollback could overwrite another process's
accepted preference (R2). Both now have scoped regression and ordinary-app evidence.
The original review remains unchanged; R3's Light-mode contrast is still open.

Sidebar edits compare their accepted catalog snapshot with the physical stored
value. A conflict keeps the accepted rows and pending edit, reports the conflict,
and reloads the peer catalog before Retry replays the key-based edit. INI preference
saves prepare and verify an isolated document under Qt's normal writer lock, then
publish atomically. A newer successful writer after publication is never rolled
back. Failed preparation/publication does not queue the rejected value in the live
Qt settings cache. Existing fallback lookup, groups and primary-key removal remain.

| Check | Original | Repaired |
| --- | --- | --- |
| Stale add/remove/move, removed-pin resurrection | Four intended failures | Pass |
| Newer OS-process save during readback, existing/absent primary key | Two intended failures | Pass |
| Saved-state suite | 28 rows, six intended failures | 31 rows, zero failures/skips |
| Existing profile-store regressions | Separate established suite | 20 rows, zero failures/skips |
| Ordinary sidebar conflict/Retry | Not newly rerun on old Qt | Visible conflict; Retry retains initial, peer and pending pin |
| Cold persistence and shutdown | Separate scope | All three pins restored; three ordinary children exit 0 normally |

The after suite adds writer-lock rejection, failed atomic replacement without a
leaked later write, and removal of a primary override while retaining its fallback.
The before test source is reconstructed and checked against the original compiler
input hash. Its original 28-row oracle is retained as [before-test.cpp](before-test.cpp).
R2's deterministic interleaving uses a test-only Qt sync linker wrapper plus a
separate OS writer. The wrapper is absent from the ordinary Qt plugin.

The ordinary plugin is `a52ecd5c5c8472f4f44d0ea15578696d182c27b604fb099f2292b79d6af619c1`.
Its build refreshes 33 native consumers and two QML units, with 3,556 recorded
inputs unchanged. It is staged beside R29's Core `ccb6a2005ca5` and other selected
plugins. The shared build and R29 runtime stay intact. The initial incomplete
stage and unsuccessful links are preserved locally; only the successful r4 link
is the candidate.

The ordinary external writer changes only the generated test catalog while VLC
is open. Its saved pins survive the failed stale edit, visible Retry, normal close
and cold restart. That establishes R1's app recovery path; the R2 interleaving is
qualified by the native regression, not by a claimed ordinary UI race.

Evidence: [verification](verification.json), [before](before.json),
[saved-state after](saved-state-after.json), [profiles after](profiles-after.json),
[ordinary build](ordinary-build.json), [ordinary journeys](ordinary-gui.json), and
[captured conflict/Retry/restart](../../reviews/graphical-inventory-20261030/README.md).
Compiler maps, exact executables, input media and raw logs remain ignored under
`build-windows/beta-saved-state-20261030` and
`build-windows/graphical-inventory-20261030`.

Every ordinary child reports 603 modules, exit 0, no forced cleanup, and unchanged
runtime/baseline/human-profile guards. This is not a zero-warning claim. Each logs
the existing video-window close notification on normal shutdown. The broader
inventory also records one recursive-layout warning during music teardown/source
replacement and one `subsdelay-factor` lookup warning. Their wider regression
scope remains open; see the inventory's diagnostic section. Windows registry
consumers, all remaining Qt exit writers, OS-DPI, Windows 10 and final-package
acceptance are not established by these checks.

Continue from [STATUS](../../../juxtopposed/STATUS.md) and the single
[BETA roadmap](../../../juxtopposed/BETA.md). Repair completion does not accept beta.
