# Proposed bounded avformat output-error notification

Status: review proposal only. The preimages and checksums here are copies of the current source, not an implementation or a compiled candidate. No native run was launched for this proposal.

## Evidence and scope

`sanitized-baselines.json` summarizes three immutable baseline receipts. R3 and R4 are setup failures, not negative controls. R5 confirms a working LibVLC error callback, a real successful avformat file output, and an avformat closed-pipe write which logs failure yet yields no dialog callback. The recording is shortened to two seconds. This does not locate the failure in the final tail, establish a result returned to a conversion controller, or prove ordinary Qt GUI feedback.

## Why probe output is different

In `modules/stream_out/record.c`, `OutputStart` iterates mux candidates, creates each temporary output with `OutputNew`, adds ES IDs, deletes those IDs, destroys that output chain and unlinks its temporary file. There is no `OutputSend` or `sout_StreamIdSend` in the candidate loop. The real output is created only after that loop; buffered data are forwarded afterward. The top-level record send/delete callback holds its `sout_stream_private` lock (see `src/stream_output/stream_output.c`, `sout_StreamIdSend` and `sout_StreamIdDel`), which serializes operations on that record stream while the loop runs. `DelStream` can nevertheless cause avformat to attempt a header without data: such a header failure alone is not evidence of a real recording failure and must not produce a user-facing error. Confirm an actual selected-avformat probe separately; whether a given probe reaches that module depends on mux selection.

## Minimal proposed state and processing

1. Add per-mux booleans initially false: `payload_observed` and `failure_reported`. Payload means a nonempty media block observed by this mux, never a header or an empty block.
2. Before each `avformatMuxRun` call from `Mux`, inspect each registered live FIFO under its own lock with `vlc_fifo_GetBytes`; inspect each retired stream's committed chain for a block whose `i_buffer` is nonzero. Latch `payload_observed` if any actual payload bytes are found. Do not hold FIFO locks while invoking any queue handler or dialog. A nonzero `MuxBlock` can also latch this state, including committed tail work in finalization.
3. If `Mux` returns failure, or finalization returns failure, attempt one generic error notification only when both `payload_observed` and not `failure_reported`; set `failure_reported` before invoking `vlc_dialog_display_error`. Consider title `Output could not be completed` and detail `The output may be incomplete. Check the messages for details.` Preserve the existing logs and return codes. A later `Mux` call that finds real payload must report an already-sticky header failure from an earlier empty probe-like phase. A later `Close` must not report the same mux failure twice.
4. A pending header with no bytes, no output provider, and zero-size-only input need explicit separate handling in tests. The one-attempt guarantee is per mux instance, not across other output stages. If the dialog provider is absent or interaction is disabled, its existing fallback applies; do not alter the user's choice.

This is an observation latch, not a claim that the mux sees every enqueue: bytes sent then flushed before the first mux scan may be invisible. It also reports only failures surfaced by the existing queue. `MuxBlock` checks `av_write_frame` return and `WriteTrailer` checks `av_write_trailer` return; buffered AVIO errors may require an additional error check or flush to detect, which needs its own careful review and phase-specific evidence. This proposal must not be described as complete failure coverage without that work.

Dialog callbacks may run synchronously while an outer stream lock is held; the existing record creation error uses the same API but does not prove arbitrary client reentrancy is safe. Test with a callback that only copies strings and returns, then clean shutdown. Analyze whether the reporting path can be made safely deferred if reentrant control is a requirement; do not assert it is safe based solely on the existing convention.

## Cancellation and completion semantics

The conversion controller sets an application-side cancel flag then stops and deletes its native player. This reason is not passed to generic avformat mux callbacks. Consequently a genuine output error during cancellation and an unwanted cancellation-only message cannot be distinguished by this generic mux alone. The present conversion workflow uses its own FLAC path, so avformat coverage must not be presented as conversion-result propagation. Test ordinary cancel separately; if distinguishing cancel from failure is a requirement, establish a Core/API propagation mechanism rather than claiming this latch implements it. A notification is not a reliable success/failure return to the conversion UI, which remains open scope.

## Required evidence before accepting a candidate

- Component: an empty add/delete probe that fails header does not report; subsequent nonempty payload on an already failed queue reports once; success has no report; header, frame, and trailer failures after observed payload each report once; retired streams, multiple streams, repeated `Mux`, and close deduplicate; no provider does not crash. Tests must exercise real guard and lifetime logic, not duplicate it.
- Headless native: positive LibVLC callback registration control; selected avformat probe with Add/Del and no Send and a witnessed failing header must produce zero output-error callbacks; normal writer must produce output and zero such callbacks; first-data/header failure and the qualified closed-pipe write each must produce exactly one relevant callback with a phase witness and normal process teardown. Record callbacks of unrelated types separately. A deterministic test-owned sink or equivalent may be needed to witness selected frame and final/trailer failures. Do not add a production fault hook or claim tail-only coverage for the existing pipe test.
- Run controls in isolated owned output/profile paths, fixed runtime and fixture identities, guard the human profile and frozen baseline, use bounded timeouts and preserve failed receipts. Test the normal cancel and failure-during-cancel cases, record their actual outcomes and the unresolved semantic distinction.
- Ordinary GUI: verify a real notification, Show Details, and deduplication separately against the pinned candidate; a LibVLC callback alone is not visual or accessibility verification. S01/S24 remain open until their respective complete roadmap gates, including conversion outcome handling, are satisfied.

No production source, package, runtime, receipt, or baseline was modified by this proposal.
