# Recording tail: FIFO retirement observation, 28 October 2026

The observation-only probe reproduces the missing audio tail and finds its
metadata still queued when the audio input retires. **This is diagnosis, not a
repair or an audio-completeness pass.** The builder and owned runtime both ended
with exit 0 because preparation, instrumentation and observation succeeded;
`strictAudioCompleteness` remains **false**.

## Observed result

| Measurement | Generated source | Recorded output |
| --- | --- | --- |
| Audio packets | 626 | 625 |
| Decoded audio samples | 2,880,528 | 2,880,000 |
| Video markers | 1,500 | All 1,500 retained |
| Two SRT exports | Reference text/timing | Both exact; both timings retained |

The output's 625 audio packet payloads and decoded PCM are exact prefixes of the
source. The missing tail is **528 samples / 1,056 decoded PCM bytes**. Neither
the full packet sequence nor full PCM equals the source.

Immediately before the existing avformat `DelStream` mapping release, the audio
FIFO reports one queued packet: **619 bytes, PTS/DTS 60,000,001, duration 11,000**.
The logged `VLC_TICK_0` is 1, so that timestamp represents the 60-second position.
The snapshot is complete, untruncated, and captures its sole queued block. The
retiring video and two subtitle inputs have empty FIFOs.

The exact audio instrumentation lines are:

```text
avcodec debug: tail_probe begin version=1 stream=3 cat=2 kind=audio active=2 count=1 bytes=619 captured=1 truncated=0 complete=1 zero=1
avcodec debug: tail_probe row stream=3 ordinal=0 size=619 pts=60000001 dts=60000001 duration=11000
avcodec debug: tail_probe end stream=3 complete=1
```

[All nine scalar probe lines](run/tail-probe-lines.txt) and the unchanged
[analysis](run/analysis.json) preserve the observation. No other VLC log lines,
device/human logs, media, binaries or settings databases are packaged here. The
queued 60-second tail and exact short source prefix identify loss at retirement;
the observation does not implement or validate a drain/retirement repair.

## Probe and preservation

The [patch](inputs/instrumentation-only.patch),
[original C](inputs/mux-original.c), [probe C](inputs/mux-instrumented.c) and
[scalar helper](inputs/probe-helper.c.txt) are frozen inputs. The helper allocates
a bounded 4,096-row metadata array before locking, reads the FIFO while locked,
then logs after unlocking. It does not dequeue/requeue/retain/submit packets or
change ownership, timestamps, readiness, trailer policy or error state. An
incomplete, truncated or inconsistent observation is rejected.

- [Build verification](build/verification.json) records the writer lock acquired
  and released, owned writers gone, no forced cleanup, and original source plus
  all six shared build artifacts restored. Production/dependency input guards
  and producer guards pass; verification/restoration/cleanup error lists are empty.
- Restoration finished before the [runtime](run/verification.json) started.
  The runtime exits normally with code 0, 603 modules, the intended merged mux
  selected, no checked diagnostics, no forced cleanup and all eight preservation
  guards true. Only the isolated runtime receives the instrumented plugin.
- This receipt checks **frozen preparation files and recorded guards only**.
  It does not reread or hash current production files after subsequent repair
  work begins. The original shared source/artifact identities are the historical
  observation's identities, not a claim about the current checkout.

The configured `MERGE_FFMPEG` target is `libavcodec_plugin.la`; avformat's mux is
inside runtime `plugins/codec/libavcodec_plugin.dll`. The observation retains the
preserved FPS runtime tuple except for that isolated plugin replacement:

| Artifact | SHA-256 |
| --- | --- |
| Preserved Core | `f8fc980bbe03da57908f79045f266710dd27a16c6ee66b6c45d60cd1f9b93598` |
| Preserved libass | `b622871c205345373343a463f193271b12cf875853e7c913c707acadb16e9b40` |
| Preserved text demux | `1e8abb4593f82b5badf9dc9d103f655d4a0f30334c28f89e24b102b301139ea8` |
| Original merged mux plugin | `545f715418c4d20659dce5d6c81ee33b292b21c87243fafa62036b9ffd739e1f` |
| Isolated probe plugin | `269fd643d9d3e8c84fc00dd0ca6b6a5dca49453354632e230e43d61c96a686a5` |

## Receipt contents and limits

[Summary](summary.json) captures the counts and preservation results.
[Producer identities](producer-identities.json) match all **17 frozen inputs**
under `inputs/`, including original/probe sources, patch, reviewed preparation,
configured/dependency guards and build/run/analyzer producers. Frozen review
documents retain their preparation-time wording; terminal build/run records
establish the later execution. Their one-use paths and authorization arguments
are historical evidence, not a request to rerun them unchanged.

[Copy verification](copy-verification.json) records byte-identical copies and the
exact selected scalar-log line numbers. The runtime verification omits
`humanBeforeSha256` while retaining `humanConfigUnchanged`; its sanitization
record retains the original verification hash. The unchanged analysis references
that original hash, not the sanitized public file's hash. `artifact-hashes.json`
inventories every receipt file except itself, and `.gitattributes` preserves the
recorded bytes through Git.

This is one generated AV-plus-two-SRT reproduction on its recorded tuple. It
does not establish other formats' behavior, real-device playback, Windows 10,
ordinary GUI behavior or general recording completeness. A repair must separately
pass full packet/PCM completeness and preserve the video/subtitle results.
