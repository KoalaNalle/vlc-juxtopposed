# Native Performance model checkpoint — 17 October 2026

The real typed model compiles and passes all **16 native Qt cases**, with normal
exit, no failures/skips/forced cleanup and no temporary-directory cleanup warning.
The full Performance page is **not implemented or accepted** in this checkpoint.
Public text is UTF-8/LF; verification.json retains the raw native result hash.

Coverage includes opening/reloading without writes; disabled-consumer gates;
strict typed/bounded edits; Unicode configuration save/reopen; native module
choices and preservation of custom values; hardware Off preserving device choice;
postproc activation, quality, inline options, aliases, other-filter preservation
and duplicate handling; actual Windows file-lock save failure/rollback/retry;
exact live scheduling class readback and saved-policy restart; Windows/config
rollback on save rejection; real-time exact-grant-or-refusal behavior; CLI override
visibility; and a real full-inventory plugin rescan with busy/completion state.
Only this quiet owned test process changes scheduling and restores its initial
class in every case. No media workload runs under raised scheduling.

The opt-in recipe built current model/helper sources serially. Core is
0a2ad359f2e2d279f0efb78acd2ae10ff7484cfd288ca08f184f7358ff282e64.
The ordinary GUI and package were preserved. Both build-system registrations
include the model; the explicit Autotools test target resolves and three Meson
files parse. A native Meson build and ordinary GUI rebuild are not claimed.
The first registration-only Meson syntax check failed and remains in ignored
model-registration-r1.log; corrected registration-r2 passes. Native tested
model sources did not change during that repair.

See [results.txt](results.txt), [verification.json](verification.json),
[source-inputs.json](source-inputs.json), [manifest.json](manifest.json) and
[reproduction guide](../../../../test/libvlc/native_performance_preferences.md).
The real-time test accepts either an exact grant or a reported refusal preserving
OS/config state; this checkpoint does not establish a granted real-time path.

Work is paused at the user's request. Next: finish native decoder/frame/clock
consumer checks, then mount all 14 Performance fields/actions in MainCtx/QML,
current GUI/native-window verification, Recording/Extensions and remaining beta
gates. Consumer flags default false; a test's enabled controller mapping does
not accept the downstream media behavior. All five unrelated untracked files
remain untouched. Nothing is pushed or published.
