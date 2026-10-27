# Bookmark and chapter workflows — 23 October 2026

Chapter times now show their correct zero-based offsets. Bookmark and chapter
rows receive pointer input, and mouse-selected rows retain Enter for local
activation even when the player prefers playback hotkeys.

The [checkpoint](checkpoint.json) qualifies these selected repairs:

- [Native model tests](native-after-process.json): nine pass, with no failures
  or skips. Actual chapter metadata/selection, bookmark add/rename/seek/remove,
  media switching, late registration, stop/restart, database persistence and
  settled destruction use the production models and a real local media library.
  The [original model](native-before-process.json) fails chapter-zero validity;
  the [separate review](native-review.json) preserves that receipt's original
  failure status and explains its two historical headless teardown diagnostics.
  The final native run retains those exact close-notify messages, verifies their
  teardown context and reports no other error diagnostics.
- [Pointer fixture](pointer-after.json): six checks pass using the exact
  production handler subtrees in real Qt Controls, with supplied selection and
  activation recipients. The [original hierarchy](pointer-before.json) fails
  all six. Native seeking and desktop behavior are separate checks.
- [Ordinary app review](ordinary-review.json): the actual app shows chapter
  times 00:00:00, 00:00:06 and 00:00:12. Double-clicking Blue seeks to its blue
  frame; selecting Green and pressing Enter seeks to its green frame at 12s.
  UI bookmark creation, pointer/keyboard seeking, media-specific empty states
  and restoration all work in the selected journeys.
- A [cold restart](ordinary-cold-process.json) restores all three bookmarks;
  activating the newly created 12s bookmark still seeks correctly while paused.
  [Read-only database verification](persistence-readback.json) confirms the
  stored offsets are exactly 0, 6000 and 12000 milliseconds for one media item.
- Both final ordinary processes exit normally with no captured errors or Qt
  exceptions. Their original `passed=false` / `observationsRequired=true` flags
  are retained; the separate review accepts only the observed UI checks.

The [investigation](investigation-review.json) retains the initial ordinary
failure, the intermediate pointer/time repair with Enter still failing, the
corrected fixture expectation and a failed native executable-staging attempt.
Next/Previous intentionally start playback; the short fixture reaches EOF
during one return/refresh. That raw state remains recorded, followed by a
successful return and bookmark restoration. This is not a latency claim.

Final GUI SHA256 starts `f528dd1c`, Core `0404d1f6`, WASAPI `4e4e33c0`.
The 2,103 [ordinary GUI/producer input hashes](source-inputs.json) have actual
copies under `build-windows/beta-queue-workflow-20261022/source-r17`. The native
model and test retain a [separate source record](native-source-record.json).
The [R17 comparison](scope-delta.json) confirms that only the existing local
navigation-scope property was added after R16; native models and pointer
subtrees are unchanged.

Ordinary UI bookmark removal/rename, physical touch, screen-reader behavior,
physical DPI, Windows 10, all formats, full design comparison and release
packaging remain open. [Queue Find](../home-queue-find-20261023/README.md),
[detached placement](../home-queue-detached-20261023/README.md) and
[live docking visibility](../home-queue-visibility-20261023/README.md) retain
their separate evidence. The full beta goal remains active.
