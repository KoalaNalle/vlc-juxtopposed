# Playlist-origin work — 19 October 2026

The native queue-origin backend passes eight headless cases. The later
[actual app replay](actual-app-verification-r4.json) passes67 checks across
Library On/Off, with two normal exits, no Qt diagnostics/native errors and601
loaded modules. It verifies actual library names and rendered literal labels,
shared-media origins, model selection, rename/deletion and paused independence.
[Provenance](actual-app-provenance-r4.json) binds that result to ordinary GUI
`fd7f8af9b480…`, private GUI `8be8eff6e4d8…` and local `source-r6`.
Generated-only rename/selection captures are included. Native media warnings,
broader interaction/visual/DPI and final-package qualification remain open.

## Implemented so far

An explicit library-playlist ID travels with the copied native Media and is
associated with each new native queue entry ID. It is independent of URL, track
title, row position or other playlist memberships. Unknown sources retain ID0.
Deleted origins are invalidated; a late conversion cannot restore a deleted
origin in this library session.

The new regression exposed an older cached-current-item defect: removing the
current item could keep the same row index while replacing its native identity.
No index-change event then refreshed the cache. Remove/reset handling now
refreshes identity against the native queue, so the next track gets its own
origin and the removed origin cannot attach to it.

Whole-playlist playback, captured model selections and playlist-row context
actions now carry their origin. The selected-playlist context is captured when
the command is issued. The current-media worker resolves that explicit ID's
native name, handles rename/deletion, and checks player/queue input identity
during independent notifications. Live and customizer-preview metadata are
wired to that name. The actual app replay now verifies these scoped paths.
No inferred membership, guessed queue name or label supplied by a view is used.
Generic file/URL queue entries with no known named origin remain blank.

## Scoped evidence

- [Native queue receipt](queue-origin-verification.json): eight passing cases,
  normal exit and no diagnostics. Uses the actual Windows Core playlist and Qt
  controller with explicit fixture collection IDs. Covers identical URLs from
  distinct origins, insertion and reorder, clipped insertion hints, deletion
  and late results, native removal/reset and a read-only metadata contract.
  It does not instantiate the GUI, decode media or prove native ML name lookup.
- [Component receipt](component-verification.json): 25 checks, including literal
  playlist-name text and the shared customizer/toggle/keyboard behavior. Uses
  owned controller contracts, not native library services or OS-DPI proof.
- [Single-play receipt](single-play-verification.json): 13 existing windowless
  routing checks pass. The existing queue/proxy mapping script also passes;
  neither establishes native pointer/touch/screen-reader acceptance.

The accepted backend executable is
`b653ca13402fbd8e167784e2e00813d07d5ee7c18bba77c188904823d6b3801e`.
Use the receipt for its other identities and source manifest. Source
and rejected runs are under `build-windows/beta-playlist-context-20261019`.
R1 had a private fixture namespace error. R2 reproduces the stale-current-item
failure; R3 repairs it and passes all eight. A first R3 runner attempt used the
old input-manifest filename and stopped before running tests; the corrected
runner checks the matching R3 manifest.

The current ordinary GUI also includes the scoped Audio restart-feedback repair
and compiles as
`6d655265b8d1de0e4fd3405335581bf4e8383e49902971fe66f62a3e79598008`.
The matching private integration GUI compiles as
`58d6381bb28f7d4a50c0506f6aba5ed48a830b27c3b9500fb4f68504744a3e3a`.
Its earlier prepared journeys cover actual library name resolution, model selection of
a shared track, rename/deletion and Library OFF. That older binary was not run
while the firewall question was pending; the later R4 pair above was verified
after the human's dismissal and fresh desktop inspection.
These are compilation results, not startup/playback/UI acceptance. The later
local source checkpoint is `source-ui-r4`, with separate frozen production and
private inputs; its compiled inputs matched when archived and its production
manifest matched the ordinary Audio R7 candidate when archived. `source-ui-r1`
through R3 preserve the earlier builds. Later fullscreen-consumer source needs
its own current-source replay; these snapshots remain immutable. No publication occurred. The
prior [Favorite checkpoint](../home-current-media-20261019/README.md) and its
protected ordinary runtime remain the accepted baseline in their own scopes.
