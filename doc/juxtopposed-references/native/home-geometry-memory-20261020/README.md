# Window memory progress — 20 October 2026

**Progress only. Geometry settings remain gated; the beta is unfinished.**

Private native tests exercise the actual Windows main window, production
serializer and fresh native restore. All observed launches were inspected on
monitor2 before their startup holds were released. No raw Computer Use screenshot
was saved. Generated fixtures and isolated settings remain in ignored
`build-windows/beta-geometry-memory-20261020`.

## Confirmed before behavior

`before-normal-maximized.json` records a successful normal-window control13,
then a cleanly reproduced maximized-state failure9: the normal client rectangle
`[-1880,397,1280,900]` was serialized as `[-1920,357,1920,1032]`.
`before-fullscreen.json` separately reproduces loss of normal bounds at fullscreen
save. Both failing app processes still exit normally with no Qt diagnostics or
native errors. These are desired-regression failures, not accepted results.
The full runner/fixture identities are retained in the receipts.

## Rejected first repair

`rejected-qpa-r1.json` and its compilation receipt preserve the native failure
after using Qt's platform normal geometry. It saves
`[-1880,405,1280,892]` for actual `[-1880,397,1280,900]`. VLC's custom Windows
non-client frame causes the8-pixel difference. The separate ordinary app
initial/cold-Maximize process and saved-geometry receipts reproduce the same
shift through real pointer Maximize, Ctrl+Q, cold reopen and pointer Restore.
Both ordinary sessions exit normally. This candidate is explicitly rejected.

## Rejected observed-geometry revision

The R2 candidate records observed logical normal client bounds and a guarded
screen pointer on the window, updating them through the production handler.
It keeps the original record during explicit fullscreen preparatory moves and
retains the existing serialized format and state flags. The actual ordinary
Windows system-menu Maximise route reproduces a transition-ordering failure:
geometry is reported before state, overwriting the record with enlarged bounds.
`rejected-system-maximize-r2.json` preserves the normal exit/no-error failure.

The fresh-window assertion has been refined to check saved state first and the
normal rectangle after returning to normal; a fullscreen current rectangle is
not expected to equal the normal rectangle.

## Verified normal-bounds scope on R3

[Current verification](verification.json) records six native processes/89 checks:
normal14, authored maximize14, native system maximize14, minimize14, production
fullscreen14 and hidden cross-monitor fullscreen19. The actual main window and
production serializer preserve exact observed normal bounds; a fresh hidden
native window restores the saved normal rectangle/state. Cross-monitor original
screen identity and bounds survive the selected-fullscreen move. All processes
exit normally, with zero Qt diagnostics/native errors and unchanged source,
runtime and generated media guards.

R3 defers handler Move/Resize/screen updates until Qt's state transition settles.
The review agent found no new concrete lifecycle defect: callbacks are context-
bound, restored normal bounds are seeded before nonnormal state, and disconnected
screens use guarded pointers. Equality avoids replacing identical records.

Three ordinary sessions separately exercise real Windows system-menu Maximise,
Ctrl+Q, cold maximized startup, pointer Restore, F11, cold fullscreen startup,
F11 normal return and Ctrl+Q. Independent saved-geometry readback retains exact
`[-1880,397,1280,900]` in all three sessions, with the correct max/fullscreen
flags. All exit normally without Qt/native errors. Ordinary GUI
`6de416fa3cae95ca7119d671ff68a1df31f22e3796685bf37db574b9db310f3a`;
private `7f7c229901fabd76bd323f3dd0a765eee6427ed1e313834283f3f9c0aafa0293`.
Frozen compiled inputs are in `source-after-r3` in the ignored work directory.

The first R3 launcher failed before app startup because copying the ignored
wrapper through the host's legacy text encoding corrupted its Unicode fixture
path. R3a selects the unique existing generated fixture by its actual filesystem
name; no production/compiled source changed and no failed launch is counted.

Still required: actual settings editor/failure/retry/reopen/Off-save/cold policy,
active media-state preservation, offscreen recovery and actual OS DPI. A narrow
lower-edge band was observed in the cold fullscreen screenshot: verify actual
startup client/frame geometry before whole-screen acceptance. Protected Core,
audio, FLAC and library modules remain unchanged. Physical unplug and Windows10
comparison retain separate gates. Both settings gates remain false; this scope
does not establish the complete geometry policy or final beta.
