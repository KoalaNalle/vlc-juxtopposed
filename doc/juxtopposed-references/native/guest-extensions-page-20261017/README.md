# Original Extensions settings — 17 October 2026

The confirmed seven fields/actions now use the real MainCtx-owned native add-on
model. This accepts the scoped Windows 10 Extensions workflow, not the full beta.
The implementation belongs to the same Git commit as this record; the base was
32cd62fc4d71b0b59d6a2e79c7ea72f3a6234112 with the recorded frozen changes applied.
Resolve the containing commit for the committed source revision.

## Behavior

| Original field/action | Native behavior |
| --- | --- |
| Add-ons list | Installed/catalog/manual entries, native state and stable selected identity; sorted/filtered keyboard movement cannot redirect actions to another entry. |
| Search | Local native-model filtering; hidden selection clears, Backspace and Escape restore the list without closing settings. |
| Find more extensions | Loads VLC's production catalog into this list, with busy/error feedback. |
| Add | Installs a selected available catalog item; otherwise opens the real local .vlp/.VLP chooser. Cancellation performs no install; Unicode paths work. |
| Remove | Removes only manageable installed entries. Locked files preserve bytes and allow retry. A failed catalog save exposes a separate retry without repeating file work. |
| Load add-ons on startup | Existing qt-autoload-extensions preference with checked save/rollback. Next start loads descriptors/menus, not every script's activation. CLI override and currently active extensions retain native semantics. |
| Advanced Options | Opens the real native expert preferences dialog. The existing Plugins window shares the application's add-on model. |

The original title, spacing and 17 SVG bytes come from the retained Juxtopposed
reference, node 6575:17137, CC BY 4.0. Sixteen visible SVG slots render in the
actual native empty page. No new design request or third-party installation was
needed for this checkpoint. Broader fidelity scores remain unassigned.

## Validation

- Five actual native processes pass **190 checks**: empty 16, edit 121, reopen
  21, startup-on 16, override-off 16. Counts include repeated target/window
  assertions, not 190 distinct features. All exit normally with no selected QML
  diagnostics or fault dumps. Installed package bytes, real file/config/catalog
  locks, retry, stable selection, Lua activation and active-extension preservation
  are exercised. The ordinary GUI remains unchanged by private instrumentation.
- A separate actual Find more run passes **17 checks**, loads 115 list rows
  including the two owned manual fixtures, and exits normally. It fetches
  metadata only and installs no third-party package.
- The current core/storage/repository tuple passes **49 native cases / 948
  assertions**, including 20 pending-operation shutdowns, malformed input,
  collision protection, selected replacement bytes, publication failure and retry.
- The ordinary GUI passes **five** bounded cases: redesigned/traditional
  controls, Library, local live HLS and read-only Downloads playback. All exit
  normally. A separate log review finds zero selected QML/assertion diagnostics
  in all five. Source runtime/package identities remain unchanged. Personal
  filenames, raw media logs and frames remain under ignored E: build output.
- Actual Qt source component loading passes two checks on the unchanged page.
  Both build-system registrations are present; no native Meson build is claimed.
- All **1,910 ordinary / five private frozen inputs** match. The ordinary
  inventory includes a historic build helper; it is not 1,910 tracked producers.
  Patch whitespace checks pass. Existing cross-linker/private diagnostic warnings
  remain in the build logs; these are incremental, not clean-release builds.

The tests exposed a real Windows caption-filter lifetime defect: a native window
could outlive MainCtx and its button model. The filter is now owned by the GUI
handler and guards borrowed QObject references. Every default journey verifies
three live caption hit tests and sends an owned WM_NCHITTEST after MainCtx
is destroyed; all five processes still exit normally. The earlier access
violation remains preserved as rejected evidence. This defect is not established
as the cause of the user's separately reported security trip.

## Reproduce and inspect

Use E:\Modding\vlc-juxtopposed and the migration kit's Common.ps1 environment.
The existing E:-backed VLC-Juxtopposed-Travel configured build supplies Qt 6.8.3,
its generated Makefile, tools and contrib. Use one worker and one workload at a
time. reproduction/ contains the exact ordinary/private build and ordinary smoke
scripts used here. They describe this guest's configured incremental build, not
a portable clean-build recipe. ordinary-inputs.json and private-inputs.json
record the tested file hashes; checkpoint.json records the binary identities.

The maintained native runner is
modules/gui/qt/tests/native_ui/run_extensions_journey.py. Give it the ordinary
page-runtime-r4 package, private page-native-ui-r10/.libs/libqt_plugin.dll and a
fresh output directory. Its default five modes exercise persistence in order;
--modes live is a separate metadata-only catalog run. The native regression is
test/libvlc/native_addons.py with events-native-r4/.libs/test_addons.exe and the
current ordinary runtime. Raw evidence is in
build-windows/beta-extensions-20261017/{window-acceptance-r10,
window-live-acceptance-r10,page-events-acceptance-r1,ordinary-page-acceptance-r1}.
Never overwrite binaries while a staged runtime hard-links them.

R8's incorrect QDialog-only inspection and R9's rejected protected close call
remain failed private-fixture evidence. The original historical checkpoint and
[security-trip log](../../../../SECURITY-TRIP.md) remain intact.

## Remaining gates

Dark/Light/System, accents, full accessibility/focus, short/narrow windows,
150/200% Windows scaling, monitor movement, Windows shell-native chooser,
independent fidelity review, final Windows 11 results and a clean licensed beta
package remain open. The rest of the confirmed redesign remains governed by the
beta plan; this checkpoint does not expand beta into vanilla feature parity.
