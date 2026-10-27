# Subtitle roles and signed estimator — 25 October 2026 night

This is a scoped checkpoint during the continuing beta session. It is not beta
readiness, full design acceptance or a clean distributable package.

Incrementally adding a subtitle alongside an already rendered track previously
left both player tracks labelled primary, although the renderer assigned the
second channel correctly. The player now publishes the actual output role before
its observers. An unchanged desired native regression fails the old Core and
passes the repaired Core. Ordinary English/Finnish captions confirm independent
secondary timing and Reset, including explicit zero versus category inheritance.

The synchronization estimator also discarded negative corrections because its
time-point subtraction rejects negative durations. The local estimator now
constructs the signed duration explicitly, preserving the shared time-point
contract. The desired test fails the old implementation; all fourteen final
native functions pass, including both signs, cancellation and Reset.

The regular player visibly applies -3601 ms to the primary caption, restores only
the secondary caption with its own Reset, and independently applies +4004 ms to
the secondary. A playing estimate applies -4213 ms while the clock continues.
Direct Quit while playing exits normally. Source, binary, fixture and plugin-cache
guards pass; the final GUI has one 602-module inventory and no error diagnostics.

Earlier ordinary runs cover fractional FPS, legacy/redesigned Reset, playing EOF
revival, seeking, next source and cold saved-default restart. Their exact older
tuples remain recorded separately. The latest Core retains the full native FPS
checks. No nondefault-rate/acoustic/source-change estimator claim is made.

[checkpoint.json](checkpoint.json) binds exact sources, binaries, ordered results,
before failures and fixture-assertion corrections. Gzip preserves original bytes.
Native/desktop evidence remains distinct from final visual, DPI, hardware and
package gates. Continue from [STATUS](../../../juxtopposed/STATUS.md).
