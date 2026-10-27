# Alpha integration, Qt CI repair and upstream assessment

Recorded 27 October 2026 in the authoritative home checkout.

## CI failures and repairs

The original [failed run](https://github.com/KoalaNalle/vlc-juxtopposed/actions/runs/40492192713)
on `e304fb9f8a5ac54202ce66d74f2ae87c8a1b4f8a` stopped at the asset-distribution
assertion in `test_juxtopposed_playback_bar.py`. Both `skip-15-arrow.svg` and
`music-navigation-music-8b7f6.svg` were in `assets.qrc` but missing from
`libqt_plugin_la_RES` in `Makefile.am`. Both entries are added; the assertion stays.

Running all subsequent Windows commands exposed an outdated settings fixture:
`PlaybackSettingsMenu.qml` reads the real `Player.hasSecondarySubtitle` property,
but the supplied Python controller omitted it. The fixture now supplies it and
checks secondary selection availability, engine-only refresh, millisecond writes
and primary-delay independence. The no-QML-diagnostics assertion stays.

The workflow keeps both jobs and every test. Push checks target `master`; feature
branches are checked through PRs, avoiding duplicate push/PR jobs. Concurrency
cancels superseded runs for the same PR or branch. Manual dispatch remains.

## Verification and integration

- Local full Windows replay: 29 commands passed and the settings fixture failed.
  After its repair, the settings command passed, including the new assertions.
  All four Linux commands also passed in the established home WSL environment.
- [Repaired PR run](https://github.com/KoalaNalle/vlc-juxtopposed/actions/runs/40493854859)
  at `0e726dcf586e9b5a9593a1d02a58601e4af9a77d`: both jobs succeeded.
- [PR #1](https://github.com/KoalaNalle/vlc-juxtopposed/pull/1) was merged with
  expected-head protection and a history-preserving merge at
  `00133b4afcbba2e865aae599c8e9629258c1c69b`. The old feature branch remains.
- [Master run](https://github.com/KoalaNalle/vlc-juxtopposed/actions/runs/40494433496)
  on that merge also succeeded. Both runs' complete job logs contain all30Windows
  and four Linux test commands, with no workflow error or Python traceback.
- GitHub emits a nonblocking Node20 deprecation warning for the pinned v4
  actions; its runners execute them under Node24. This is separate from the
  repaired failures.

These are Qt component and build-rule checks, not a new native build, complete
graphical acceptance or final-package evidence. Existing native receipts keep
their exact binary scopes. README and BETA now describe the current development
stage as **alpha**; no release, tag or beta-completion claim was created.

## Upstream assessment only

Fetched the already-configured `https://github.com/videolan/vlc.git` master.
The fetched tip is `874d229c1896037992f9d10b06ce6c341f77c0eb` (26 October).
The common base is `1176b56bdef7b96c9d6518b62be37ec8c32d92ed`.
Upstream adds195commits and changes237files, with15,724insertions/5,058deletions.
Thirty-five files were also changed in our branch.

`git merge-tree --write-tree --name-only --no-messages HEAD upstream/master`
reported these four textual conflicts, without changing the worktree/index:

- `modules/audio_output/mmdevice.c`
- `modules/gui/qt/maininterface/qml/MenuTopbar.qml`
- `modules/gui/qt/maininterface/qml/NoMedialibHome.qml`
- `test/modules/meson.build`

Relevant upstream work includes the `ed4ca71a3a` Windows audio stop fix,
`eb14258480` recording probe cleanup, FreeType style-cache corrections, Qt
startup/window/DPI changes and contrib updates. Our audio implementation queues
teardown to its audio thread, so the conflict requires a deliberate comparison;
accepting either side mechanically would discard part of one implementation.
Clean textual merges in the other31overlapping files still need semantic review.

Recommendation: integrate upstream on a separate branch before final beta
validation, preserve the current tested runtime, resolve these overlaps and run
native build/media/shutdown regressions. This assessment does not claim the
latest dependencies build with the current cache. No upstream merge/rebase,
dependency installation, active-runtime replacement or native rebuild occurred.
The four unrelated audio experiments were excluded and left untouched.
