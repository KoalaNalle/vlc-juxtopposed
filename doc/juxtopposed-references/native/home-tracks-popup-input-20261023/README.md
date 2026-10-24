# Track-menu input repair, 23 October 2026

The track menu now closes with Escape and consumes the outside click used to
dismiss it, preserving paused playback. Escape from an audio/subtitle
synchronization editor returns to the list; the next Escape closes the menu.
Keyboard-visible focus returns to the originating audio/subtitle button, so
Space reopens that menu instead of accidentally starting playback.

The complete-menu Qt fixture passes four cases; the original fails its two
outside-click cases. Actual NavigationAttached plus exact production bindings
passes six checks including setup/cleanup; the same executable fails four
cancellation rows with the original bindings. These rows share navigation
routes and use supplied event/hierarchy/recipient objects, not native decoders.

Ordinary GUI716862a1/Core4422fbb0 verifies both originating buttons, Escape,
Space reopening, outside dismissal, audio synchronization Back, resume/Pause
and normal Quit0. Selected menu interactions retain the exact paused6s slider
value. The prior R3 ordinary test qualifies subtitle synchronization Back on
identical page handlers, and retains its failed Space check before the focus
repair. Component/native setup failures and intermediate results remain saved.

This accepts selected popup input only. Every ordinary run still exposes an
initial-paused H264/MKV video placeholder with a buffer-deadlock diagnostic;
first Play establishes video. Paused embedded/external cue display is also
unresolved: the external first cue appears after resume. These are open
findings, so none of the ordinary runs is promoted to an overall clean pass.
External-file automatic-selection policy also needs investigation.

Source-r4 contains2,139 checked copies. Core4422fbb0, WASAPI4e4e and native track
selection/decoding code are unchanged. The final ordinary plugin contains no
private diagnostic hook. Windows10, physical audibility/outputs, full design,
accessibility/DPI, remaining consumers, independent review and packaging remain
open under the full13-category/32-surface beta goal. No dependencies, commit,
push, release, upload or shutdown occurred. Nonurgent questions stay for morning.

- [Checkpoint and artifact hashes](checkpoint.json)
- [Review, limitations and open findings](review.json)
- [Component original failures](component-before.json) and [repaired result](component-after.json)
- [Native navigation original failures](navigation-before.txt) and [repaired result](navigation-after.txt)
- [Final ordinary process](ordinary-r4-process.json) and [observations](ordinary-r4-observations.json)
- [Initial cue investigation](ordinary-r1-observations.json)
- [Intermediate Escape repair and focus failure](ordinary-r3-observations.json)
- [Native test setup failure](native-setup-failure.json)
- [Build warnings and identities](build-review.json)
- [Frozen source inputs](source-inputs.json)
