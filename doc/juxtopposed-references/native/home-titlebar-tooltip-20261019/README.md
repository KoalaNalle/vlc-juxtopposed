# Title-strip tooltip placement — 19 October 2026

**R7 native bounds/single-help checks and ordinary maximized help replay pass.**
The copied-fixture cropping/coordinate observation has a scoped comparison below.
Whole-beta and full geometry/visual/DPI/package acceptance remain open.

## Actual failure

[Before R5](before-r5.json) uses unchanged R4 production inputs and a private
observer. Keyboard focus opens the actual Close tooltip without clicking Close.
The maximized monitor-2 window's available bounds are
`[-1920, 357, 1920, 1032]`; the native tooltip appears at `[0, 329, 44, 32]`.
Its complete width lies beyond the right edge, and its top is28 pixels above the
available screen. The app exits normally with no forced cleanup, Qt diagnostics
or native errors. The geometry assertion fails as intended; this is not an
accepted placement run. No raw Computer Use screenshot was saved.

## Repair and validation boundary

Each original title-strip button now owns a `Popup.Item` tooltip below the
button, constrained by the application window margins. The attached native
shared popup is replaced only for these buttons. Existing artwork, dimensions,
navigation, accessible button names and window actions remain unchanged.

[Title-strip component](component-r6.json) retains12 normal/narrow Home/player
geometry cases at100/150/200% component scale, original artwork checks and
focus/controller seams. No Qt messages are recorded. This is not Windows-DPI
or native popup acceptance.

The private [R6 bounds observer](bounds-r6.json) passes34 checks normally, with
no Qt diagnostics/native errors. It resolves the new local popup or old attached popup
and retains the same complete-global-bounds assertion. It also adds keyboard
and pointer activation in restored1000- and640-pixel windows positioned near
the monitor edge. Its [generated-only capture](duplicate-help-r6.png) nevertheless shows simultaneous Home
keyboard help and Close pointer help. Bounds success did not accept that
interaction. R7 gives hover priority without taking keyboard focus; keyboard
help returns when hover leaves the strip.

The same desired [component test](component-r7.json) fails on recovered R6 source (its hash matches
the frozen manifest), reporting both tooltip names, and passes on R7. The first
probe attempt used an unsupported QQuickView `rect()` helper and is retained as
a fixture error; the corrected paired test is the negative control. The later
R7 native/ordinary results below retain their distinct scope.
Source snapshots are under `build-windows/beta-settings-activation-20261019/source-r5`,
`source-r6` and `source-r7`. No new dependency, security-setting change or publication.

## Accepted R7 scope and remaining observations

[Actual R7](actual-app-r7.json) passes39 checks with normal exit, no forced
cleanup, no Qt diagnostics/native errors/warnings and unchanged source, ordinary
GUI and generated-media identities. The maximized tooltip is `[-46,395,40,28]`,
inside monitor2. Four restored1000/640 keyboard/pointer cases each retain one
visible title-strip tooltip. The [generated-only capture](single-help-r7.png)
shows Close help without simultaneous Home help.

[Compilation R7](compilation-r7.json) binds ordinary GUI
`9892032f4456e38b831884aa72c9bbdcbeeb75113d670abe804de52e4adaabbf`
and private GUI
`a14c5d7c7cb429c0fe901795ce4d6c36eb1113acb2cb5be68e00131405080df6`.
Only production title-strip QML changes from the R4 tuple; repaired FLAC modules
and protected Core/audio/library binaries remain unchanged. Ordinary runtime:
`build-windows/beta-settings-activation-20261019/ordinary-runtime-r7`.

[Direct ordinary Windows replay](ordinary-windows-r7.json) confirms complete
Close help after accessible Maximize, Home-hover priority while Close retains its
keyboard ring, and return of Close help when hover leaves. Ctrl+P/Escape and
normal Ctrl+Q work; visible paused time remains00:00. No Qt diagnostics/native
errors or security prompts are observed. This does not establish screen-reader
focus or exact native-clock invariants.

The initial copied startup geometry shows a cropped right edge. A coordinate
click intended for Restore enters fullscreen; F11 exits normally and accessible
Maximize produces a complete window. Preserve both observations for actual
geometry/CSD/capture-origin investigation; their cause is not assigned or
accepted by that first replay. The first R7 launcher attempt also stopped before app startup after a
copied geometry path referenced a nonexistent fixture. Corrected R7a uses the
same binaries and the preserved geometry source. That launcher failure is not
an app failure. Earlier R6 bounds success does not override its duplicate-help
finding; component/native/ordinary scopes remain separate.

[Owned geometry readback](owned-geometry-comparison.json) identifies the copied
fixture as a normal1920-pixel-wide client at x=-1901 on a monitor spanning
x=-1920..0: its right edge is19 pixels outside the screen. The post-Maximize
record starts at x=-1920 and records the maximized state. Source fixtures remain
unchanged.

[Fresh-geometry R7b comparison](geometry-comparison-r7b.json) uses the same
ordinary GUI and starts with a complete window from the latter record. Direct
coordinate Restore and Maximize perform their expected native actions, with
the corresponding accessible action labels and no unintended fullscreen. It
exits normally without Qt diagnostics/native errors; generated source, runtime
and geometry source remain unchanged. This supports attributing the earlier
cropping to its off-screen owned fixture; it does not prove every restoration,
multi-monitor or real Windows-DPI case. Use the preserved in-bounds fixture for
future ordinary replay rather than silently adjusting a clipped screenshot.
