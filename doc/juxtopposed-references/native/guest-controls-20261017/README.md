# Controls implementation — Windows 10 guest, 17 October 2026

Work in progress toward the full beta. The source implements all sixteen
authored Controls rows from the [original reference](../../figma/20261017/controls/README.md).
The complete Controls surface, desktop/DPI review and final package are not yet
accepted. This is a local implementation checkpoint, unpublished.

The shared settings host mounts the real Controls controller. Native key defaults,
scoped edit/reset, four skip amounts, distinct single/double video actions,
physical wheel/reverse and shared or independent remote wheel/reverse have checked
persistence. Opening the page does not save. Rejected writes leave native values
unchanged and permit an explicit retry. Shortcut/wheel mappings and HTTP listener
changes clearly require restart; click/skip and independent remote wheel edits
affect new gestures after a successful save. Unavailable consumers and custom saved enums
stay explicit.

The user's requested **Use the same wheel settings** toggle adds one row to the
sixteen authored rows and defaults ON. ON aliases the three Mouse/Remotes wheel
preferences in both directions. Shared edits require restart; HTTP consumes the
same active startup wheel snapshot until then. OFF immediately restores the
saved independent remote policy. Linking and shared edits preserve those saved
independent values, including through repeated toggles and process restart.
Rejected writes change neither saved preferences nor the active link flag. New web
remotes require a masked password setup and default to localhost. Enabling a
configured listener outside localhost requires configured HTTPS credentials.
The controller reports its own service state without probing another listener
with credentials. Disabling preserves unrelated interface chains and applies on
restart. The native default port is displayed instead of presenting the empty
reference illustration as a valid saved port. All eighteen Controls SVGs retain
their original complete bytes and 24px intrinsic dimensions.

Video clicks use the system double-click interval and drag threshold on Windows.
Pending single clicks are cancelled by double clicks, drags, lost grabs, context
menus, media changes and modal transitions. One Qt guard composes QML, widget
and popup ownership. It also observes Windows native common-dialog owner
disabling; an owned-window negative control fails that case before the observer
and all fourteen widget/dialog cases pass afterward. The main player delegates to one
policy; separate embedded LibVLC players retain their legacy fallback. Mini-player
mouse gestures reach the same native consumer, while touch retains its existing
control-visibility action. Owned pointer testing caught and repaired a disabled
tap handler sending a release instead of cancellation.

The HTTP wheel command is authenticated, axis-specific and bounded. The browser
script batches high-resolution deltas and leaves form/slider gestures and zoom
modifiers alone. Lua now has public controller getters that prefer a known
pending seek target: paused finite-media controls no longer remain at the old
clock point until resume. Ordinary output-clock getters retain their semantics.
Interface-chain startup now preserves quoted colons and option blocks.

## Accepted focused evidence so far

All outputs below are owned local fixtures under
`build-windows/beta-controls-20261017`; they are absent from distribution.

| Evidence | Result | Scope |
| --- | --- | --- |
| `native-input-r13/verification.json` | 89 assertions, seven normal positive processes | Real core/hotkeys/window input, cancellation, wheel mappings and separate embedding |
| Retained `native-input-r12` old-core controls | Two expected failures, normal termination with code 1 | Quoted interface chain; double click fails after successful consumer setup |
| `linked-acceptance-r1/model/verification.json` | 19 Qt cases, no failures/skips/forced cleanup | Actual controller, checked persistence, link/unlink, rollback/retry, scoped bindings/reset, credentials and service state |
| `dialogs-acceptance-r3/results/verification.json` | 14/0/0/0 | Real shortcut editor, masked setup, cancel/save/conflicts, failure/retry, ownership/teardown and native disabled-owner blocking |
| `native-http-r9/verification.json` | 29 checks, four normal processes | Actual authenticated loopback Lua service, default linking, preserved independent/reversed wheel, pending restart, paused seek/resume, exact served JS, 401/403, busy port and retry |
| `qml-controls-r5/verification.json` | Eight groups | Actual host/page with an explicit supplied controller seam, default-ON link/repeated toggles, search/errors, layouts and original asset hashes |
| `privacy-shared-fields-r2/verification.json` | Eight groups | Actual Privacy host/page regression after extracting shared field components |
| `qml-video-input-r4/verification.json` | Five groups | Actual mini-player helper with an owned provider seam; tap/double/drag/cancel/touch |
| `http-ui-js-r1/verification.json` | Four groups | Complete actual UI script parsed/executed in QJSEngine with an explicit owned DOM seam |

The HTTP replay independently confirms the resumed output clock after paused
seeking. The pre-observer native owner test terminates normally with code 1 and
exactly one expected failure; its raw generic runner report remains failed.
Other failed intermediate diagnostics and fixture builds are retained locally
and do not count as accepted results.

Current core SHA-256 is
`218708c12b18fa9e69b9242fd0c0ea38c4ad4f61bf6c9b0798bd7cd8cf91d682`;
hotkeys is `0e397625c303e0a53bbf3e212542def17f10a5ec31471740fba77ccab8d54b8a`;
Lua is `0e52a7072c1e86ed64981281823ed6b1e1739aca0922da54c842db6e16a12d90`.
The ordinary GUI is
`7fb8d8251d40e1265b4443cab5adcaeb2a759b02dee647d62a8fa506562a455d`;
the widget executable is
`74436f0d1e8d910b1d41535195bd2769af36802fce375ab5e087bbeb844068f5`.
Exact controller, driver, script and module hashes are in the copied receipts.

## Build and remaining gates

Both production build systems register the controller/editor, shared fields,
Controls page, mini-player helper and original assets. The maintained
[native recipes and runners](../../../../modules/gui/qt/tests/native_library/README.md)
describe isolated reproduction. The ordinary GUI/resources build completed with
one job and 2,001 unchanged selected production/build/fixture input hashes.
Meson syntax parsed; a native Meson build is not claimed. The new ordinary
runtime passes redesigned/traditional video/audio/sidecars, Library ON and
local HLS: four normal exits, zero QML/file-write/visual-callback/PCR errors.
Current-tuple Library18/42/9, Subtitle37 and player controller4/25/8 pass with
normal exits, no failures/errors/skips and no forced cleanup. The copied
[verification receipt](verification.json) records exact identities and scopes.
Component captures show [Dark](controls-default-dark.png), the requested
[default-ON link](controls-linked-remotes-dark.png), and
[narrow Light](controls-light-narrow.png). These are owned QML seams.

The QML and JavaScript seams do not establish actual desktop/browser interaction,
Windows DPI behavior or an independent visual score. Real widget ownership is
tested separately; native full-page interaction, HTTPS/protocol coverage,
unseekable/unknown-duration remote behavior, sustained performance and final
Windows 11 acceptance remain open. The original ordinary package and all prior
accepted runtimes remain preserved. WSL uses CPU compilation only, with its GPU
path disabled, 4 GiB/two CPUs and E:-backed swap. This is not a complete clean
licensed beta package.
