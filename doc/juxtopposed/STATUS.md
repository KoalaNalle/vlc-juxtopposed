# Current status

Updated27 October2026. Desktop work has resumed after the owner dismissed the
firewall prompt. The tested music-navigation nightly source is `4441e3449b`.
The beta remains unfinished; all six readiness gates remain open.

## Selected checkpoint

- Branch: `juxtopposed-playback-bar`.
- Ordinary GUI388 `b3b8024d`, Core265 `fdfcfcd5`, library282 `13ccdc36`,
  recording322 `930205b1`, MP4r240 `df15e8ed`, subtitle130 `11f2b78e`,
  WASAPI15 `e5560794`, MKV `f46d8d15`.
- Runtime: `build-windows/beta-single-frame-20261025-night/ui-music-nav-final-r397/runtime`.
  This is an isolated tested runtime, not a release package. Earlier tested
  GUI357/349/344/337/329/311 tuples remain recoverable.
- All owned desktop/native tests are closed. The read-only firewall inventory
  also finished. Freeze fresh inputs before the next guarded job; current docs
  postdate `inputs-r397.json`. The four unrelated audio experiments stay untouched.

## Latest verified work

[Music navigation receipt](../juxtopposed-references/native/home-music-navigation-20261027/README.md):
the music player reuses the native shared pane for six routes, Add and saved
pins, with a50px rail/377px wide source panel. The hidden video pane yields pin
callbacks while music is active. The maintained component regression fails before
and passes after. Ordinary checks cover native folder-pin creation/cold activation,
short-window wheel/keyboard visibility, separate Add actions, paused/playing
navigation and video-drawer/audio return. The fresh final run exits0/602plugins/
zero checked errors/all ending guards.

The intermediate wrong Videos export, firewall-observer timeout and separate
normal-exit recovery are preserved distinctly. Source-header behavior, full
visual scores, screen-reader/actual Windows DPI, all pin variants and final-package
acceptance remain open. Earlier [toolbar](../juxtopposed-references/native/home-music-toolbar-20261027/README.md),
[finish-action](../juxtopposed-references/native/home-finish-action-20261027/README.md),
[sidebar](../juxtopposed-references/native/home-music-sidebar-20261027/README.md),
[library-query](../juxtopposed-references/native/home-library-defaults-queries-20261027/README.md)
and [global-shortcut](../juxtopposed-references/native/home-global-shortcuts-20261027/README.md)
receipts retain their exact scopes.

## Continue here

1. Finish music source-header/title-strip context. Its complete cached Figma
   subtree and native-source review are retained in the navigation receipt.
   Back/Forward, breadcrumbs, separate external-folder and view controls need real
   consumers. The title strip currently follows unrelated library History.
   Define coherent native source navigation before wiring these actions.
2. Continue W3/W2 in [BETA](BETA.md) and [SETTINGS](SETTINGS.md): remaining settings,
   shortcuts/conflict feedback, rapid queries, recording formats/subtitle overlap,
   appearance, accessibility and Windows DPI. HDR stays truthfully disabled;
   unsupported independent Disc sort stays disabled.
3. W1's secondary-fullscreen caption capture issue also occurs on older Core204;
   physical output versus capture remains unqualified. The earlier cold RGB
   assertion stays failed; only the later dual-caption Reset pair matches.
   Sparse/long-cue cost, per-track combinations, other MP4 indexes and live
   behavior remain open.
4. Online subtitle search/download needs implementation and the owner API decision.
   Windows10/audio endpoints/receiver/casting retain explicit external deferrals.
   Finish integrated regression, independent review and clean-package/delivery
   gates after the remaining behavior/design work.
5. The owner authorizes test-related firewall rules with a cleanup journal.
   Preserve Private/LocalSubnet scope and enabled protections. The read-only
   `build-windows/firewall-ledger-20261027/` inventory records58project-program
   rules and the earlier backup location; this session added/changed no rules.
   Log exact before/after identities and undo steps for any future rule mutation.

## Saved question

Should Continue Watching retain all recent videos or only unfinished items?
The native query includes a fixture that reached EOF; the owned database has
position0.995987,duration-1,play_count0. This is a policy observation. A resume-only
policy needs native count/order/search/events qualified before limiting.
