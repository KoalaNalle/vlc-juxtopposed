# Native Library metadata checkpoint — 16 October 2026

Historical checkpoint: the later [real-service validation](../live-services-20261016/README.md)
supersedes the current package, network-coverage and five-second optional timeout
status below. It also records directory ordering/HTTPS repairs and publication.

The bounded metadata/artwork slice is operational in the ordinary Windows x64
package. Library's metadata control is enabled. File/local mode remains distinct
from internet-enabled lookup, uses the existing shared Privacy preference, and
applies after restart. No personal setting was changed for these checks.
This is local acceptance, not full Library or beta acceptance.

## Behavior and implementation

File parsing delivers embedded metadata first. Optional enrichment runs separately
on a copy, so cancelled or timed-out lookups cannot replace parsed titles with
partial provider output or discard duration/tracks. A single operation keeps its
interrupt and deadline across local, network and download stages. Cancellation
covers queued and running work, including queue removal and library shutdown.
Local-only requests do not download remote artwork URLs left in metadata/cache.
An artwork-only request leaves the file's parsed flag untouched and merges only
metadata, preserving playback details discovered while lookup is in progress.

The library parser still uses the external preparser for file extraction, then
an internal optional fetcher for the populated copy. It commits metadata/tracks
once. Fetched JPEG/PNG artwork is offered through the library's thumbnail
save/hash interface, preserving existing embedded covers. This matters because
the current library analyzer does not consume an album's `ArtworkUrl` field.
The handoff reads only regular local files up to 20 MiB; unsupported or missing
images leave metadata usable. The queue uses the same parse/enrichment separation.

The five-second library lookup timeout is cooperative. Providers that ignore
VLC interrupts are not proven to have a hard deadline. The additional local
thumbnail transfer is not part of a hard wall-clock guarantee.

## Validation

| Check | Result |
| --- | --- |
| Metadata lifecycle, consumer scopes, fallback, shutdown/reopen, child identity, artwork copy | 18 passed, 0 failed/errors/skipped |
| Opening/folder regression | 42 passed, 0 failed/errors/skipped |
| Actual Windows sender/receiver processes | 9 passed, 0 failed/errors/skipped |
| Unchanged native subtitle renderer/preferences suite | 37 passed, 0 failed/errors/skipped |
| Identical lifecycle executable against earlier/fixed Core | 6 failures before; all 8 cases pass after |

All final parent processes exited normally with code zero. No forced cleanup
was used in those runs, and checked personal VLC configuration was unchanged.
Exact binaries, source hashes and individual results are in
[verification.json](verification.json), [metadata-final.txt](metadata-final.txt),
[opening-final.txt](opening-final.txt), [process-final.txt](process-final.txt),
and [subtitles-final.txt](subtitles-final.txt). The eight-case control pair is a
separate earlier executable, not a before/after claim for the expanded 18 cases.
The final audit also reproduces the parsed-flag mistake with the final executable
against the preceding Core (one failure in eight lifecycle cases); the expanded
18-case final run of that same executable passes after correction, including a
duration update made during a delayed successful lookup. Only metadata is merged
back, so newer playback details remain intact.

Network scope was exercised with private owned providers that return generated
local artwork, record process identity and support an interruptible delay. They
make no internet requests and are installed only in isolated test runtimes.
The ordinary package contains no private provider/probe. This proves native
scope selection and lifecycle behavior, not availability or correctness of
every public metadata service. Maintained reproduction instructions are in
[native_library](../../../../modules/gui/qt/tests/native_library/README.md).

Four ordinary desktop launches on monitor 2 confirm enabled local metadata
mode, playback title/artist/album/embedded cover, indexed album cover, and album
and mode persistence after restart. The generated FLAC is silent and owned.
The first three launches test the artwork implementation; the fourth smoke check
uses the final Core after the parsed-flag/metadata-only-merge correction. Each
launch was quit normally and its log ends with the logger stop marker.
The original portable directory was restored byte for byte. Both checked
personal VLC configuration files are unchanged. Screenshot examples:
[embedded tags/cover](03-embedded-tags-cover.jpg),
[indexed album](04-indexed-album-cover.jpg),
[mode after restart](05-local-mode-after-restart.jpg),
[album after restart](06-album-after-restart.jpg).

## Failures retained and resolved or bounded

The user's assertion screenshot matched `config_GetInt`'s missing-option
assertion. A private provider queried an undeclared test option in the external
child. It now checks the variable type and uses the explicit child environment
fallback. The corrected fixture completes without that dialog. This identifies
the reproduced test assertion; the earlier morning transient window did not
retain enough details to establish that it was the same occurrence.

Earlier provider/harness revisions failed or timed out and remain under the
ignored work directory. They are not acceptance runs. One attempted final run
stopped before execution because private modules were still building; it was
rerun after a successful build. The first artwork assertion then correctly
found a missing album thumbnail. The thumbnail handoff above repairs it;
the final check compares the library-owned copy's bytes to the source image.

The first unchanged subtitle regression had one shadow-distance save assertion
failure. The same isolated case passes on both earlier and repaired Core, and
subsequent complete unchanged runs pass all 37 cases, including the final
matched tuple. No subtitle-store change was made; the initial failure's cause
remains unconfirmed. Its report is retained alongside the controlled replays.

## Remaining beta gates

Actual Windows startup with saved media-library OFF is still overridden by the
launcher. Broader real-provider/format coverage, Figma/DPI/full design acceptance,
other unfinished categories, clean licensed packaging, remote CI for these local
changes and publication remain open. No commit, push or release was performed.
The archived twelve shadow proposal files were not consumed.

Ignored work: `build-windows/metadata-20261016`. The final ordinary package is
`build-windows/package`; build and profile backups remain in the work directory.
