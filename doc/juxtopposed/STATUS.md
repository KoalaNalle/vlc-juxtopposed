# Current status

Updated 27 October 2026. The desktop is available and beta work is continuing.
The tested Windows key-translation source is `546eecd8e1`.
The beta remains unfinished; all six readiness gates remain open.

## Selected checkpoint

- Branch: `juxtopposed-playback-bar`.
- Ordinary GUI441 `3f6170f7`, Windows-hotkeys452 `28e53fd1`,
  Core265 `fdfcfcd5`, library282 `13ccdc36`,
  recording322 `930205b1`, MP4r240 `df15e8ed`, subtitle130 `11f2b78e`,
  WASAPI15 `e5560794`, MKV `f46d8d15`.
- Runtime: `build-windows/beta-single-frame-20261025-night/ui-global-punctuation-r458/runtime`.
  This is an isolated tested runtime, not a release package. Earlier tested
  GUI418/388/357/349/344/337/329/311 tuples remain recoverable.
- All owned desktop/native tests are closed. Freeze fresh inputs before the
  next guarded job; current docs postdate `inputs-r456.json`. The four unrelated
  audio experiments stay untouched. No firewall rules or security/privacy settings
  changed; the read-only firewall inventory remains available.

## Latest verified work

[Windows key translation](../juxtopposed-references/native/home-global-key-mapping-20261027/README.md):
the legacy byte fallback's two false-success cases reproduce before. Layout-aware
punctuation/modifiers, Unicode nontruncation and explicit extended-key mappings
pass ten native functions/seventeen pass incidents with no skips. An ordinary
Finnish-layout semicolon key starts/pauses playback, with matching Controls/editor
feedback and an exactly held clock across inspection. Exit0/602/no checked errors/
all guards; native config and player profiles stay identical. The preliminary
OS-owned Tab fixture outcome remains preserved. Startup layout is used until restart.

[Registration feedback](../juxtopposed-references/native/home-global-shortcut-feedback-20261027/README.md)
retains its earlier module432 tuple: native/component checks, two-player collision,
actual delivery, saved-versus-running feedback and cold replacement activation.
The new module does not silently inherit that exact-binary ordinary result.

[GUI418 source-header receipt](../juxtopposed-references/native/home-music-source-header-20261027/README.md):
native per-player folder history, real breadcrumbs, current-track folder and a
separate file-manager action now drive the header. List/cover views reuse shared
components. The title badge follows the actual folder or queue and reopens the
inline source panel. Nine native test functions, maintained component negatives/
positives, title-strip checks and ordinary compact/wide/paused/playing journeys
pass. The final run exits0/602plugins/zero checked errors/all ending guards.
Ten paused observations retain the exact position across panel, queue, search
and Home/return actions. Preliminary alias/layout/menu defects stay preserved.

[Source appearance](../juxtopposed-references/native/home-music-source-appearance-20261027/README.md),
[navigation](../juxtopposed-references/native/home-music-navigation-20261027/README.md),
[toolbar](../juxtopposed-references/native/home-music-toolbar-20261027/README.md)
and other receipts retain their earlier scopes. Full visual scores, screen-reader/
actual Windows DPI, all pin variants and final-package acceptance remain open.

## Continue here

1. Continue W2's recording-format/subtitle-overlap and remaining settings journeys;
   key translation and registration feedback now have scoped evidence. The setup
   tour uses local bindings, so failed global mappings do not alter its hints.
   Keep W3's live System-theme change and native UIA/screen-reader focus open. UIA reports
   stayed stale even when Enter activated the correct cover. Cover view remains
   a documented functional adaptation rather than a complete visual acceptance.
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
