# Recent with Library browsing OFF — 18 October 2026 guest

Recent now reads VLC's existing history with Library browsing OFF. Sidebar
availability does not open a database; only an explicit Recent visit opens the
shared history backend asynchronously. A failed open appears on the actual page
with Retry. Local and HTTP entries reopen through native typed IDs. Playlist
activity browsing stays disabled while Library browsing is OFF.

HistoryMediaSource owns its Qt frontend and borrows VLC's shared backend. The
opening worker finishes before the interface is released. Its frontend drains
running model reads, suppresses queued callbacks and skips Home activity and
thumbnail generation. Minimal QML reader types register independently of the
normal Library frontend. The model receives a source only after it exists.

Windows 10 guest native checks pass **800** across seven normal
process exits: history-off-optout 24, history-off-shutdown 10, populate 298, reopen 178, disabled 102, replacement 106, history-off 82. All Qt warning
lines and selected diagnostics are zero. New cases verify no database on opted-out
startup, locked existing storage and actual Retry, decoded local/loopback HTTP
reopen, paused playback independence, live clear with no stopped-input resurrection,
continued saving opt-out, and destruction during asynchronous opening.

Thread-runner **34** cases pass, including both new draining thread arrangements.
Five ordinary playback cases (modern, traditional, Library, localHLS, read-only
Downloads) pass normally with decoded output and all Qt warnings zero. The offered
Downloads file's size/time is unchanged; its name and frames stay private.
Production 1926, private five and thread four inputs
match. Retained 458 Core/eight store inputs match; those suites were not rerun.
The ordinary GUI is `e2ec064b905f916c260b321f7b702d1a67f0e168f41f36cad879dbe586980533`.
Core0cdebe and media-library e3ec5e are unchanged. The prior runtime and generated
media are preserved; ordinary GUI test/diagnostic markers are absent.

## Reproduction and rejected attempts

All builds/tests/profiles/cache/output use E:. Use the migration Common.ps1 and
VLC-Juxtopposed-Travel, one heavy task at a time, JOBS=1/make -j1. The Linux legacy
/mnt/c source alias resolves to /mnt/e inside the E: virtual disk. Reproduction
scripts and exact frozen inputs accompany this proof. The ordinary runtime is
E:/Modding/vlc-juxtopposed/build-windows/beta-recent-history-only-20261018/ordinary-runtime-r1.

Initial thread 32 omitted new tests because discovery metadata was stale; explicit
regeneration passed 34. A launcher argument error started no child. Private R1 was
interrupted 130 after static review caught Library-only registration and null model
binding. R2 built but its actual Recent click asserted at setMl after 14 successful
startup/no-database checks. R3 replaces conditional Binding with guarded assignment;
R3 passed six processes but failed the genuine locked-storage error expectation.
R4 performs a status-returning history count before publishing the frontend,
thereby separating an empty history from initialization failure. All accepted
results above use that source. An old ordinary runner identity path stopped the first ordinary
attempt before playback; the corrected fresh output passes five cases. Rejected artifacts remain local.

## Limits

This accepts scoped Recent functionality, not full Recent/sidebar/beta fidelity.
The unreferenced internal Recent composition remains provisional. Multiple-row
selection/scroll restoration, broader search/sort/error states, theme/accessibility/
DPI, full shell/Home fidelity, Windows 11 and clean matched licensed-package checks
remain open. Two original Videos/Music icons await the user's additional-reference
decision; no extra Figma reads were spent. No new security detector explanation or
automatic approval rejection was received; original trip cause remains unknown.
No push, upload or publication was performed. Five unrelated untracked files remain
untouched. Original designer, VLC and font attributions are retained.
