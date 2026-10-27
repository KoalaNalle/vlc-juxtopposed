# Music source placeholder and theme checks

27 October2026. Source `e39da9c7ee`. The source-search placeholder now
uses an explicit theme-aware foreground instead of the Basic control's implicit
placeholder color. This is a scoped appearance follow-up to the
[source-header receipt](../home-music-source-header-20261027/README.md).

The maintained music component checks pass. Ordinary GUI423
`42c18ca0` on the existing
Core265/library282/recording322 tuple verifies a readable placeholder in native
Dark, Light and current-System modes. Source options, standalone ancestors and
the nested ancestor menu follow Light mode. Search still filters First light
and Escape restores the three-item list while playback stays paused at zero.

System currently resolves to Dark. After normal restart, System remains selected
and displays the same palette. Only `MainWindow/color-scheme-index` changes in
the isolated interface settings; the complete player-profile catalogue remains
identical before/after/cold restart. Both ordinary runs exit0 with602plugins,
zero checked errors and every source/runtime/fixture/producer/cache ending guard.
Their exact binaries, producers, captures and configuration snapshots are retained.

A live Windows appearance-change check was attempted. The explicit Settings
executable launch and fresh window inventory exposed no targetable Settings window.
No OS theme change was performed. Read-only before/after checks confirm the five
recorded appearance values remain unchanged. System selection/restart is not
acceptance of live Windows theme propagation. No security/privacy, firewall or
Defender setting was changed and no firewall rules were added.

The first post-run settings analyzer used the wrong group for the theme key and
stopped with KeyError; the corrected analyzer reads `MainWindow/color-scheme-index`.
That was a harness lookup failure, not an application failure. Earlier immediate
UI observations can precede settled frames; capture04 is the settled Light field.

Complete theme/accent combinations and visual scores, native screen-reader/focus,
actual Windows DPI, multi-instance combinations, integrated regression and final
ordinary-package gates remain open. This GUI's one-line appearance change does
not silently replace the earlier GUI418 behavior receipt or establish full beta
acceptance. All test media is generated; no private media is included.

`checkpoint.json` binds compressed artifacts to their original and compressed
SHA-256. Decompression was verified. Binaries and test profiles remain ignored.
