# Video flip and rotation checkpoint

Recorded 14 October 2026. This advances milestone 5 of the active 0–8 vertical-slice Goal. Original visual design: [Juxtopposed](https://www.youtube.com/@juxtopposed).

The [09:00 video reference](video-reference-09m00.png) confirms a three-part None/Horizontal/Vertical selector and a numeric degree field above Zoom. The implemented controls follow that structure using the saved reference and local Inter font. Dimensions remain provisional. [Video component capture](video-component.png) is the actual production settings popup in a Qt harness with supplied controller state, **not a native VLC desktop capture**.

## Native behavior

`Player.videoTransform` follows the actual video output supplied by the existing player lifecycle callbacks. It holds that output, observes the filter chain/type/angle/motion variables and clears its observers when the output changes. Unavailable output disables the controls. Merely creating or changing tabs makes no engine writes.

User changes enable/remove VLC's existing `transform` and `rotate` filters. Other filter entries retain their original option text and relative order; VLC's chain parser handles colons inside quoted options. The managed rotation and flip are appended after those entries, with transform last: transform sets orientation metadata, which would otherwise be lost by a later pixel-producing filter. Rotation uses VLC's native positive counterclockwise angles, 0–360 degrees; 360 removes rotation. The flip applies after rotation. Invalid/non-finite input is ignored. Static user rotation turns off motion-sensor rotation.

Existing effects-dialog quarter-turn, flip, transpose and antitranspose states are interpreted in the new controls. A user edit normalizes them to the supported filter arrangement. The native pixel test confirms orientation preservation for all seven types on the square fixture. Arbitrary rotation retains VLC's fixed canvas; non-square bounds/cropping, source orientation metadata and hardware-renderer variants still need native acceptance. Flip/rotation persistence across app restart is also pending.

Filter mapping variables live with the output, matching VLC's proxy-callback ownership contract. Observer references are released on detach, while mappings remain available to other effects controls.

The numeric component also gains sufficient vertical padding for its 14px text inside a 32px field. The preceding 10px padding clipped the text. The popup regression verifies available text height, typed rotation, equal-width flip cells, external updates, availability and absence of tab-switch writes.

## Verification

All nine Qt 6.8.3 component suites pass on the final sources. Production popup/control loading, model updates, typed numeric input, delay units, actions, availability, dismissal and 100/150/200% renders remain covered. This supplies component evidence; VLC imports/controllers and the actual desktop journey remain separate gates.

The native Windows QtTest executable uses the **same VideoTransform class**, actual packaged LibVLC/core/filters and a generated four-quadrant MPEG-4 video. It receives final memory video output as BGRA and tests decoded/rendered pixels. It waits for actual frame callbacks before holding the output: LibVLC's `has_vout` can report a temporary dummy output before startup and is insufficient as a readiness check. An earlier diagnostic failed because it obtained that dummy; that run is not counted as a pass.

[Final results](video-transform-results.txt): **11 passed, 0 failed, 0 skipped**, including setup/cleanup, flips, 90°/37° rotation, combined transforms, restoration, all seven external orientations, live engine callbacks, invalid input, quoted option preservation and observer detach/reattach. The 37° case also checks black corner pixels, distinguishing actual rotation from an unchanged quadrant pattern.

Quadrant order is top-left, top-right, bottom-left, bottom-right:

| Output | Expected and observed |
| --- | --- |
| Original / restored | Red, Green, Blue, Yellow |
| Horizontal | Green, Red, Yellow, Blue |
| Vertical | Blue, Yellow, Red, Green |
| 90° native rotation | Green, Yellow, Red, Blue |
| Horizontal after 90° | Yellow, Green, Blue, Red |

Captured final video frames: [original](native-original.png), [horizontal](native-horizontal.png), [combined](native-horizontal-rotate-90.png), [37°](native-rotate-37.png). These are generated test media, not designer artwork or full-app screenshots. Full logs and additional frames remain under ignored `build-windows/transform-native-checks`. The tracked [fixture recipe](../../../test/libvlc/generate_slice_fixtures.sh) creates `quadrants.mp4` in a fresh directory. Set `VLC_TRANSFORM_FIXTURE` and `VLC_TRANSFORM_OUTPUT` to use the native diagnostic; no supplied fixture means a reported skip.

The final Windows Qt plugin rebuild exits 0. Autotools/Meson include VideoTransform and its diagnostic, and the class is registered in VLC.Player. Staged Qt DLL SHA-256: `e2ee5626d356bf885e73e5ed009636373a4ac88dd910e90aec02525bd50114b8`. Native test executable SHA-256: `8fe535fdf5ae23ce34d0ec5448ef80a97d68d86c6988a01c149a59a6200a5e89`. Tested core SHA-256 remains `1d3c3b8e9794d512fd0abd8e9d4bc86a7d892a9a1cc8b6cb3447908875bc9463`.

The new DLL is staged in `build-windows/slice-package/plugins/gui`; the preceding settings DLL is retained as `libqt_plugin.dll.before-transform-20261014`. The user's working package is preserved. A successful build and these rendered-backend checks do not establish latest-GUI startup or visual acceptance.

## Open gates

The existing native desktop test still awaits the user's Windows Firewall prompt dismissal. GUI flip/rotation/focus/media-switch checks, whole-app captures, HDR/resolution, remaining effects, subtitle appearance/presets/timeline, other audio effects, preferences/theme setup and the rest of the [full plan](../../juxtopposed-vertical-slice-plan.md) remain open. No whole-app fidelity score or completion is claimed.
