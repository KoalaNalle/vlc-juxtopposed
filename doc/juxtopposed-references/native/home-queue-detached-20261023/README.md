# Detached Queue placement and lifetime — 23 October 2026

The detached Queue stays reachable when the player is maximized. It first
tries an adjacent display, then fits its complete native frame within an
available work area. A deferred check after showing the window also keeps its
title bar visible after Close/reopen. Three topbar bindings tolerate detached
Loader teardown without reading a removed item.

The [checkpoint](checkpoint.json) qualifies this bounded repair. It does not
accept the complete queue workflow, all monitor/DPI states or the full beta.

- [Native geometry policy](policy-verification.json):15 cases, no failures,
  errors, skips or diagnostics; normal exit0. Supplied rectangles cover side
  space, maximized owner, taskbar offsets, negative origins, adjacent displays,
  small areas and logical scaling. These are not physical DPI tests.
- [Exact topbar-binding fixture](lifetime-after.json):18 supplied-state checks
  pass in a real Qt engine. The [original bindings](lifetime-before.json) fail
  exactly five cases involving a null Ready item or absent Loader. This checks
  binding behavior, separately from native window teardown.
- [Final ordinary review](ordinary-review.json):the maximized2560×1392 player
  retains Cedar paused at0. Its detached queue opens completely on the left
  display, with a402×1033 captured frame at(-411,357). Closing only that window
  and reopening reproduces the complete frame and identical bounds. Mouse
  selection, Ctrl+F, Birch filtering and Escape work after reopening.
- Redocking removes the floating window; explicit Play queue activation returns
  all five original rows. Find still works after row focus returns. Redocking
  currently hides the panel, so seamless visibility retention remains open.
- The [final process receipt](ordinary-r13-process.json) confirms normal Quit0,
  no runtime diagnostics, and unchanged input/runtime identities. Its original
  `passed=false`/`observationsRequired=true` is retained; the separate UI review
  accepts only the observed checks above.

The [investigation review](investigation-review.json) preserves the
[off-screen baseline](ordinary-r10-process.json), [first-show-only candidate](ordinary-r11-process.json)
and [candidate with the teardown warning](ordinary-r12-process.json). None was
silently accepted after a later success. Exact hidden frame-margin values were
not sampled; the mapping explanation is distinguished from observed bounds.

Final ordinary GUI SHA256 starts `2aef3286`, Core `0404d1f6`, WASAPI `4e4e33c0`.
Complete identities and2,086 [source hashes](source-inputs.json) are saved.
Actual copies are frozen under
`build-windows/beta-queue-workflow-20261022/source-r13`. The native policy and
its test are unchanged from source-r11, where the15-case executable was built.
The [routing-scope comparison](routing-scope-delta.json) preserves the separate
earlier key-routing evidence; it is not a new whole-app test result.

[Queue Find ownership](../home-queue-find-20261023/README.md) has its separate
before/after input-filter and routing receipts. Physical dragging, queue
addition/removal, real bookmark/chapter flows, full accessibility/design,
Windows10, physical DPI/display hotplug and final packaging remain open.
The slow hand-drag check is saved for the morning. No production drag code,
security/privacy settings, installed dependencies or published package changed.
