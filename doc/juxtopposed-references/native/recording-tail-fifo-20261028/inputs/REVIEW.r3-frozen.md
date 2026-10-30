# Retiring-FIFO observation: prepared for review, not executed

The patch adds a scalar-metadata snapshot immediately before the existing
avformat DelStream mapping free. It logs stream index, category/kind, remaining
active input count, FIFO count/bytes, each queued block's size/PTS/DTS/duration,
and `VLC_TICK_0`. It never dequeues, requeues, retains or submits a packet, and
does not change readiness, ownership, timestamps, trailer policy or error state.
The fixed 4,096-row metadata buffer is allocated before locking; the queue is
walked read-only while locked; all logging occurs after unlocking. Allocation
failure, truncation, inconsistent counts, missing markers or missing ordinals
invalidate the observation.

`instrumentation-only.patch`, `mux-original.c` and `mux-instrumented.c` are frozen
review inputs. The production source is still the original, SHA-256
`d1de834e1108e03565b3a347830f30debf9948b64315355ba5ae57e0acef777a`.
The generated patch's application is checked read-only during preparation.

## Confirmed configured target and exact tuple

`modules/codec/Makefile.am:399-406` merges avformat into avcodec under
`MERGE_FFMPEG`. The configured home Makefile confirms
`demux/avformat/libavcodec_plugin_la-mux.lo` and `-DMERGE_FFMPEG`.
The incremental target is **`libavcodec_plugin.la`**, build output
`/root/vlc-juxtopposed-build/win64/modules/.libs/libavcodec_plugin.dll`, runtime
path **`plugins/codec/libavcodec_plugin.dll`**. The separately named
`libavformat_plugin.la` is not this configured runtime's mux plugin.

The preserved baseline is
`build-windows/beta-fps-20261028/forced_srt_matroska_clean_candidate/runtime`:

| Component | SHA-256 |
| --- | --- |
| Core r5 | `f8fc980bbe03da57908f79045f266710dd27a16c6ee66b6c45d60cd1f9b93598` |
| libass r2 | `b622871c205345373343a463f193271b12cf875853e7c913c707acadb16e9b40` |
| text demux r1 | `1e8abb4593f82b5badf9dc9d103f655d4a0f30334c28f89e24b102b301139ea8` |
| merged avcodec/avformat r2 | `545f715418c4d20659dce5d6c81ee33b292b21c87243fafa62036b9ffd739e1f` |

Preparation checked every runtime file against its preserved manifest and checked
the original generated fixture/output hashes. `preflight.json` records the exact
counts, source hashes, configured Makefile excerpt and six shared build artifact
identities/link counts. The shared plugin also matches the above r2 hash and has
one hardlink. Current Core sources have changes since r2, but **none of the
project headers/sources appearing in the merged plugin's actual compiled
dependency files changed against the r2 source inventory**. Those dependencies,
system/contrib headers, config and current source inventory are frozen guards;
a changed guard requires a fresh reviewed preparation, not relaxed assertions.

## Execution proposed for root authorization

No build, cache regeneration, VLC run or desktop job has been started. The two
future execution producers require explicit reviewed-execution flags.

1. After other native writers finish, run the WSL build producer. It acquires the
   established `/root/vlc-juxtopposed-build/.vlc-build.lock` nonblocking, rechecks
   the frozen inventory and preserves original source plus six shared plugin/
   object artifacts. It atomically installs only the instrumented source, runs
   `make -C .../win64/modules -j2 V=0 libavcodec_plugin.la`, and copies the new DLL
   into the fresh preparation `build/` directory. Its finally path restores
   original shared artifacts and restores original source last; the source's
   newer mtime makes a later normal build recompile it. Recovery copies and
   exact before/after hashes remain available even if the build fails. The
   process group is bounded to 600 seconds and cleaned on timeout.
2. Only after a successful terminal build receipt proves restoration, run the
   Windows producer. It rechecks frozen producers, restored sources, fixture,
   prior receipt/output and complete preserved runtime. It stages a fresh
   runtime and **unlinks the destination plugin before copying** the probe DLL.
   Every other runtime binary remains the preserved Core r5 tuple. It regenerates
   the isolated cache and launches one owned dummy-interface VLC process with
   the original forced-Matroska AV+two-SRT command and an **85-second** bound.
   Normal exit and unchanged source/fixture/runtime/human-config guards are
   required. Existing module selection/count and diagnostic checks remain.
3. The read-only analyzer uses the installed home WSL FFmpeg tools. It verifies
   complete begin/row/end observations, actual four-stream inventory, audio
   packet hashes/count, decoded sample count and full PCM, all 1,500 video
   markers, and both SRT exports/durations. It records the strict audio failure
   independently; an instrumentation-only run does not become a repair.

Exact future commands, **not run during preparation**:

```powershell
wsl.exe -d VLC-Juxtopposed-Home-20261018 -- /usr/bin/python3 /mnt/c/Modding/vlc-juxtopposed/build-windows/beta-fps-20261028/fifo-tail-probe-preparation-r3/build-probe.py --execute-reviewed-instrumentation
& C:/Modding/vlc-juxtopposed/build-windows/homecoming-20261018/tools/VlcUiTests/Scripts/python.exe C:/Modding/vlc-juxtopposed/build-windows/beta-fps-20261028/fifo-tail-probe-preparation-r3/run-probe.py --execute-reviewed-probe
```

The discriminating prediction is a queued audio block of 619 bytes at normalized
PTS/DTS 60.000 s when its mapping is about to be freed. With output still missing
that source packet, this locates the loss at FIFO teardown. If the tail is absent
and output still loses it, inspect upstream/earlier submission rather than apply
the retained-queue proposal. If output becomes complete under observation,
logging may have perturbed scheduling; record a non-reproduction, not a fix.
No retained-queue repair may be applied solely from this preparation.

The full source has 626 FLAC packets / 2,880,528 samples. Existing AV loses the
last 528; audio-only preserves all of them. Video remains exactly 1,500 markers.
Source fixture SHA-256:
`ff9c28f52e759fcfcfd657dda5af5d3115b29d0255363b208dbbc08c9ae16a21`.

Earlier preparation directories r1/r2 preserve dependency-parser failures only;
they changed no production source and started no build/runtime job. This r3 is
the reviewed complete preparation. Existing historical evidence is untouched.
