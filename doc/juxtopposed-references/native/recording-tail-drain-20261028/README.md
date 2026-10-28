# Recording tail repair — 28 October 2026

This frozen candidate repairs committed packets discarded when an input ends.
The earlier [FIFO observation](../recording-tail-fifo-20261028/README.md) found the
missing 619-byte/528-sample FLAC tail still queued at input deletion.

| Evidence | Actual result |
| --- | --- |
| Production queue helper | 41 native cases, 5,772 checks, zero failures; all 24 A/V/S/S deletion orders, ownership, live FIFO gaps/flush, header/write/trailer failures and cleanup. |
| Unchanged codec control | Reproduces 625/626 audio packets and 2,880,000/2,880,528 samples, exact source prefix; video/subtitles/timing remain intact. This is the required negative control. |
| Repaired real Matroska writer | All 626 packet payloads and 2,880,528 decoded audio samples exactly match the source. All 1,500 frame markers, both exact SRT exports/durations and audio/video timelines pass. |
| Opposite actual A/V deletion order | The remapped fixture produces audio-before-video deletion instead of video-before-audio; the same complete output checks pass. Existing Core add/remove logs identify order without another observation build. |
| Incompatible MP4 header | Expected native header failure and failed finalization, normal process exit, no forced cleanup. This deliberately incompatible subtitle fixture is a negative test. |
| Closed owned output pipe | Reader accepts 4,096 bytes then closes only its own sink. Native Broken pipe/frame failure and failed finalization are observed; child and reader finish normally, with no assertion/forced cleanup. |

Every runtime run loads 603 modules, selects the actual avcodec/avformat mux and
passes nine explicit preservation guards. Positive/control media runs have no
checked diagnostics; the two deliberate failure cases retain expected errors.
Unit, module build, runtime and independent decoding have distinct scopes.

The source snapshot is mux `d06d150d3fdb92c80fce3afe47af6bf7933876d81900a2dac5d5becea7422f35`,
queue helper `37eaa3c00753f5dc34ba43632ceb928492bd04f1277d9a7c125cc5e3893701d2`,
test `34a467dedff11259b4612b52646b2a20cd506f2887e29d9b8fac359658699e2b`.
The rebuilt codec is `79a565a9398756aaf22ceb194771a9599c499771971638bfdfb06e9fc531882f`.
Runtime Core remains r5 `f8fc980bbe03da57908f79045f266710dd27a16c6ee66b6c45d60cd1f9b93598`;
the full support/renderer/demux identities are in the frozen runtime configuration.
The ordinary Home candidate uses another Core and needs integrated qualification.

The module owns only queues detached at Delete; active FIFOs remain Core-owned,
with original readiness/flush semantics. Packets are merged by original DTS;
only a successful complete drain permits trailer finalization. Header/write
failures are sticky. ASS mapping and subtitle durations stay preserved.
Additional guards handle failed pre-keyframe flush/header I/O/allocation.
The allocation branch is source-reviewed, not an actual forced-OOM test.

The sister [container smoke receipt](../home-subtitle-refill-20261028/avformat-writer-smoke/)
has exact MPEG-TS and short-startup Matroska results. MP4 default presentation
remains open: ignoring edit lists recovers complete payload, while default
playback crops the ending. A later duration repair is a separate candidate.
This receipt does not close ordinary Record-toggle/UI, all format/hardware,
accessibility/DPI, integrated-current-tuple or final-package beta gates.

Frozen producers are one-use local records: review/adapt into fresh directories
before reuse. Native source/header, exact input inventories, compile/link logs,
unit results and selected runtime/decoder results are included. Personal
configuration hashes, raw VLC logs, media, binaries, databases and dumps are not.
The generated remap command and content/timing proof are retained without media.
The inventory preserves bytes across Git newline conversion.
