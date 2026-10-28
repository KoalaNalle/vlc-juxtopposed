# Timed-packet duration regression — 28 October 2026

All five generated real-writer cases pass on the duration follow-up. Normal MP4
presentation now retains the final video picture and audio block. This is a
separate tested revision from the earlier
[MP4 default-presentation failure](../avformat-writer-smoke/README.md), whose
receipts remain unchanged.

| Case | Strict result |
| --- | --- |
| Default MP4, H264/AAC | All 60 decoded video frames and 116,736 PCM samples exactly match source; all 114 AAC packet payloads/counts match; no video packet has the discard flag. No edit-list override is used. |
| MPEG-TS, MPEG2Video/MP2 | All 60 frames, 115,200 PCM samples and compressed audio/video packets match source. |
| 120 ms Matroska, H264/FLAC | All 3 frames and 5,760 PCM samples match source. The header is written during ES deletion with the 1,500 ms mux-cache threshold. |
| Long Matroska, original deletion order | All 626 FLAC packets, 2,880,528 source-exact PCM samples, 1,500 ordered video markers and both SRT exports/timings pass. Actual A/V deletion order: H264, then FLAC. |
| Long Matroska, reversed deletion order | The same strict completeness checks pass. Actual A/V deletion order: FLAC, then H264. |

Both long runs also preserve audio and video packet timelines within 2 ms. All
five VLC children exit normally with code 0, load 603 modules, select the actual
`avcodec` sout mux through explicit `avformat{mux=...}`, and produce no checked
error/assertion/QML diagnostics. Every applicable source, fixture, producer,
configuration, build/unit receipt, full original/staged runtime and human-config
preservation guard passes. Independent long decoders also verify source,
producer, candidate DLL, config and tool identities after analysis.

H264 compressed payload bytes can differ through parameter-set insertion or
container framing; the MP4 and short Matroska video gates therefore compare
decoded frames exactly. Audio packet payloads/counts and decoded PCM are both
checked. The TS case additionally preserves compressed video payloads.

## Change and exact tuple

The narrow follow-up forwards positive `block_t::i_length` as
`AVPacket.duration` for audio/video as well as subtitles. Unknown or nonpositive
lengths remain at the packet's default zero. PTS/DTS conversion, ownership and
queue selection are unchanged. The [duration-only patch](duration-only.patch)
is relative to the previously qualified tail-repair source `d06d150d…22f35`.

- Mux source: `f2a90481441d369949b4132e0ef67614ab462a5b41f61d3542ca2d952a3c372c`.
- Avcodec DLL: `26e1b8cc539ac5299c8e821dfba36f2449e588af5f8582b4a33c7605c0508cce`.
- Actual runtime Core5: `f8fc980bbe03da57908f79045f266710dd27a16c6ee66b6c45d60cd1f9b93598`.
- Regression configuration: `03b073b9d8a6673fff4ed5f7da9723d17b79fc0459658ce6580ff85ffe3aec13`.

The guarded plugin build actually recompiles the mux and relinks the plugin,
exits 0, preserves its inputs and recoverable prior plugin, and releases its
exclusive writer lock. Its first preparation attempt failed before acquiring
that lock because of a helper-test path typo. That failed receipt remains
preserved; a fresh R2 producer corrects only the lookup and records the successful
build. No dependency installation or Core/Qt rebuild occurs in this lane.

The executed 41-case, 5,772-check helper/FIFO test is reused only for its unchanged
queue ownership scope. It is not described as a new packet-duration adapter
test. The five actual writers and independent decoding checks above qualify
the new adapter on this recorded tuple.

## Evidence and limits

[observations.json](observations.json) records exact source/tool/fixture hashes,
process outcomes, preservation guards, decoded media and packet comparisons,
both long analyses and the successful/failed build provenance. Frozen producers
and their configuration are under `producers/`; generated MP4 default packet
metadata is under `mp4-packets/`. `SHA256SUMS` covers all other files here.
The synthetic short fixtures and shared 533-file baseline inventory remain in
the earlier sibling smoke receipt; they were verified unchanged before staging.

Staging and replacement DLLs use hardlinks without a full-copy fallback. The
native build and runtime lanes respect the owner's C: reserve and bounded
process limits. All ten owned VLC/cache process IDs are confirmed absent after
completion; both independent analyzer commands also finish with code 0. No
compiler, VLC, cache or decoder job owned by this regression remains running.

These are generated, headless real-writer checks. They do not establish the
ordinary Qt Record-toggle journey, design/DPI acceptance, a clean final package,
Windows 10 comparison or beta readiness. No private media, native binaries or
raw personal logs are included. Local original receipts remain under
`build-windows/beta-fps-20261028/tail-drain-repair-r1/adapter-duration-r1/real-writer-regression-r2/`.
The dated producers are evidence, not instructions to rerun them blindly.
