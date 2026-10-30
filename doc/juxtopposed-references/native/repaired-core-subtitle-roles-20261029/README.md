# Repaired-Core subtitle roles and estimators — 29 October 2026

Ten qualified selected processes use Coreccb6/current SRT/ASS/WebVTT38/TTML9d.
R2's first eight processes pass category-relative adjustment/Reset callbacks,
own/inherited large secondary delays and resume, held/playing FPS preservation
of embedded secondary captions, opposing per-track delay retiming, estimator
directions and cancellation without publishing a late adjustment.

R2's ninth process failed all 24 rows at the initial requirement for production
QML pages: the runner omitted VLC_TRACKS_SOURCE_DIR. That failed result is
preserved; it does not qualify estimator speed behavior or a product failure.
The context-cancellation selector was not reached in R2.

Fresh R3 supplies two frozen production TracksPage QML files and full two-track
initialization rather than the FPS-only fresh setup. The same identified native
client passes all 24 estimator rows (audio/primary/secondary, 0.5/1/1.5/2 rates,
both timing directions) and all 16 context-cancellation rows (Qt/native seeks,
rate changes, pause/resume, subtitle/audio selection changes). Exact data-row
counts and executed functions are checked; none is skipped. The handlers and
connection code are extracted from the frozen production pages and executed
against the actual compiled controller/native player, not a whole ordinary UI.

All ten qualified children/observers exit 0 and all source/input/runtime/fallback
guards pass. Each emits one known native test-window-close teardown diagnostic,
with no other captured errors. Raw logs, test binary and profiles stay ignored.

Executable 64846efd9fab is the previously identified compiled regression client
with exact test source 80838e7b8198. It owns its compiled controller; these are
new Coreccb6 interaction results, not a fresh full controller-source rebuild or
ordinary page/design/DPI/package acceptance. Ordinary Qt7c7 preset/held-picture
proof is recorded separately in the preset-retry-held-frame receipt. Preserve
the earlier Core4a evidence and R2 setup failure as distinct scopes.
