# Native subtitle preset workflow — 17 October 2026

The actual paused-player preset workflow now passes on this Windows10 guest.
This is scoped behavior and regression evidence, not full subtitle-surface or
beta acceptance. The containing Git commit is the source revision; the base was
092f212ca9efc177ce1cbc593d9806c9f5f673b6 with the recorded frozen patch applied.
All development data and outputs stay on E:, one heavy workload at a time.

## Fixed behavior

A temporary settings-file lock left the original long-lived QSettings object
in its retained first-error state, so Save could not recover after unlocking.
SubtitlePresetModel now uses the existing CheckedPreferencesStore fresh checked
sessions, including rollback/readback. Native failure keeps the prior records,
selection and bytes; retry succeeds without replacing the service or duplicating
records. Save errors scroll into view through the existing page scroller without
stealing focus or changing original geometry.

Applying a preset replaces its menu entries. That could destroy the triggering
item before automatic menu closure. The menu now captures the stable ID and
closes before applying; New Custom Preset also closes its menu explicitly.

The native window's preferred-hotkey filter intercepted Escape from a clicked
preset button because mouse focus does not set visualFocus. The actual popup
stayed opened while the main window shrank from1280x900 to320x360. Focused
controls inside a visible QQuickPopupItem now receive press/release delivery
before playback hotkeys. The existing outside-popup visual-focus policy remains.
The pinned Qt6.8.3 popup type is checked in the actual integration; rerun this
regression when changing Qt versions.

## Accepted checks

- Three actual native processes pass **136/37/24 checks (197 total)**: edit,
  restart and Default after another restart. Counts include repeated pointer,
  window and viewport assertions, not197 distinct features. They use the real
  MainCtx player, subtitle service, compiled popup and portable settings.
- The UI exercises Unicode Save, empty-name disabling, cancellation/focus,
  duplicate rejection, edit/Custom settings, saved apply, real locked-INI
  failure/visible error/retry, stable selection, restored375ms timing and red
  appearance, and Default persistence. Four stable native/UI pause samples
  precede the baseline. Input identity, rate and both clocks remain unchanged.
- With real mouse focus (visualFocus=false), Space opens the submenu without
  playing the primary input. One Escape closes that menu; another closes the
  actual settings Popup.visible/opened state. All three processes keep the
  main window1280x900 and return focus to the original Settings button.
- **38** current native subtitle cases pass, including the new real no-delete
  file-lock rollback and retry on the same service/QSettings object. **10**
  standalone preset cases pass, including actual Freetype pixels, restoration,
  track roles, timing, reset and same-object recovery from blocked storage.
  **Six** exact-production key-routing cases preserve outside-popup mouse
  hotkeys, modal press/release/direct-dispatch guards and Qt navigation policy.
  Their adapted unrelated dependencies do not claim a full MainCtx window;
  the actual popup branch is proven by the three UI processes above.
- Both affected supplied-state/offscreen component scripts pass. **Five**
  ordinary cases pass: redesigned/traditional controls, Library, local HLS and
  bounded read-only Downloads playback. A separate scan finds zero selected
  QML/assertion diagnostics in all five logs. Downloads size/mtime are unchanged;
  no full file-content hash is claimed. Personal filenames/logs/frames stay private.
- Every accepted process exits0 without timeout/forced cleanup. The actual UI
  has zero selected QML diagnostics and no owned fault dumps. All1910 ordinary
  and five private frozen inputs match. Ordinary source/package hashes remain
  unchanged; QA code is confined to the private DLL. Original artwork/fonts
  are unchanged. Meson syntax passes; no native Meson build is claimed.

Core is bf29403169b936e2156bcfcd0f03a38fbbaad0d9b54fd0e7d9e843e4bdb5bc33;
ordinary GUI is eb75324f988cdde1974275f20df87cce8ff22ed3a5d3b8f9c6c9d9f7632a0641.
Full binary identities, named cases, capture hashes and popup/clock comparisons
are in checkpoint.json and results/. GUI captures show owned controls; native
subtitle pixel captures are separately labelled and use the VMEM renderer.

## Rejected evidence and reproduction

R1 is the real retry negative. The component menu negative records visible/
opened=true after selection. R2 throughR5 remain failed UI attempts; early clock
and child-visibility checks lacked sufficient diagnostics. R4 proves a queued
UI pause update while the native clock is fixed; an early separate-window
hypothesis was later disproved. R6 reads the actual popup flags and establishes
the independent native Escape leak. R7 is the accepted complete replay. A first
standalone attempt used retained white text where its assertions require green;
it remains a fixture failure. Fresh black video/green primary SRT and secondary
text meet the explicit pixel contract. A routing runner initially expected five
rather than the source's six cases; the corrected replay verifies all exact names.

Use Common.ps1 and the configured E:-backed VLC-Juxtopposed-Travel environment.
The maintained runner is modules/gui/qt/tests/native_ui/run_subtitle_presets_journey.py;
its default modes are edit, reopen, default-reopen. Give it an ordinary runtime,
matching private DLL and fresh E: output. It stages immutable hard links and an
owned portable profile. Never overwrite a linked binary or reuse failure output.

reproduction/ contains the actual incremental recipes. build-r2.sh built the
unchanged native preset/subtitle executables; build-r4.sh built the final GUI,
exact key-routing executable and private UI. For a fresh native rebuild, the
configured Qt Makefile targets are test_qt_subtitles_preferences.exe and
 test_qt_subtitle_presets.exe, with -j1; stage their .libs executables into fresh
owned output. Earlier recipes retain historical output/manifests and must be
adapted to fresh outputs/current frozen inputs, not executed as a chronology
against later source. The Linux /mnt/c source alias resolves internally to E:.
These are guest incremental receipts, not a clean portable-release recipe.
Existing cross-host wrapper/static-link diagnostic warnings remain in raw logs.

## Remaining gates

Playing-state UI interaction, rapid media changes through this popup, full
accessibility/screen-reader behavior, Light/System/OS notification variants,
150/200% Windows DPI, monitor movement and independent fidelity remain open.
The existing naming form is explicitly provisional because no original form
was shown. Other confirmed subtitle/library/player surfaces, final Windows11
coverage and a clean licensed matched beta package remain in the beta plan.
