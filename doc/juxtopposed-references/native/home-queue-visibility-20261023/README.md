# Live Queue visibility — 23 October 2026

Switching a visible Queue between docked and detached mode now keeps it visible.
The live transition follows the requested panel visibility instead of applying
the startup rule again. The startup behavior and existing state architecture
are unchanged.

The [checkpoint](checkpoint.json) qualifies this selected repair:

- The [production FSM fixture](visibility-after.json) passes 20 supplied-state
  checks in a real Qt engine, including startup controls, audio/video redocking,
  hidden queues, toggles and repeated round trips. The [original FSM](visibility-before.json)
  fails exactly three redocking cases. This fixture is separate from native
  media and desktop acceptance.
- The [ordinary review](ordinary-review.json) records two actual app journeys:
  paused synthetic audio and embedded H264 video. Show, undock and redock retain
  all five original rows without another Play queue activation. Cedar remains
  current and paused at 00:00 throughout. The audio journey also verifies that
  undocking a hidden queue keeps it hidden.
- Both [audio](audio-process.json) and [video](video-process.json) receipts show
  normal Quit, exit 0, stable source/runtime identities and no captured errors
  or Qt exceptions. Their original `passed=false` and `observationsRequired=true`
  remain intact; the separate review accepts only the observed checks.

The first video redock capture was transitional. A fresh observation, without
another click, showed the complete docked panel. No animation timing bound is
claimed. MP4 parser warnings and an initial window-size warning remain in that
run's log; it is not described as warning-free.

The [investigation](investigation-review.json) records the cause and the failed
initial fixture setup. Final ordinary GUI SHA256 starts `bd8f6cf7`, Core
`0404d1f6`, WASAPI `4e4e33c0`. All 2,090 [input hashes](source-inputs.json) have
actual frozen copies under `build-windows/beta-queue-workflow-20261022/source-r14`.
The later [video runner](video-runner.py) has its own hash in its process receipt.

[Detached placement and loader lifetime](../home-queue-detached-20261023/README.md)
and [Queue Find](../home-queue-find-20261023/README.md) retain their earlier,
separate evidence. Physical dragging, addition/removal, real bookmarks/chapters,
full design/accessibility, physical DPI, display hotplug, Windows 10, performance
and final packaging remain open under the complete beta goal.
