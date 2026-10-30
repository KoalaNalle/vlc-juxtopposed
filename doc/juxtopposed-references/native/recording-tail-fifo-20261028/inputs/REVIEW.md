# R4 FIFO-tail observation preparation — no execution

This revision changes only ignored preparation files. Production mux source,
shared plugin/object artifacts, the preserved runtime and r3 remain unchanged.
No build, cache generation, VLC process or desktop operation is authorized by
this preparation or has been launched by it.

The reviewed C patch is unchanged: scalar metadata only, immediately before
avformat DelStream frees its stream mapping. It never dequeues, retains, requeues
or submits a packet. Allocation precedes the FIFO lock, scalar capture occurs
under that lock, and logging follows unlock. See the preserved r3 review for
the C observation contract and configured MERGE_FFMPEG target.

## Strict identities

- Production mux source remains
  `d1de834e1108e03565b3a347830f30debf9948b64315355ba5ae57e0acef777a`.
- Preserved merged avcodec/avformat DLL remains
  `545f715418c4d20659dce5d6c81ee33b292b21c87243fafa62036b9ffd739e1f`.
- Core r5, libass r2, text demux r1 and generated source fixture identities remain
  the exact r3 tuple. The actual build target is `libavcodec_plugin.la` in the
  configured `win64/modules` directory, with two workers and the exclusive
  established WSL native-build lock.
- All535 inherited project input guards and355 actual compiled-dependency guards
  are compared to their existing expected values. A mismatch stops preparation
  or execution; it cannot be repaired by silently refreshing a manifest.
- The original415 manifested binaries remain individually checked. The new full
  runtime inventory additionally guards118 support files:533 files total,
  including scripts/assets and the original cache. The staged533-file set is
  checked before launching, with only the instrumented DLL and newly generated
  isolated cache allowed to differ. Human configuration and source fixtures
  retain their separate before/after guards.

## Storage budget

`preflight.json` itemizes a conservative additional-disk peak, calculated from
the actual six backup sizes and actual DLL size. It includes the original source
and artifact recovery copies, saved candidate DLL, staged replacement DLL,
two DLL-sized linker output/temporary allowances, object/dependency temporaries,
bounded generated output, cache, logs/receipts, directory metadata, and128MiB
snapshot/paging allowance. Candidate DLL growth is allowed16MiB and checked by
the future builder. Marker analysis streams one cropped frame at a time rather
than retaining172.8MB of decoded video.

This is an intentionally conservative sum; it does not assume link scratch has
already disappeared when later files are made. The runtime-only budget excludes
build outputs already present and is rechecked before staging. Both checks use
actual free bytes minus ceil(20% of total C capacity), rather than a flat600MiB
threshold. The preparation records `fits=false` honestly if that upper bound
does not fit; future execution refuses before changing shared sources.

Staging is hardlink-only and fails closed on every link error, including the
NTFS link limit. It never falls back to copying the2.83GB baseline. The owned
destination DLL is unlinked before replacement so original hardlinks stay intact.

## Recovery and receipts

The future builder creates its receipt before preflight. All six original
artifacts and source are saved and hashed before the single instrumentation
write. Every caught interruption/failure terminates and verifies the complete
owned make process group before restoration, including children that outlive
make. Unconfirmed writers prevent restoration and require manual recovery.
Missing linker outputs are recreated from backups; new hardlinks/symlinks and
unexpected source drift are rejected. Shared artifacts restore first and source
last, with newer source mtime requiring normal recompilation afterward.

Final hashing collects missing-file errors instead of aborting receipt generation.
Build success requires clean restoration, clean inspections, stopped writers,
unchanged inputs/producers and a terminal receipt. The runtime producer requires
those explicit build-success fields and cleanup→artifact→source event order.
Its own post-run inspection likewise retains a terminal failure receipt if a
required log/output/file is missing. The analyzer writes an independent failed
analysis receipt if its observation/decode checks fail. Its WSL process group is
bounded by110seconds plus a10second termination grace; the decoder has its own
90second deadline and owned-group cleanup.

## Attribution boundary

The analyzer may associate the missing tail with the retiring FIFO only when
output is exactly625 of the626 source packet payloads, exactly2,880,000 of
2,880,528 samples, byte-identical to the full source PCM prefix with exactly
1,056 missing PCM bytes, and the known final packet hash/619-byte60s timestamp
match. A complete FIFO observation must also contain queued audio metadata
for619 bytes at PTS and DTS60,000,000 normalized ticks. Other differences get
an inconclusive diagnosis. All1,500 video markers and both exact SRT exports/
timings remain independent requirements. Instrumentation is never a repair or
strict audio-completeness acceptance result.

## Review before execution

Only after reviewing the frozen `producer-identities.json`, full preflight,
storage fit and recovery paths should root authorize the two explicit execution
flags. All output directories are one-use; failures remain preserved. No command
from r3 should be rerun unchanged. This preparation does not apply the separate
retained-queue proposal.
