# Audio-only Matroska tail comparison

28 October 2026. This is a new supplementary native test; the earlier AV
comparison and [read-only diagnosis](tail-analysis.md) remain unchanged.

## Result

The audio-only output preserves **all 626 FLAC packets and all 2,880,528 decoded
samples**, including the 528-sample final packet at 60.000 s. Every compressed
packet payload and every decoded s16 PCM byte matches the source. `ffprobe`
confirms exactly one output stream, FLAC audio, with no video or subtitle streams.
The same preserved runtime's prior AV output still contains 625 packets and
2,880,000 samples. The complete source tail can pass through these demux,
packetizer and avformat binaries when video/subtitle outputs are disabled.

| Artifact | Checked SHA-256 | Audio packets | Decoded samples |
| --- | --- | ---: | ---: |
| Source `media-record-flac/frame-marker.mkv` | `ff9c28f52e759fcfcfd657dda5af5d3115b29d0255363b208dbbc08c9ae16a21` | 626 | 2,880,528 |
| Prior AV `forced_srt_matroska_clean_candidate/output.mkv` | `f1296003bcf8cf5d2f2c8181f2092b0a793ec68301fc2a052530b39da6e0f4f3` | 625 | 2,880,000 |
| Audio-only `forced_srt_matroska_audio_only_r1/output.mkv` | `922f96b22df4da1b323b76e790c21ea6a05e3da69a9c7245a3bc8eade752a804` | 626 | 2,880,528 |

Paths in the table are relative to
`C:\Modding\vlc-juxtopposed\build-windows\beta-fps-20261028\`.
The audio-only output's full decoded PCM SHA-256 is
`7ea9ed935c066e9d388c931f6aa87b86399425be6642c23dab662e9a49ffe028`,
matching the source. Its final packet payload SHA-256 is
`7b2a0c0f0e91ab6804b11077160743336c3ead5f7c1c583e4a4c072946766343`.

## Identified runtime and process

The fresh ignored run hardlinks the preserved
`forced_srt_matroska_clean_candidate/runtime` into a new directory. Every staged
binary identity is checked against that accepted AV run; none is rebuilt or
replaced. The four selected identities are:

- Core r5: `f8fc980bbe03da57908f79045f266710dd27a16c6ee66b6c45d60cd1f9b93598`.
- libass r2: `b622871c205345373343a463f193271b12cf875853e7c913c707acadb16e9b40`.
- Text subtitle demux r1: `1e8abb4593f82b5badf9dc9d103f655d4a0f30334c28f89e24b102b301139ea8`.
- avcodec/avformat plugin r2: `545f715418c4d20659dce5d6c81ee33b292b21c87243fafa62036b9ffd739e1f`.

One owned headless VLC playback process, PID 27900, exits **0** normally without
forced cleanup. The helper exits 0 after its read-only analyzer. The log loads
603 modules, selects the `avcodec` stream-output mux (explicit libavformat
Matroska), and contains no checked diagnostics. Source, fixture, producer,
previous verification record, preserved runtime, staged runtime and human
configuration guards all pass. A plugin-cache regeneration precedes playback.
No GUI/desktop input, native build, dependency install or production edit occurs.

## Reproduction and evidence

The supported `sout-video` and `sout-spu` booleans are declared in
`src/libvlc-module.c` and consumed by `EsOutSelectEs` in `src/input/es_out.c`.
The frozen [producer](run-forced-srt-audio-only-r1.py) retains the earlier command
and adds only output-selection switches:

```text
--sout-all --no-sout-video --no-sout-spu --sout-audio
--sout=#standard{access=file{no-overwrite},mux=avformat{mux=matroska},dst=.../output.mkv}
```

It uses a fresh portable/configuration root, `--ignore-config`, dummy interface,
no recent-history/network metadata/library discovery, and `--play-and-exit`.
The source is read-only. A fresh cache is generated without inheriting a duplicate
`VLC_PLUGIN_PATH`. The complete command, runtime manifest and preservation results
are in [audio-only-verification.json](audio-only-verification.json).

The analyzer runs the installed WSL FFmpeg tools against the source, prior AV
output and audio-only output. It probes the complete stream inventory, hashes
all audio packet payloads, sums independently decoded `nb_samples`, and compares
full s16 PCM bytes. The exact arguments are frozen in the producer and the
results are in [audio-only-comparison.json](audio-only-comparison.json).
[Supplementary file hashes](audio-only-artifact-sha256.json) cover these new
receipt files without rewriting the earlier artifact manifest.

## Conclusion and limits

This test narrows the preserved AV loss to behavior that depends on the additional
outputs. It strongly supports the mux FIFO scheduling/deletion hypothesis in
`tail-analysis.md`; it does not instrument or prove the exact internal drop site.
An audio-only pass does not repair or accept AV recording completeness. Keep the
strict 528-sample failure open until a matched AV repair preserves every source
packet/sample while retaining video and subtitle results. Qualify both audio-
first and video-first end orders and normal stream teardown in the repair.

These results apply to the preserved Core r5 tuple. They do not qualify the newer
Home Core/GUI, the final package, all media formats, or beta readiness.
