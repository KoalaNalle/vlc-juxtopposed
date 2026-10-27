# Fullscreen and volume-step activation — 19 October 2026

Scoped implementation, actual settings journeys and ordinary pointer-to-keyboard
volume checks pass on R4. The complete beta remains unfinished.

MainCtx now links only the verified embedded-fullscreen and authored volume-step
capabilities. Other Interface gates remain independent. Audio device support no
longer authorizes receiver passthrough: `windowsPassthrough` is separate and both
Windows device/receiver gates remain disabled in this R4 ordinary app. Later
[Windows Audio activation](../home-audio-device-activation-20261019/README.md)
has its own source/binary scope and links device/fixed startup independently.

## Evidence

- [Actual settings journeys R4](actual-app-r4.json) pass paused96, cold reopen77
  and playing92: 265 checks, three normal exits, unchanged source/runtime/media
  identities, no Qt diagnostics and no unexpected native errors. The two save
  failure/retry cases retain their deliberately induced config-sharing error.
  New direct-pointer-then-Right checks establish active mouse focus without
  `visualFocus`, the correct five-point native increment and paused-clock
  preservation. The same desired R3 app regression failed before the repair.
- [Compilation R4](compilation-r4.json) binds ordinary GUI
  `758df308cca3974c767652d66d2ce4273c2ec585a393db2d40b31aa2022df71b`
  and private GUI
  `f7e7d95db75e1dbff83a15d40ff4707c07010e8a7cb35562eccda6bf24c38e48`.
  `build-windows/beta-settings-activation-20261019/source-r4` preserves the inputs.
  Its build-time status predates the later runtime passes. This tuple includes
  the separately verified repaired FLAC packetizer/demuxer. Core/audio/library
  remain protected and the ordinary GUI has no private hooks.
- [Key routing R4](key-routing-r4.json) passes12 cases using exact extracted
  production bodies, native hotkey callbacks and Qt receivers. All four plain
  arrows reach the mouse-focused volume control without native dispatch;
  Ctrl+Right retains the prior native policy. Modal/sidebar/unscoped policies
  retain their checks. Compiled extraction/source identities match. This is a
  routing fixture; actual Windows input has separate evidence below.
- [Affected fullscreen replay R4](fullscreen-r4.json) passes138 checks across
  audio/video On/Off, four normal exits, no Qt diagnostics/native errors and
  unchanged source/runtime/media identities. Hidden-input rejection, focus
  transfer and exact paused input/clock/rate/volume/mute remain covered. Generated
  captures verify controls/focus, not D3D video pixels or whole-layout fidelity.
  MP4 edit/sample-table and initial window-size warnings remain recorded; the
  latter still needs a separate rendering/size review.
- [Audio defaults/PCM32](audio-defaults.json) and [real Windows Audio4](audio-windows.json)
  pass normally without Qt diagnostics, including two real WASAPI output sessions.
  The defaults suite now verifies that a device-only capability refuses receiver
  writes without changing native state or saved bytes. Explicit receiver linking
  in that suite tests storage mapping, not physical receiver playback.
- Historical [actual settings journeys R2](actual-app-r2.json) pass paused92, cold reopen77
  and playing88: 257 checks, three normal exits and no Qt diagnostics or unexpected
  native errors. Actual pointer/keyboard edits, invalid-value refusal, immediate
  volume-step consumption by native actions, fullscreen restart semantics,
  genuine locked-save rollback/retry and reopening are covered. Each mutation
  journey retains its one deliberately induced Windows config-sharing error;
  it is not advertised as an error-free negative-control run.
- [Compilation R2](compilation-r2.json) binds ordinary GUI
  `ca6e393d13b9bf9fd709047d24ea80ded9b3d9eb15904d8f7070b2bf9308ce2a`
  and private GUI
  `9248d347f2de868c83a98f2c60a4ac595d410a2f901b5d106cd9a323fe4b77fe`.
  The local source checkpoint is
  `build-windows/beta-settings-activation-20261019/source-r2`.
  The ordinary GUI contains no private test hooks. Those journeys used their
  recorded pre-FLAC-repair module tuple; later decoder changes need separate
  qualification.
- [Interface component11](component-interface.json) retains geometry/assets,
  picker/busy/rollback and focus coverage. A new boundary regression failed on
  the previous QML: inserting restart feedback left the focused32-pixel switch
  ending at889.5 in a viewport ending at882.5. The unchanged test passes after
  Interface re-reveals its active control when form height changes. R2 native
  checks and the [generated-only capture](native-fullscreen-settings-editor.png)
  independently confirm the full switch remains visible. Component scales are
  not Windows-DPI acceptance.

## Ordinary Windows input repair

[Ordinary R4](ordinary-windows-r4.json) on monitor2 accepts Ctrl+P, Audio selection, actual numeric
typing and Return. Entering2.5 writes `volume-step=6.400000` to its owned native
config without restart feedback. Its generated input stays paused while editing.

Direct coordinate selection followed by Right changes72.5% to75%; Left returns
to72.5%. The paused clock remains00:00:00 throughout. Up follows Qt navigation
to the source row without changing volume; this horizontal slider does not use
Up as an increment. Ctrl+Q closes normally; generated media, protected runtime
and source geometry stay unchanged. No Qt diagnostics/native errors/security
prompt were observed. UI Automation retains an old source label as focused
element; complete screen-reader behavior remains open.

The earlier R2 ordinary attempt instead sought00:00:00 →00:00:10 →00:00:20.
The R3 private regression reproduced this normally. The window handler deferred
player hotkeys only for `visualFocus`, which mouse focus does not set. R4 gives
the live slider an explicit plain-arrow scope, leaving modified shortcuts and
other controls under their existing policies. The [component8](component-volume.json)
retains original artwork/geometry, pointer, keyboard, wheel and mute contracts.
It passes before/after; it alone did not reproduce the window-handler defect.

Earlier R1 functional app checks and the initial R2 launcher failure are retained
under the ignored work folder. The launcher stopped before app startup after
referencing a nonexistent fixture directory; corrected R2a uses the preserved
fixture and the same R2 binaries. No security setting, dependency installation,
commit, push or publication occurred.
