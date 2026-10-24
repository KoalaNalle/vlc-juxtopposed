# Native add-on foundation — 17 October 2026

This accepts the native prerequisites below, **not the Extensions page or beta**.
Source: `c37137a1c3637aa9f90a4982185b575035bc5bd4`. The original Extensions7 design remains unmounted.

Install and Remove now retain their requested direction. Wrong-state, repeated,
busy and unmanaged requests fail admission. Accepted requests reserve their state
and own a queue reference. Manager shutdown signals its condition variables
before interrupting/joining workers, avoiding immediate interrupt callbacks
relocking their already-held queue mutex.

Filesystem removal now reports a real unlink error and retains resource metadata
for retry. Missing files are harmless during retry. A queued removal stays in the
catalog until its storage operation succeeds. Windows user-data environment
overrides use the wide CRT environment and convert to UTF-8 rather than losing
characters through the active ANSI code page.

## Evidence

- Actual manager + repository ZIP extraction + filesystem storage + catalog:
  **31 cases, 324 assertions**, normal exits, no forced cleanup.
  Covers actual files/bytes, wrong directions, duplicates, reentrant queued
  removal, process restart, Unicode user data, Windows deletion-denying file
  handle, failed removal persistence, unlocked retry, empty catalog reload and
  20 immediate pending-operation shutdowns.
- Standard Autotools executable builds and independently passes
  **12 cases/134 assertions** with one pending shutdown. The explicit
  fresh-output native make helper also builds. Meson syntax parses; no native
  Meson build is claimed.
- Final ordinary tuple passes redesigned controls, traditional controls,
  Library, owned local live HLS and a bounded read-only Downloads sample.
  All five exit normally; no QML diagnostics. Generated paths and the user's
  media logs/frames stay private. Source runtime/package hashes are preserved.
- All **14 scoped frozen inputs** still match. Build logs retain the cross-linker's
  host-path warnings; the actual Windows binaries run normally. This is an
  incremental build, not a clean rebuild or release package.

Core: `48099238a9f15e5d60e3b713219c950589d549ea83bab4c2df7aa6470038ae03`
Filesystem module: `fcf6c7bbb51a979537c75f374560fd5665be197a78c1ea9a9ec27063681122e1`
GUI (unchanged Recording checkpoint): `d2708b57bb1978a9c0da552b5b56dc6ecd6991dc1a718e6fb252dc93508756d2`

## Rejected runs retained

`baseline-direction.json`: the original consumer admits Remove on an uninstalled
item and installs its file; a second Install removes an installed item.
`baseline-locked-remove.json`: a deletion-denying handle leaves the file present
while the original consumer reports not installed and drops the catalog record.
That process then asserts on shutdown; its assertion is preserved separately.

`unicode-negative.json`: the first direction/shutdown fix passes ASCII cases but
fails installation/catalog reload in Unicode user data. It is rejected, with
locked-file cases unexecuted. The wide-environment correction passes those cases.
`pending-catalog-negative.json`: the same final probe/Core with the preceding
filesystem module fails exactly the reentrant queued-removal catalog check;
the corrected module passes it. All earlier raw work directories are retained.

## Remaining work

Reuse `AddonsModel` and VLC's existing `qt-autoload-extensions` preference.
Finish checked catalog/discovery and installer completion results, stable UUID
selection, installation collisions/partial multi-file failures and catalog-write
failures before accepting the redesigned Add/Remove flows. Complete the original
list/search/Find more/Add/Remove/startup/Advanced actions and native window tests.
Script activation/startup policy, external catalog behavior and the broader
theme/accessibility/DPI/platform/regression/clean licensed package gates remain
open. No YouTube/Twitch feature or new unshown UI is included here.

Reproduction instructions: [native add-on driver](../../../../test/libvlc/native_addons.md).
`manifest.json` hashes every public evidence file; raw/private artifacts have
separate hashes. The Linux `/mnt/c/Modding` path in build logs is the established
internal alias to the E: checkout, not a Windows system-drive source mount.
