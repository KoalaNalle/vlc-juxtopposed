# Current status

Updated 27 October 2026. The desktop is available and beta work is continuing.
The tested recording/subtitle source is `ac762a8bbb`.
The beta remains unfinished; all six readiness gates remain open.

## Selected checkpoint

- Branch: `juxtopposed-playback-bar`.
- Ordinary GUI441 `3f6170f7`, Windows-hotkeys452 `28e53fd1`,
  Core265 `fdfcfcd5`, library282 `13ccdc36`,
  recording464 `9f4a3f03`, MP4 mux464 `42e53a5e`,
  MP4 demux240 `df15e8ed`, subtitle130 `11f2b78e`,
  WASAPI15 `e5560794`, MKV `f46d8d15`.
- Runtime: `build-windows/beta-single-frame-20261025-night/ui-record-overlap-r466/runtime`.
  This is an isolated tested runtime, not a release package. Earlier tested
  GUI418/388/357/349/344/337/329/311 tuples remain recoverable.
- All owned desktop/native tests are closed. Freeze fresh inputs before the
  next guarded job; current docs postdate `inputs-r472.json`. The four unrelated
  audio experiments stay untouched. No firewall rules or security/privacy settings
  changed; the read-only firewall inventory remains available.

## Latest verified work

[Audio-only recording/retry](../juxtopposed-references/native/home-recording-audio-retry-20261027/README.md):
two WAV, two Ogg-FLAC and two MP3 recordings decode completely; paused arming/holding,
normal exits and all native guards pass. Exact sample counts are retained, with up
to278ms difference from the observed playback-clock span; exact onset/tail is not
accepted. A file-as-folder output fails without stopping playback; valid retry in
the same native player decodes and preserves the marker. Ordinary error feedback
remains unqualified, including the log's unhelpful No error suffix.

[Recording subtitle overlap](../juxtopposed-references/native/home-recording-subtitle-overlap-20261027/README.md):
finite buffered captions now survive the selected A/V keyframe. MP4 shutdown text
conversion and final subtitle edit duration are repaired. All22 direct production
checks pass. Native MP4/mov_text and Matroska/SRT recordings decode aligned AV and
both exact caption intervals; the ordinary Record-control journey retains all
three captions and decodes811video frames. Exit0/602/no checked errors/all guards;
native config and player profiles stay identical. Before/intermediate failures
and the baseline mux inventory limitation are retained. Unknown/already-consumed
captions, other formats, live behavior and full package acceptance remain open.

[Windows key translation](../juxtopposed-references/native/home-global-key-mapping-20261027/README.md)
retains ten native functions/seventeen pass incidents and ordinary Finnish-layout
play/pause evidence. Startup layout is used until restart.
[Registration feedback](../juxtopposed-references/native/home-global-shortcut-feedback-20261027/README.md)
retains its earlier module432 two-player/restart tuple; exact-binary scopes stay separate.

[Source-header](../juxtopposed-references/native/home-music-source-header-20261027/README.md),
[Source appearance](../juxtopposed-references/native/home-music-source-appearance-20261027/README.md),
[navigation](../juxtopposed-references/native/home-music-navigation-20261027/README.md),
[toolbar](../juxtopposed-references/native/home-music-toolbar-20261027/README.md)
and other receipts retain their earlier scopes. Full visual scores, screen-reader/
actual Windows DPI, all pin variants and final-package acceptance remain open.

## Continue here

1. Check the ordinary recording destination-failure feedback and recovery. Native
   WAV/FLAC/MP3 repeated recording and destination retry now have scoped evidence.
   Continue remaining formats/settings afterwards. Finite buffered subtitle overlap has
   scoped native/ordinary evidence. The setup
   tour uses local bindings, so failed global mappings do not alter its hints.
   Keep W3's live System-theme change and native UIA/screen-reader focus open. UIA reports
   stayed stale even when Enter activated the correct cover. Cover view remains
   a documented functional adaptation rather than a complete visual acceptance.
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
