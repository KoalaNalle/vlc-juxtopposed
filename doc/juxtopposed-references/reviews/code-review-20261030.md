# Preparatory code review - 30 October 2026

The roadmap now selects **graphical alpha** as the next implementation milestone.
This focused review is complete; work pauses for the owner's decision before any
repairs, graphical audit or desktop testing. The three findings below remain open.
No production code was changed, committed or pushed in this review.

## Scope and evidence

Reviewed the current worktree on `codex/beta-20261028`, HEAD
`8b7675a266f9325d8827627dc4d3a68fa33a6684`, with `master` as the working alpha
comparison context. The inspection covered selected project-owned UI and native
integration paths: Home/history mutations and worker lifetime, modal routing,
conversion cancellation/publication, checked settings and cached catalogs,
profiles, track-timing contexts, title/fullscreen controls, recent player redraw/
recording changes, TTML/WebVTT timing, FFmpeg failure reporting and test registration.
The alpha comparison also contains upstream changes; this is not an audit of all
upstream VLC or a line-by-line review of every file in the identity inventory.

[Machine-readable evidence](code-review-20261030-evidence.json) identifies 69
inspected source/test/build files, their current hashes, five completed component
suites and the controlled reproductions. Raw captures, helpers and test logs are
in ignored `build-windows/code-review-20261030-r1/`. The four unrelated experiments
excluded by AGENTS.md were not read, hashed or changed.

This is a preparatory self-review. It does **not** satisfy beta G4's independent
review of the final candidate, establish visual completion, or transfer historical
native evidence to a different binary tuple. No native build, ordinary VLC test,
desktop input, live Figma call or clean-package test was run here.

## Findings

### R1 - P2: a stale window can discard or resurrect sidebar pins

Location: [sidebar_pins_store.cpp](../../../modules/gui/qt/maininterface/sidebar_pins_store.cpp),
`add()` at line 282, `save()` at lines 256-277, and the analogous remove/move paths.

Two windows can both load `[P]`. Window A then saves `[P, A]`. Window B adds B
using its cached `m_pins`, builds `[P, B]`, and successfully replaces the complete
serialized catalog. A disappears from saved pins even though both operations
report success. A stale add can likewise restore P after the other window unpins
it. The writes need not overlap: this follows from using the old in-memory
catalog as the next candidate. `QSettings::sync()` cannot merge records inside
the one JSON string. The model's successful add/remove/move paths do not reload
or compare the previously accepted document before saving.

Evidence: traced the actual store's read/modify/save path and its model callers;
existing persistence tests cover restart, invalid data, write failure and retry,
but not this two-writer sequence. No native two-window reproduction was run, so
the finding is source-established rather than an ordinary-desktop pass/failure.

Recommended repair: serialize read/modify/write against current catalog data,
or reject a stale expected document and offer reload/retry. Preserve stable IDs,
order and the user's pending operation. Qualify two stale stores adding different
pins, unpin followed by a stale add, reorder conflicts and both closing orders.
This is a saved-state defect to fix before beta; it does not prevent a read-only
graphical inventory.

### R2 - P2: readback rollback can overwrite another accepted settings save

Location: [checked_preferences_store.cpp](../../../modules/gui/qt/util/checked_preferences_store.cpp),
`saveSettings()` at lines 148-162; theme writes reach it through
`GeneralPreferencesModel::storeSettings()`.

The write/readback/rollback sequence has no transaction-wide serialization or
ownership check on the rollback. Start with a key at 0. Writer A saves 1, then
writer B saves and successfully reads back 2. A now reads 2, treats that legitimate
newer value as a failed save, and restores its old 0. B's accepted value is lost;
A reports rejection even though it also undoes someone else's commit. The same
rollback branch can remove a newly created key when A's original snapshot lacked it.

Evidence: source tracing plus a deterministic two-process demonstration with
real PySide6/Qt 6.8.3 `QSettings`, a private INI and the production operation order.
It observed B's 2 before rollback and 0 afterward, with all Qt settings statuses
`NoError`. See `settingsInterleaving` in the evidence JSON and the local
`probe_settings_interleaving.py`. This demonstrates the primitive interleaving;
it does not execute the C++ store or an ordinary VLC theme journey.

