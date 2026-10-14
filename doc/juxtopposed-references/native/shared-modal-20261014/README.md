# Native shared-modal key routing — 14 October 2026

The same frozen native harness was compiled against the preserved unguarded
production extraction and the guarded extraction. The unguarded build fails
exactly at the visible-modal Qt key-delivery gate: zero Space/Up/Down presses
reach the Qt receiver where three are required. The guarded build passes all
six ordered cases, exits zero and prints the expected production identities.

| Variant | Actual native result | Evidence |
| --- | --- | --- |
| Before guards | 5 passed, 1 failed; exit 1 | `results/before-results.txt` and `results/before-verification.json` |
| After guards | 6 passed, 0 failed, 0 skipped, 0 blacklisted; exit 0 | `results/after-results.txt` and `results/after-verification.json` |

The cases cover real native direct dispatch without a provider, mouse-focus
routing outside a modal, Qt press/release delivery and blocked native direct
senders while a modal is visible, restoration after hiding it, Tab/Backtab and
preferHotkeys navigation policy, native initialization and cleanup. The helper
requires all six PASS names in order, exact 6/0/0/0 totals, no rejected status,
exit zero, matching printed source identities and generated-include hashes.

The source methods `InterfaceWindowHandler::applyKeyEvent`, `MainCtx::sendHotkey`
and `MainCtx::sendVLCHotkey`, plus the key-only event-filter branch, are extracted
from production without duplicating their routing policy. Unrelated class
members are dependency adapters. The fixture uses QCoreApplication, detached
QQuickItem metadata/key receivers, synchronous Qt events and actual LibVLC
key-pressed callbacks. It creates no window, controls, focus scene or rendering.
It does not establish complete MainCtx/provider/window ownership or lifetime.

`build/before-build.log` and `build/final-build.log` show actual MOC, CXX and
link steps for both variants. The build owner reported exit zero, copied each
result executable, and ran the matching helper. Libtool host-path diagnostics
followed by Continuing, plus clock-skew warnings, are retained. The subsequent
native runs establish execution separately. Harness identity comes from the
frozen source/build-owner record; production identities are printed by the
compiled extraction and checked at runtime. A source hash alone is not build
proof. Automake was built; Meson registration exists but no native Meson
configure/build is claimed here.

`source-runtime-manifest.json` records source, extraction, executable and core
hashes. `evidence-index.json` pairs the SHA-256 of each original ignored local
file with its separately hashed publishable copy. Paths use /checkout and
/build/vlc labels; recorded identities are retained. Generated before/after
extractions are copied byte for byte. No executables, DLLs, userdata or media
are copied into this proof bundle.

History preserves the initial compile/link issues and bounded timeout. The
callback API returns void; the fixture now proves real callback registration
by triggering and observing it. Registering the existing wheel-converter MOC
closed the link gap. The first QApplication run timed out after 30 seconds with
no results or output; that alone does not prove its cause. Reading actual Qt
source established that the detached fixture supports QCore-only setup. The
final fixture also keeps normal QKeyEvent accepted entry state, satisfying
Qt Quick's delivery precondition without changing any six-case routing/count
assertion. Passive startup markers remain in both final native runs.

The related native conversion backend passed all ten cases against the mounted
package with its exact core/module audit. The supplied-state offscreen production
QML component also passed with zero messages. Their sanitized verification
copies are under `related/`; neither is desktop picker/primary-transport proof.
Actual packaged modal controls, keys during Encoding/Cancelling, primary playback,
queue/drop behavior and full lifecycle acceptance remain separate. The conversion
form is a shared-style functional adaptation: no original Figma conversion form
was found, and broader video/format/profile or original-form fidelity acceptance
is not claimed.

## Reproduction

Use an already configured MinGW x64 Qt build and the same frozen harness bytes.
Build `test_qt_shared_modal_hotkeys.exe` in the configured Qt build directory.
The default target generates the current production extraction and required MOC.
For the negative baseline, override only the target's CPPFLAGS to set
VLC_SHARED_MODAL_PRODUCTION_INCLUDE to the absolute preserved before .inc file;
force recompilation of the harness, copy the baseline executable, then restore
default target flags and force the same harness to recompile for the final
executable. `build/executed-before-after-build.sh.txt` preserves the actual
owner-run sequence with generic path labels. Adapt paths; it is evidence text,
not an automatically executed setup script. Do not reuse a stale object or
modify production sources/assertions to construct the baseline.

From the checkout root in Windows PowerShell, with the exact package and both
executables already staged, use fresh output directories:

```powershell
$modalCheckout = (Resolve-Path .).Path
python "$modalCheckout\test\libvlc\native_shared_modal_hotkeys.py" `
  --package "$modalCheckout\build-windows\slice-conversion-modal-package-20261014" `
  --test-exe "$modalCheckout\build-windows\core-tests\test_qt_shared_modal_hotkeys_baseline.exe" `
  --production-include "$modalCheckout\doc\juxtopposed-references\native\shared-modal-20261014\extraction\before\shared_modal_hotkey_production.inc" `
  --output-dir "$modalCheckout\build-windows\shared-modal-baseline-FRESH"
python "$modalCheckout\test\libvlc\native_shared_modal_hotkeys.py" `
  --package "$modalCheckout\build-windows\slice-conversion-modal-package-20261014" `
  --test-exe "$modalCheckout\build-windows\core-tests\test_qt_shared_modal_hotkeys.exe" `
  --production-include "$modalCheckout\build-windows\core-tests\shared_modal_hotkey_production.inc" `
  --output-dir "$modalCheckout\build-windows\shared-modal-guarded-FRESH"
```

The baseline helper must exit nonzero with the concrete modal-delivery failure;
this is intentional negative evidence. The guarded helper must pass its exact
six-case gate. Each run uses a fresh userdata directory, exclusive selected
VLC_LIB_PATH, removes inherited VLC_PLUGIN_PATH and has an owned 30-second bound.
A new source/runtime identity creates new evidence, not an extension of this
frozen run. Actual packaged GUI and original-design verification still require
their own checks.
