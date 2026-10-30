# Existing subtitle regression client on current Core — 29 October 2026

Eight selected processes pass, with actual Core `4a2865915e34`, current SRT/ASS,
WebVTT38 and TTML9d modules staged from ordinary R18. Exact identities, generated
fixture hashes, commands, selected functions, snapshots and normal child exits
are recorded. All input/runtime/fallback guards pass. Each process emits one
known native test-window-close teardown diagnostic and no other captured errors.
Raw stderr remains ignored locally; the scoped classification is included here.

Cases cover category-relative changes/Reset notifications; large own and inherited
secondary delays with held-frame/resume checks; held and first-playing FPS changes
preserving an embedded secondary caption; opposing external delays where the
secondary has ended; estimator directions and cancellation; and cancellation
without publishing an adjustment. No function was skipped.

This reuses regression executable `64846efd9fab` from `test-r23`, with its exact
test source `80838e7b8198`. The existing compiled PlayerController client is not
a fresh ordinary Qt build or a full current controller-source-closure audit.
Ordinary Qt7c7 is staged but this client owns its compiled controller. These new
results qualify this identified client interacting with current Core/codecs;
they do not inherit the older CoreF8 results, establish current ordinary GUI
role behavior, or close beta/design/DPI/package gates. The regular R18 GUI's
single-track tick/FPS/Reset evidence is recorded separately. The next check is
the visible ordinary multi-track journey.

Only generated media/captures and scoped reports are included. The executable,
runtime, profiles and raw logs stay ignored. The frozen producer requires fresh
outputs and revalidated inventories before another run. Do not package this
private regression client in an ordinary release.
