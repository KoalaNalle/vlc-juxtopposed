# Queue Find shortcut — 23 October 2026

The redesigned Queue now owns Ctrl+F when one of its items has mouse focus.
It opens the queue filter without also opening VLC's global Open Folder dialog.
The [checkpoint](checkpoint.json) qualifies this repair and selected ordinary
queue checks; the complete queue workflow and full beta remain open.

The investigation found correct actual queue focus. A toolbar input observer
delivered the same key through QApplication again, which ran global shortcut
matching after the focused queue had already reserved the key. The filter now
dispatches directly to its QQuickItem event handler, retaining consuming and
observing Keys handlers. The queue's narrow owner scope is also honored by both
native hotkey entry points. Other tabs, hidden queues and unclaimed shortcuts
retain their existing behavior.

Evidence:

- [Real QApplication/production-filter regression](filter-fixed-verification.json):
  eight cases, zero failures/errors/skips, normal exit0 and no Qt diagnostics.
  [Original filter](filter-original-verification.json) fails precisely the two
  extra-folder-action cases, exits with the expected QtTest failure status2,
  and has no test errors/skips/Qt diagnostics. No forced cleanup.
- [Native routing](routing-fixed-verification.json):17 cases pass, including real
  LibVLC key callbacks, both hotkey entry points, modal/sidebar/volume policies
  and inactive/hidden/outside-queue controls. Dependency adapters supply focus;
  this fixture does not qualify a full GUI lifetime. Its [negative control](routing-negative-verification.json)
  fails only the direct Player dispatch case.
- Earlier [proxy](component-proxy.json), [logical geometry](component-geometry.json)
  and [overlay](component-overlay.json) fixtures pass. All production QML in
  their R3 scope is unchanged here. Logical uiScale100/150/200 is not OS DPI.
- The [ordinary UI review](ordinary-review.json) covers mouse-focused Find,
  filtering Birch, Escape restoration, Title sorting, queue hide/show and
  Bookmarks' retained global Open Folder action. Cedar remains paused at0.
  The [process receipt](ordinary-process.json) confirms normal Ctrl+Q exit0,
  clean diagnostics and unchanged runtime/source inputs. Its original
  `passed=false` is retained because process success alone cannot accept UI.
- The [investigation review](investigation-review.json) retains failed ordinary
  revisions, the private observer compile failure and the corrected native
  test-counter mistake. Earlier component passes did not qualify failed UI.

Ordinary GUI SHA256 starts `903dc977`, Core `0404d1f6`, WASAPI `4e4e33c0`.
The complete hashes are in the checkpoint. R8 has2,072 [source identities](source-inputs.json)
and actual copies under `build-windows/beta-queue-workflow-20261022/source-r8`.
The GUI was built from source-r7; the [only R8 delta](test-only-source-delta.json)
excludes the separate Control modifier from the regression test's character-key
counter. Production is identical. R4 routing bodies/header and R3 QML were
checked against their original identities before reusing those bounded receipts.

Two ordinary drag attempts did not move Echo. A later [passive private trace](drag-review.json)
records an automated gesture ending after17ms while native drag startup is
still pending. No native drop event reaches the window. This explains that
observed attempt; a slower physical drag still needs checking. The private
observer [quit normally](drag-process.json) with no diagnostics and no input or
production-state mutation. No production drag code changed. The
[bounded event trace](drag-observation.json) preserves its timing and limits.
Removal/addition, detached queue, real bookmark
editing, accessibility, Windows DPI, broader media and final packaging still
need their own acceptance. The [fresh Figma reference](../../figma/20261023/queue/README.md)
has a complete screenshot/context; actual background blur is still missing.
No package, renderer-performance or whole-surface visual acceptance is implied.
