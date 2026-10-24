# FLAC coded final block — 19 October 2026

The recorded frame-size warning represented actual tail loss in the generated
60-second fixture: the old tuple decoded479,232 of480,000 declared samples.
The repaired tuple decodes **480,000**, with the PCM MD5 exactly matching its
STREAMINFO digest. [Before](pcm-before.json) and [after](pcm-after.json) use the
same maintained runner and unchanged source fixture, with normal exits and no
forced cleanup. This proves the tested complete audio, not every FLAC stream.

## Cause and repair

The fixture uses fixed576-sample blocks and a final192-sample block encoded with
the standard block-size code. The preliminary header check rejected that final
block against STREAMINFO's nominal minimum, although the later frame-info check
already permits a shorter final block in a fixed-block stream. The combined
lookahead then exceeded the maximum frame size and discarded both final frames.

The shared preliminary check now follows the existing fixed-block policy while
retaining the maximum bound and existing variable-block policy. Both users of
the header, FLAC packetizer and FLAC demuxer, were rebuilt. Core, GUI, MMDevice,
WASAPI and media-library binaries remain unchanged. The FLAC standard explicitly
excludes the final block from the declared minimum:
[RFC9639 section8.2](https://www.rfc-editor.org/rfc/rfc9639.html#section-8.2).

## Native proof

- [Paired old-module failure](packetizer-before.json) exits with ordinary test
  failure1 after joining/releasing native owners: the small576+192 fixture
  produces no packets. This is distinct from the earlier missing-runtime loader
  attempts and the abandoned test-link experiment.
- [Same desired executable, repaired modules](packetizer-after-paired.json)
  passes all eight cases, including exact packet bytes/PTS, four chunk sizes for
  the new fixture, prior split/EOS, truncation, CRC/flush and bounded-header cases.
  The two coded frames are counted independently of the parser under test.
- [Recompiled current-header executable](packetizer-after-current.json) also
  passes all eight normally, covering the current directly included header.
- [Existing nine-case encoder/drain suite](encoder-drain.json) passes on the
  repaired package, including strict88,200 samples, boundary lengths and repeat.
- [Compilation/source provenance](compilation.json) records both changed module
  hashes, preserved runtime hashes, immutable desired executable and source
  copies under `build-windows/beta-flac-coded-tail-20261019/source-after`.

The maintained packetizer runner adds an independent valid constant16/44100
fixture with576+192 samples and checksums; it preserves the earlier real native
encoder fixtures read-only. The test executable needs the already installed
compiler's `libssp-0.dll` beside it; that file was copied only into the owned
test folders, with its hash recorded. No dependency was installed. Failed loader
attempts and link logs remain ignored evidence and are not counted as tests.

The remaining `no more input streams for this mux` warning comes from normal
last-stream removal in `sout_MuxDeleteStream`, not the repaired packetizer. It
remains recorded; complete sample count/checksum and normal termination pass.
The old FLAC discard warning is absent from the repaired full-decode result.

This tuple uses ordinary GUI `ca6e393d13b9…`, FLAC packetizer `5c9309434d44…`
and FLAC demuxer `0e736e133723…`. Complete GUI/package/Windows10 and broader
beta acceptance remain open. No commit, release or security-setting change occurred.
