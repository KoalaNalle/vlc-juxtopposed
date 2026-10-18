# Guest native network artwork — 18 October 2026

Scoped follow-up to [ordered sidebar pins](../guest-sidebar-pins-20261018/README.md).
All project files, runtime profiles, media and evidence stayed on E:. Build and
native playback jobs ran sequentially, using one build worker and CPU-only WSL.
The full beta remains incomplete.

## Failure and fix

Opening pinned folders/sources with the media library enabled retained two
empty SVG requests (four Qt warning lines) per process. A private-only colorizer
diagnostic raised a marked QML error for an empty input. All six traced calls
pointed to `NetworkGridItem.qml:58`, where a delegate could evaluate its artwork
binding before the artwork role was available. This diagnostic is not acceptance.

A separate unchanged-GUI negative control exited normally and passed its 203
functional checks, while the stricter runner correctly rejected four original
empty-file/SVG warnings. Thus normal functional success alone did not cover this bug.
The binding now returns an empty image while its artwork role is absent, then
reacts normally when the model supplies a path. The provider is unchanged. The
maintained native runner rejects these warnings on future runs.

## Accepted evidence

- Fresh native populate/reopen/disabled/replacement: 203/70/68/72 checks (413).
- All four processes quit normally; no selected diagnostics or Qt warning lines.
- Ordinary modern/traditional/library/local-HLS/Downloads replay: five normal
  exits, decoded output, no Qt warning lines; the Downloads sample stat is unchanged.
- Owned generated media and previous runtime preserved. The ordinary module has
  neither the private journey marker nor the temporary diagnostic marker.
- All 1916 ordinary and 7 private source inputs match; prior 458 Core and
  8 store inputs are unchanged. Earlier storage/component results retain their
  prior scope and were not rerun for this binding-only change.

Core: `0cdebe2f6950939228eac94304393ef946b1947c80d1cbb8a8f83abe546a8072`
Ordinary GUI: `f891112fc48f6a8c6d9436d1c09c33ad09dad07fdea92f52caedc5de6e56982d`
Media library: `e3ec5ed46c8abb561c99c45864261efb003abfa276ee9b341a50bacfba213c0a`

The ordinary build log shows fresh NetworkGridItem cache generation, compilation
and network resource/archive rebuilding. The private test module reuses the
unchanged, hashed journey object from the prior accepted build; its normal make
recipe can compile that source from scratch when no object is supplied. The first
private diagnostic link failed because its extra object was missing from target
prerequisites. That harness attempt is retained as rejected; R2 corrects it.

## Reproduce and remaining scope

Read guest setup, dot-source Common.ps1 and use the configured E: guest checkout.
[Reproduction](reproduction/) retains the one-worker build, diagnostic, unchanged-GUI
negative and sequential native/ordinary runners. Inputs refer to the retained
immutable E: baseline. Work logs and full owned native reports are retained under
`build-windows/beta-network-artwork-20261018`; media filenames/raw playback logs and
frames stay local. The private diagnostic and journey DLLs must never be shipped.
[Results](results.json) records the exact binaries, case counts and inventories.

This closes the observed empty-SVG warning. Original sidebar 217/50px geometry,
Home/Recent, full design fidelity, broader browsing protocols/authentication,
accessibility/themes/DPI/monitors, Windows 11 and a clean licensed beta package
remain open, together with the remaining confirmed redesign. DVD probe stderr
while browsing ordinary folders is separate and does not establish disc playback.
