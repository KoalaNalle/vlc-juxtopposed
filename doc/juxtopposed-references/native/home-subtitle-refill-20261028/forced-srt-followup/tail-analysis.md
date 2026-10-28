# Forced Matroska FLAC tail diagnosis

Supplementary read-only analysis, 28 October 2026. The earlier comparison,
verification records, producers and artifact manifest remain unchanged. This
report does not accept the audio-completeness gate or claim a repaired runtime.
All three inputs to the analysis are preserved generated-media artifacts under
`C:\Modding\vlc-juxtopposed\build-windows\beta-fps-20261028\`.

## Reproduced facts

| Artifact relative to that directory | Checked SHA-256 | FLAC packets/decoded frames | Decoded samples |
| --- | --- | ---: | ---: |
| `media-record-flac/frame-marker.mkv` | `ff9c28f52e759fcfcfd657dda5af5d3115b29d0255363b208dbbc08c9ae16a21` | 626 | 2,880,528 |
| `forced_srt_matroska_clean_baseline/output.mkv` | `62850c54552e74d8fa045a4d4561ac7eeb294a5de881f27137832710af6e69c7` | 625 | 2,880,000 |
| `forced_srt_matroska_clean_candidate/output.mkv` | `f1296003bcf8cf5d2f2c8181f2092b0a793ec68301fc2a052530b39da6e0f4f3` | 625 | 2,880,000 |

Both outputs contain the exact first 625 compressed audio packet payloads from
the source, in order. Both s16 PCM decodes also match the exact source prefix.
The absent source packet is at PTS/DTS **60.000 s**, is **619 bytes**, and decodes
to **528 samples** (11 ms at 48 kHz mono). Its packet SHA-256 is
`7b2a0c0f0e91ab6804b11077160743336c3ead5f7c1c583e4a4c072946766343`.
The preceding source/output packet is at 59.904 s and decodes to 4,608 samples.

The missing tail contains **772 nonzero bytes among 1,056 bytes** when decoded
to mono s16le. This is a byte count, not a nonzero-sample count or a perceptual
audibility assessment. The FLAC source is 24-bit; the s16 conversion is used
here for a consistent comparison, while `ffprobe` independently confirms the
frame/sample counts. Source PCM SHA-256 is
`7ea9ed935c066e9d388c931f6aa87b86399425be6642c23dab662e9a49ffe028`;
both output PCM hashes are
`6a90de2906192a3ab98215c50b455fb02aa96879726b30f83ea4d6ff3e23d88f`.

All three FLAC streams report `initial_padding=0` and identical 34-byte
STREAMINFO extradata, SHA-256
`a9dfe39d93c57653e8fb5ed971128c064d0ed9d8ba4181efe379bc047c9cbd4e`.
The source's audio duration tag is 60.011 s. The source was itself made by
transcoding the fixture MP4 audio to FLAC; the analysis does not determine whether
the original AAC encoding introduced padding. It establishes that an actual
compressed FLAC source packet is absent from both passthrough outputs. A duration
tolerance or identical baseline/candidate audio cannot close this failure.

The frozen producer requests forced libavformat Matroska passthrough with
`--play-and-exit`; it supplies no stop-time, run-time or shortest-stream option.
Both runs exited normally and their logs remove video, then audio, then the two
subtitle streams after EOF. The earlier comparison still separately establishes
all 1,500 video markers and the candidate's repaired subtitle durations/exports.

## Read-only reproduction

The installed tools are `/usr/bin/ffprobe` and `/usr/bin/ffmpeg` in WSL distro
`VLC-Juxtopposed-Home-20261018`. These are the exact argument forms used for each
of the three absolute WSL paths below; stdout was captured in memory, with no
media or report files overwritten:

```text
/mnt/c/Modding/vlc-juxtopposed/build-windows/beta-fps-20261028/media-record-flac/frame-marker.mkv
/mnt/c/Modding/vlc-juxtopposed/build-windows/beta-fps-20261028/forced_srt_matroska_clean_baseline/output.mkv
/mnt/c/Modding/vlc-juxtopposed/build-windows/beta-fps-20261028/forced_srt_matroska_clean_candidate/output.mkv
```

For each path as `FILE`:

```sh
ffprobe -v error -select_streams a:0 -show_streams -show_packets -show_data_hash sha256 -of json "$FILE"
ffprobe -v error -select_streams a:0 -show_frames -show_entries frame=pts_time,nb_samples -of json "$FILE"
ffmpeg -v error -threads 1 -i "$FILE" -map 0:a:0 -f s16le -
```

The first probe supplies packet payload hashes, counts, timestamps, sizes and
extradata hashes. The second supplies decoded `nb_samples`; sum these integers
to reproduce the sample totals. Python's `hashlib.sha256(path.read_bytes())`
checks the file hashes. Hash captured PCM bytes likewise, divide their lengths
by two for mono s16 samples, compare each output to the source prefix and count
`byte != 0` in the remaining source tail. The compact results are in
[tail-analysis.json](tail-analysis.json).

## Static inference: mux FIFO teardown is the strongest lead

The following is a source-based hypothesis, not an instrumented proof of which
stage held the missing packet:

- `modules/demux/avformat/mux.c` calls `sout_MuxGetStream(..., 1, ...)` from its
  `Mux` loop. Its `MUX_CAN_ADD_STREAM_WHILE_MUXING` answer is false.
- `src/stream_output/stream_output.c:520-525` returns no selectable stream when
  any non-subtitle input FIFO is empty under that policy. A final audio packet
  after the last video packet can therefore remain queued in the mux input FIFO.
- `sout_MuxDeleteStream` at lines 445-466 invokes another mux pass only if initial
  stream waiting is still active. Normal removal releases the input FIFO.
- The observed avformat `DelStream` at `mux.c:372-375` only frees its stream
  mapping; it does not drain queued packets. The final 619-byte audio packet
  could be released there after video has already been removed.

This fits the exact-prefix loss and the recorded removal order. The packetizer's
NULL-input drain path exists (`DecoderThread_ProcessSout` and
`vlc_input_decoder_Drain` in `src/input/decoder.c`), and earlier FLAC tail repairs
are present, but neither fact alone proves the final packet reached this mux.
No source changes or new native runs were made for this analysis.

## Next discriminating test

Using a new isolated run and the same source/runtime/avformat mux, select only
the audio output while suppressing video and subtitle outputs. Compare all
packet payloads, decoded samples and normal exit with the preserved AV result.
If all 626 packets survive audio-only, the multi-input mux scheduling/deletion
path is implicated. If the same final packet is absent, trace the Matroska demux
and FLAC packetizer drain before changing mux teardown. Retain strict source
sample equality in either case. A subsequent repair must cover both track-end
orders and preserve the existing subtitle-duration and full-video results.
