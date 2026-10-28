# Indexed WebVTT FPS callback qualification — 28 October 2026

The final standalone candidate passes **8 cases / 314 checks / 0 failures**.
The same tests against the saved original demux produce 38 failed assertions;
unchanged stream-mode and invalid-rate controls pass on both versions.

This exercises the actual production WebVTT Open/Read/Control/Demux and packet
serializer using Core memory streams and captured ES output. It does **not**
load the candidate as a plugin or run its subtitle decoder, rendered pixels,
player refill, ordinary GUI or final package. Full WebVTT acceptance and TTML
remain open. Existing shared/staged WebVTT plugins were not replaced by this
standalone work. The native plugin build is held pending shared-tree recovery.

## Implemented and tested scope

- Indexed external files retain immutable parsed cue text and cue/index times.
  Presentation boundaries and genuine inline timestamp tags are transformed
  from those originals, including repeated and fractional rate changes/Reset.
- Containing-interval refill retains active overlaps, handles duplicate/zero
  boundaries and empty input, revives relevant cues after EOF and avoids wrapping
  an out-of-range seek to the first cue.
- Packet box sizes, IDs/settings, escaped text and malformed timestamp-like tags
  are checked. The stream callback retains rate1; its rate control stays rejected.
- Nonfinite/invalid rates and unrepresentable times reject before mutation.
  Large-length position1 uses exact integer length. Standalone EOF does not
  advance an overflowing clock. Zero-relative and meaningful negative seek/
  preroll times remain valid; true INT64_MIN/MAX boundary failures are atomic.

`VLC_TICK_INVALID` is zero in this tree, whereas `VLC_TICK_MIN` is INT64_MIN.
Relative `DEMUX_SET_TIME(0)` is a meaningful seek, so the final guard does not
mistake the timestamp sentinel for an invalid relative start position.

The unchanged parser translation unit also contains its module descriptor.
Unused decoder/encoder/muxer callback references need definitions in this PE/COFF
standalone link. Those test definitions fail if called; they do not implement or
simulate decoding. Real decoder/pixel validation remains a separate requirement.

## Exact tuple and checks

- Final demux source: `433da57aa248b6be9b7bb65a4ddedd6c940eb1fb4f3fd3c9c6b14c25674df4e6`.
- Test source: `a6677c8cafe551c00c752adaa6a541d2601af53f305ae2d0c7e2ffae5483b5a0`.
- Core DLL: `f8fc980bbe03da57908f79045f266710dd27a16c6ee66b6c45d60cd1f9b93598`.
- Final candidate EXE: `7ef88a9bbab3bf304cf69f05650ba7fc51d6ca6801f208d9b88ce8513dce0e69`.
- Frozen source/fixture/header inputs:144; existing read-only dependencies:257.
  All 38 dependencies discovered by the actual compile are contained in those
  frozen maps, match their hashes and preserve the compiler identity.
- Candidate exits0; deliberately failing original exits1. No forced cleanup.
  Source/dependency/producer/config/build/Core/runtime EXE guards pass.

The standalone compiler writes only fresh ignored output; it invokes no `make`
and mutates no shared native build tree. The two-file private runtimes use the
preserved Core. Generated fixtures contain no private media. No desktop use,
new dependencies, source restoration or plugin replacement occurred here.

## Preserved development evidence

| Revision | Actual outcome and limit |
| --- | --- |
| R1 | Test setup compile failed on the generic NULL object macro; no test executed. |
| R2 | Test setup link exposed unused descriptor callback references; no test executed. |
| R3 | Initial callback candidate7/276 passes; original26 failed assertions. No large-clock coverage yet. |
| R4 | Position rounding fix and large-length test:8/292 passes. Later review found EOF advancement overflow. |
| R5 | EOF advancement fix:8/294 passes. Later review requested explicit invalid seek guards. |
| R6 | New boundary assertions expose3 candidate failures: an incorrect sentinel guard rejected zero and negative preroll. Preserved unchanged. |
| R7 | Correct finite-clock guards and zero/negative tests:8/314 passes; original38 failed assertions. |

`final/` freezes current demux/test source; `fixtures/` freezes all five synthetic
inputs; `preimage/` retains the original demux. `revisions/` contains exact dated
producers, configurations, reports and available source snapshots, including
failures. `observations.json` provides the consolidated receipts.
`actual-compile-commands.json` records actual compiler/link argument arrays;
`compiled-dependencies/` and the final dependency inspector retain closure proof.
`SHA256SUMS` covers every other file here.

Use [REPRODUCE.md](REPRODUCE.md) for the maintained standalone test recipe. The
historical producers retain home-machine paths and one-use output names; they
are evidence, not commands to rerun blindly. Shared plugin integration must wait
for explicit writer authorization and refreshed build-tree inventories. No result
here closes ordinary rendering, secondary-track, recording/export, GUI, DPI,
TTML or beta-readiness gates.