Recommended repair: make the checked transaction and conflict handling preserve
newer commits; rollback only a value still owned by the failed attempt. Use a
locking/staging scheme compatible with QSettings' own INI lock rather than
holding its `.lock` while asking ordinary `sync()` to reacquire it. The isolated
profile-store transaction is an existing pattern to assess. Add a production
store regression with a controlled second writer between write and readback,
including the absent-key case. Beta's multi-instance saved-state gate remains open.

### R3 - P2: conversion and New Playlist lose text contrast in Light mode

Locations: [JuxtopposedConversionDialog.qml](../../../modules/gui/qt/dialogs/dialogs/qml/JuxtopposedConversionDialog.qml)
line 152 and
[JuxtopposedNewPlaylistDialog.qml](../../../modules/gui/qt/dialogs/dialogs/qml/JuxtopposedNewPlaylistDialog.qml)
lines 83 and 287.

Both dialog bodies hardcode `Qt.rgba(25/255, 25/255, 25/255, .95)` while labels
use the theme-aware palette. Switching to Light leaves the body at `#F2191919`
but changes primary labels from `#C6C6C6` to `#212121`, making section headings
nearly disappear. New Playlist's drop instruction also uses the border/surface
token `#404040` as foreground; Light maps it to `#E0E0E0` against its light drop
area, so fixing the body alone will not repair that instruction.

Evidence: loaded actual production dialogs through their existing supplied-state
component-test setup with the real QML palette and explicit Dark/Light providers.
Recorded color properties and inspected both rendered Light captures. See
`dialogThemeProbes` in the evidence JSON and local `conversion-theme-r2/light.png`
and `playlist-theme-r2/light.png`. Native providers, Windows compositing and
screen-reader behavior were not exercised. The initial probe attempt failed to
resolve a lexical QML name before measuring; corrected r2 is the qualified run.

Recommended repair: route body fills through the existing translucent theme
token and give instructional text a foreground/placeholder token. Verify empty,
populated, busy, error and retry states in Dark/Light/System and nondefault accent;
retain the authored dark colors. This belongs in the graphical alpha repair list.

## Checks completed and practical limits

All five existing supplied-state/offscreen suites exited normally with code 0:

| Suite | What it establishes here |
| --- | --- |
| Audio timing selection | Production QML marker/token actions across supplied track, rate, delay and state changes. |
| Error notification | Extracted production notification/DrawerExt, timeout/hover/focus, Details dispatch and dismissal against supplied boundaries. |
| Home/history actions | Production route/handler selection, clear confirmation, busy checks, item removal and resume policy against supplied native boundaries. |
| Conversion dialog | Production QML states, progress, cancel/delayed close, errors/retry, focus and short/scaled layouts with a supplied controller. |
| New Playlist dialog | Production QML ordered selection/drop dispatch, busy/partial-save/error/retry, focus and short/scaled layouts with a supplied controller. |

Their assertions and receipts passed. They do not exercise real settings writes,
native conversion/history mutations, ordinary dialogs, OS DPI or whole-screen
fidelity. In particular, these existing default-dark suites passed while the
separate Light probes exposed R3. Component Details dispatch does not close the
ordinary R29 Show Details/Messages task.

Recent native changes were inspected without a new confirmed defect in those
reviewed paths. Existing scoped native receipts remain evidence for their exact
source/binary/fixture tuples. No fresh native regression or universal correctness
claim follows from this inspection. In particular, close-under-load history work,
remaining subtitle-format/role/default/estimator cases, other shared Qt writers,
Windows theme propagation/DPI, physical audio endpoints, Windows 10 comparison
and final packaging remain open in BETA/SETTINGS.

## Decision for the next session

Recommended continuation: first agree how to schedule R1/R2's saved-state repairs,
then carry out graphical alpha's GA1 audit across all S01-S32. Fix R3 during that
graphical pass and use named evidence to choose further presentation work. Keep
the already prepared subtitle/recording continuation available after the visual
checkpoint or if it blocks the walkthrough. Preserve all six beta gates and all
six graphical alpha checks as open. No repair or audit is started by this receipt.
