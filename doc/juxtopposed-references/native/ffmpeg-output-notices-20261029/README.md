# FFmpeg output notices, full generated output and cancellation

29 October 2026. This is frozen evidence for named scopes, not beta readiness.
All jobs and the ordinary VLC window were terminal at the user-requested pause.

## Identified build and execution

The configured Windows build merges FFmpeg into `libavcodec_plugin.dll`; the
separate avformat target is disabled. Plugin R2 normally recompiles its eleven
translation units and two common units with two workers under the native lock.
All 356 actual compiler dependencies belong to its pre-build inventory; 519
inventory entries also guard static link inputs, producers and source receipts.
All 47 owned shared artifacts were restored, with Core, Qt, TTML, WebVTT and the
closed R28 runtime preserved. Compiler exit 0; no live writer; lock released.
The existing deprecated FIFO-count warning is retained in the build log.

- Candidate FFmpeg plugin: `3022efec963d223c4b8ac3195bb23068f20a4a0413e87db0ca188cb40a73aa38`.
- Before plugin: `26e1b8cc539ac5299c8e821dfba36f2449e588af5f8582b4a33c7605c0508cce`.
- Current test Core: `ccb6a2005ca575c0ab99277864eb23f7da824407615a45c06eb0ef52a2ab6e6a`.
- Ordinary Qt: `7c7db5cb675edc20b3366120dc55282b09d2db2dc731b7c6ed8c1815e0706624`.
- Generated source: `ff9c28f52e759fcfcfd657dda5af5d3115b29d0255363b208dbbc08c9ae16a21`.

The earlier queue-only tests remain 41 cases/5,772 checks before and 51/6,062
after, on their own identified Core. Those are not FFmpeg/provider integration.

## Actual callback comparison

Six rows in callbacks R3 are qualified: old/new MP4 header failure, old/new
Matroska write failure after the parent closes its stdout sink, and old/new
successful Matroska output. All six child exits are 0 without forced cleanup,
603 modules and the actual `avcodec` sout mux selected. Old failures produce no
error callback; each new failure produces exactly one callback titled
“Output could not be completed”, including subsequent Close. Successful runs
produce no callbacks or captured error diagnostics. The worker registers the
real public LibVLC error callback and uses trusted media-item output options.

Two additional rows in callbacks R5 qualify cancellation. A paced parent keeps
the pipe open, observes 8,192 actual output bytes, and signals the worker. Both
workers request asynchronous Stop, observe Stopping then Stopped, release normally
and drain a valid final output without closing the sink early. Old PID31208 and
new PID1984 each exit 0, reader joined, 84,196 output bytes, no reader errors,
no error callback and no forced cleanup. Cancellation happens during real output;
the unpaced media clock can still read zero and is not used as its proof.

All R3/R5 runtime, fallback, source/input and human-profile guards pass.

## Complete generated output

Output analysis R3 qualifies both successful R3 files against the entire source:
all 1,500 decoded video frames, exact 5,761,056 PCM bytes (2,880,528 mono samples),
all 626 FLAC packets including the tail, and both three-packet SRT tracks with
exact exported text and timestamps/durations within 2 ms. Audio PCM SHA256 is
`7ea9ed935c066e9d388c931f6aa87b86399425be6642c23dab662e9a49ffe028`.

VLC registers subtitles first, so output stream IDs differ from source IDs.
Streams are matched by category and ordinal. H264 keyframes contain repeated
SPS/PPS parameter sets: six packets gain 39 bytes. Raw video packets therefore
are not claimed identical. After converting both streams to Annex B and removing
NAL types7/8, all remaining encoded video bytes match exactly (SHA256
`079eff3ce73c1bed890e0cf7b52e4282ff46aa1cc100420c925f2923922b116a`).
All decoded frames and packet clocks are independently compared.

## Ordinary GUI R29

Runtime: `build-windows/beta-record-notice-20261029/gui-r29/runtime`.
A copied generated profile opens a deliberately incompatible MP4 passthrough
item followed by the ordinary two-language MP4. The actual Qt error provider
shows the new toast, including accessible Show Details and Dismiss controls.
The successor opens normally at its held origin and renders the Finnish middle
caption. Returning to the first source reproduces the notice.

Show Details is **not qualified**: both attempted indexed actions found that the
five-second toast had expired. No Messages window was opened. The app subsequently
closed normally, child/observer exit0,603 modules, all preservation guards true.
Its close was observed, not attributed to an automation Quit action. Expected
header/stream/finalization diagnostics remain in verification.json. No raw human
logs or private media are exported. A fresh run must finish Details and recovery;
R29's owned window/old handles are closed and must not be reused.

## Failed setup/oracle attempts are preserved

- Plugin R1 compiled successfully but failed its dependency inventory because
  preflight omitted the actual `MERGE_FFMPEG` define and included avformat.c.
  Shared artifacts restored; no R1 artifact was qualified. R2 fixes that flag.
- Callback R1 mistook the initial Stopped state for completion; no mux ran.
- Callback R2 used global sout options, which LibVLC's player does not inherit;
  it played normally without the intended output. R3 uses trusted item options.
- R3's cancellation row finished before cancellation and remains unqualified.
  Its six separately passed rows are usable; the overall R3 receipt says false.
- R4's producer accidentally launched a second cancellation child during checks.
  Its final report is contradictory: `passed=true` coexists with child exit1,
  normalExit=false and a reader FileExistsError. **Reject R4**, including its
  displayed overall pass. R5 has one call per row and final exit assertions.
- Output analysis R1 assumed unchanged stream IDs; R2 rejected expected repeated
  H264 parameter sets. R3 preserves packet clocks and independently checks decoded
  content plus normalized encoded payload. Both earlier failures remain.

The current maintained worker differs from the earlier R1/R2/R3 workers. Frozen
producer snapshots document those versions; a later edit cannot inherit a past
worker hash. No current source claim is inferred solely from a passed verifier.

## Remaining scope

Finish the ordinary Details interaction, record-toggle/probe silence with this
plugin, other failure phases/containers and the distinct conversion workflow.
Do not infer all-format, arbitrary PCM, Win10, visual/DPI, multi-instance or clean
package acceptance. All BETA gates remain open. No dependency/security changes,
Git commit, push or publication occurred.
