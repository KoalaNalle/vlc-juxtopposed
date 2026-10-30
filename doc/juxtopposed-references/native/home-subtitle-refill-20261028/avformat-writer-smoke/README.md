# Generated avformat writer smoke —28 October2026

These are bounded real-writer checks of the local committed-tail repair. They
are separate from the41-case queue-helper test and the main long Matroska,
opposite-deletion-order and I/O-error qualification. They establish neither an
ordinary Qt recording journey nor beta/final-package acceptance.

| Format / generated source | Old plugin | Tail-repair plugin | Scope |
| --- | --- | --- | --- |
| MPEG-TS /2.4s MPEG2Video+MP2 |60frames;114048 PCM samples |60frames;115200 PCM samples | Candidate decoded video, PCM, and all compressed packet payloads/counts exactly match source. Control loses one1152-sample MP2 frame. |
| Matroska /120ms H264+FLAC |3frames;4608 PCM samples |3frames;5760 PCM samples | Candidate decoded video and PCM exactly match source. Both logs write the header only after the first ES deletion, exercising forced startup below the1500ms mux-cache threshold. Control loses1152 samples. |
| MP4 /2.4s H264+AAC, default edit lists |59frames;113664 PCM samples |59frames;115712 PCM samples | **OPEN.** Both default presentations hide the final video picture. Candidate retains more audio but default output is not source exact. |
| Same MP4 files, decoded with `ignore_editlist=1` |60frames;114688 PCM samples |60frames;116736 PCM samples | Candidate video and PCM exactly match source. The complete candidate payload is present; default edit-list/track-duration presentation still crops it. Control remains2048 samples short. |

All six R2 VLC processes exit normally with code0, load603 modules, select the
actual `avcodec` sout mux through explicit `avformat{mux=...}`, and produce no
checked error/assertion/QML diagnostics. Candidate/Core5 identities are bound
below; no production edits or native compiler runs occurred in this smoke lane.
The ten source, fixture, producer/config, build-receipt, candidate, full original
runtime, full staged runtime and human-configuration preservation guards pass.

R1's source-exact MP4 assertion failed. Its frozen receipt and producers are
preserved locally, and the failure/hash are recorded in `observations.json`.
R2 completes the matched observations and guards; its producer exit0 does **not**
turn MP4 into an accepted format. R2's precise preexisting MP4 limitation is
recorded, and MP4 default presentation remains an unresolved task.

## MP4 diagnosis

The candidate contains60 H264 packets. With default edit-list handling its final
video packet has the discard flag, although its payload hash matches the final
source packet. The video stream reports60 frames and2.360s duration; its final
picture falls outside the presented duration. With edit lists ignored, all60
pictures decode to the exact source SHA256
`1124a8fac11b4fb573df093e020456c8bf38871288eff7e2b61265a551cecf30`.

Default probing exposes113 candidate AAC packets; ignoring edit lists exposes
all114. The latter decodes to116736 source-exact samples, SHA256
`e41ed49dd2a247af20d2b374ca517cb35229737ad052f75b82181320a97376ab`.
No packet `skip_samples` side data appears in either inspection. The saved
default/ignore-edit-list packet metadata permits inspection of PTS, DTS,
durations, flags and payload hashes. The adapter currently forwards explicit
packet duration for SPU only; this is a follow-up hypothesis, **not a tested
duration repair**.

## Identities and artifacts

- Core5: `f8fc980bbe03da57908f79045f266710dd27a16c6ee66b6c45d60cd1f9b93598`.
- Old avcodec: `545f715418c4d20659dce5d6c81ee33b292b21c87243fafa62036b9ffd739e1f`.
- Candidate avcodec: `79a565a9398756aaf22ceb194771a9599c499771971638bfdfb06e9fc531882f`.
- Mux source: `d06d150d3fdb92c80fce3afe47af6bf7933876d81900a2dac5d5becea7422f35`.
- Exact four-source identities, tool identities, per-fixture commands/hashes,
  packet/decode observations, process results and preservation guards are in
  [observations.json](observations.json). The full533-file preserved runtime
  inventory is recorded once, separately.
- `fixtures/` contains only the three287823-byte-total synthetic sources.
  `producers/` retains the frozen generation, R1/R2 observation and MP4 diagnostic
  producers/configs. `mp4-packets/` contains generated-media metadata.
- `SHA256SUMS` covers every other file in this directory. No native binaries,
  private media, raw personal logs or personal profile files are included.

All runtimes, including replacement candidate DLLs, were hardlinked without a
copy fallback. File-identity inspection confirms each staged candidate is the
same148292772-byte file as the preserved build output. Across R1 and R2, unique
new non-baseline file data totals6212378 bytes before this public export;
directory/file metadata is additional. The producer reserved96MiB and checked
the owner's20% C: free-space floor before staging. No new runtime was launched
after the subsequent storage hold.

Local frozen originals remain under
`build-windows/beta-fps-20261028/tail-drain-repair-r1/` in
`writer-smoke-r1`, `writer-smoke-r2` and `mp4-editlist-diagnosis-r1`.
These dated producers are evidence, not current instructions to rerun them.
