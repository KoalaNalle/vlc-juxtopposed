# Queue filter layout, 23 October 2026

The filter now opens below the toolbar and reserves space above the first row.
Closing it restores the existing authored layout. Add is fully reachable while
filtering. The change affects two presentation components; native models,
SearchBox behavior, key routing and docking state engine are unchanged.

The real Qt regression uses the complete panel and exact popup/inset bindings
with supplied filter content. All six width/scale combinations pass. Original
source fails precisely the toolbar separation and Add-center input checks in
each combination. Existing canonical panel/row geometry, artwork and opt-out
checks also pass. These supplied scales do not qualify physical Windows DPI.

Ordinary GUI21d55d1c verifies the actual filter below the toolbar, center-click
Add opening the native picker, appending Fir under Birch filtering, and Escape
restoring original row positions and Cedar/Birch/Fir order. The owner stays
paused. Normal Quit exits0 without captured runtime errors/Qt exceptions or
source/runtime drift. Source-r19 preserves2,108 actual inputs.

The separate initially-paused audio finding is reproduced without a start-time
option: the thumb moves while elapsed stays0 until playback begins. It remains
open. No physical dragging/removal, touch, complete design/accessibility/DPI,
Windows10 or full Queue/beta acceptance is claimed. The initial fixture font
warning and original-source failures remain recorded.

- [Checkpoint and artifact hashes](checkpoint.json)
- [Bounded ordinary review and limits](ordinary-review.json)
- [Original-source component failures](layout-before.json)
- [Repaired component cases](layout-after.json)
- [Canonical geometry and asset result](canonical-panel.json)
- [Initial font setup failure](initial-font-setup-failure.json)
- [Ordinary process receipt](ordinary-process.json)
- [Raw UI observations](ui-observations.json)
- [Source inputs](source-inputs.json)
- [Source delta](scope-delta.json)
