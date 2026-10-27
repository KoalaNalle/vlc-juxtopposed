# Music source header and folder history

27 October 2026. Source `d80fb40b3a`. This is a scoped nightly checkpoint;
the beta and final-package gates remain open.

## Result

The music source panel has native per-player Back/Forward history and real local
ancestor URLs. Browsing changes the displayed folder without changing the playing
input or queue. History is bounded to64 locations and is not saved to settings.
Current track folder returns to the input's origin. A separate external-folder
button opens the browsed directory. The title-strip badge follows the actual
folder or Play Queue and reopens/focuses the inline panel rather than returning
to unrelated library History.

The header retains the original two-row geometry,20px padding and exported SVGs.
Source options and ancestors remain reachable through the compact folder caption.
Cover view reuses the existing library card; this layout is a functional adaptation
for the referenced view action, not a newly claimed original music-player frame.

## Evidence and limits

- Native LocalMusicFolder test:9 QtTest functions including init/cleanup, no failed
  incidents, normal exit and all guards. Covers actual folder data/history,
  rejected locations without losing Forward, rapid folder/source changes,
  unchanged queue/audio callbacks while browsing, real playback/reuse and worker
  teardown. Its three native sources match the final GUI input manifest.
- Maintained music component: the preceding header fails on missing Forward.
  A second negative control reproduces a menu remaining visible when native-style
  synchronous navigation replaces its selected delegate. The final test passes
  actual qrc aliases, disabled/history/ancestor/external actions, menu dismissal
  and palette, list/cover selection and keyboard activation, search, queue fallback,
  loading/error/empty states and five component-scaled layouts. Title-strip12
  geometry cases and source-description/controller seams pass separately.
- Ordinary GUI418 `de024969`,
  Core265 `fdfcfcd5`, library282 `13ccdc36`, recording322 `930205b1`:
  cold602x421 and wide window, compact ancestor navigation/dismissal, Back/Forward,
  empty parent, original icons, actual File Pilot folder window, cover click without
  playback and Enter starting Second light, playing parent/Back, paused source badge,
  real three-item queue/list/folder modes, no-match search/Escape and Home/return.
  Ten paused observations retain position `0.5140644333333334`. Final normal exit0,
  602plugins, zero checked errors including missing SVG resources, and all source,
  runtime, producer, cache and fixture ending guards pass.
- Raw File Pilot observation stays ignored. All retained media/UI journeys use
  generated project fixtures; no personal media is packaged here. No firewall,
  Defender, security/privacy choices or rules were changed by these checks.

The preliminary GUI408 run preserves the squeezed compact label and missing qrc
menu alias. GUI414 preserves the white/retained ancestor menu. Both close normally;
their checked-error count alone did not establish header acceptance. The first
asset inspector stopped on a zero-alpha unrelated existing SVG; the revised
inspector tolerates such existing assets while requiring visible new exports.
No unrelated SVG was changed. Component run412 completed its assertions but failed
Windows temporary-font cleanup; later outputs keep the resource mirror ignored
with verification artifacts and terminate normally.

Actual Windows100/150/200% DPI, complete Dark/Light/System/accent states,
screen-reader behavior, risky media/instance combinations, final-package testing
and the full visual scoring gates remain open. Native UIA focused-element reports
were stale across several actions, despite the observed correct Enter activation;
this is not screen-reader acceptance. The existing source-search placeholder was
hard to read in Dark mode and needs a follow-up.

## Provenance

Figma file `zdNUYRVOH9lEtIxMYvh8DV`, open music frame `4778:25123`, header
`I4778:25149;4778:25000` and its two rows. Use the complete cached subtree retained
by the [navigation receipt](../home-music-navigation-20261027/README.md); sparse
follow-up wrappers did not replace it. Original art is unchanged. Sort/Search
reuse exact existing renders; More reuses the equivalent existing18px asset through
its qrc alias. Four other original exports are copied byte-for-byte with authored
horizontal mirroring. The asset hashes and input manifests are retained below.

`checkpoint.json` records exact binaries and each compressed artifact's original
and compressed SHA-256. Decompression was verified. Large binaries and isolated
profiles remain ignored on the development SSD. This receipt does not contain a
release package or claim that the complete music surface is accepted.
