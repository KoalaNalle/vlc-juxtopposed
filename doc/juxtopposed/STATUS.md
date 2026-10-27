# Current status

Updated 27 October 2026. The desktop is available and beta work is continuing.
The tested recording-error source is `9bc894ae8e`.
The beta remains unfinished; all six readiness gates remain open.

## Selected checkpoint

- Branch: `juxtopposed-playback-bar`.
- Ordinary GUI441 `3f6170f7`, Windows-hotkeys452 `28e53fd1`,
  Core265 `fdfcfcd5`, library282 `13ccdc36`,
  recording476 `fb4e9c06`, MP4 mux464 `42e53a5e`,
  MP4 demux240 `df15e8ed`, subtitle130 `11f2b78e`,
  WASAPI15 `e5560794`, MKV `f46d8d15`.
- Runtime: `build-windows/beta-single-frame-20261025-night/ui-record-error-fixed-r478/runtime`.
  This is an isolated tested runtime, not a release package. Earlier tested
  GUI418/388/357/349/344/337/329/311 tuples remain recoverable. The test profile
  has one preseeded audio Record tile; helpers requiring the native default
  catalogue can use the isolated `ui-music-appearance-cold-r425` profile seed.
- All owned desktop/native tests are closed. Freeze fresh inputs before the
  next guarded job; current docs postdate `inputs-r476.json`. The four unrelated
  audio experiments stay untouched. No firewall rules or security/privacy settings
  changed; the read-only firewall inventory remains available.

## Latest verified work

[Recording error feedback](../juxtopposed-references/native/home-recording-error-feedback-20261027/README.md):
failed output-open now produces a visible alert and retained destination guidance.
Public API before/after receives zero/one error callbacks. Native and ordinary
same-player recovery decode; the ordinary retry contains22.2seconds of generated
PCM. Markers, native config and profiles stay unchanged; normal exits/602plugins/
all guards pass. The three expected failure log lines are separated from zero
unexpected diagnostics. The toast's Show Details click remains unqualified because
its five-second timeout can expire during automation; Ctrl+M / Errors is verified.

[Subtitle-overlap](../juxtopposed-references/native/home-recording-subtitle-overlap-20261027/README.md)
and [audio-format](../juxtopposed-references/native/home-recording-audio-retry-20261027/README.md)
receipts retain their record464 scope:22direct checks, decoded MP4/Matroska captions,
ordinary811-frame recording and six WAV/Ogg-FLAC/MP3 outputs. Exact onset/tail,
already-consumed/indefinite captions and broader combinations remain open.

[Windows key translation](../juxtopposed-references/native/home-global-key-mapping-20261027/README.md)
retains scoped native/ordinary evidence; startup layout is used until restart.
[Registration feedback](../juxtopposed-references/native/home-global-shortcut-feedback-20261027/README.md)
retains its earlier module432 two-player/restart tuple; exact-binary scopes stay separate.

[Source-header](../juxtopposed-references/native/home-music-source-header-20261027/README.md),
[Source appearance](../juxtopposed-references/native/home-music-source-appearance-20261027/README.md),
[navigation](../juxtopposed-references/native/home-music-navigation-20261027/README.md),
[toolbar](../juxtopposed-references/native/home-music-toolbar-20261027/README.md)
and other receipts retain their earlier scopes. Full visual scores, screen-reader/
actual Windows DPI, all pin variants and final-package acceptance remain open.

## Continue here

1. Qualify the error notice's Show Details pointer/keyboard/focus path and timeout
   behavior; the first attempt did not establish opening. Continue remaining
   formats/settings afterwards. Mid-write failures remain separate from output-open
   failure. Keep live System-theme changes, native UIA/screen-reader focus and
   actual Windows DPI open. Cover view remains a documented functional adaptation.
2. Continue W3/W2 in [BETA](BETA.md) and [SETTINGS](SETTINGS.md): remaining settings,
   remaining shortcut actions/layouts, rapid queries, recording formats,
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
