# Settings quality checks — 17 October 2026

Native settings, Extensions replay and ordinary playback all passed in the scoped checks below.
This is scoped Windows 10 settings quality evidence; the full beta is incomplete.
The source is the containing commit, based on edc97c6d26575ecb7a9ae7a9c27ddcf66bc93deb.
All development storage is on E:, with one build/test workload at a time.

## Changes and evidence

Light Search placeholders previously reused the palette mapping for borders:
#404040 became #E0E0E0 on #EDEDED (1.13:1). The actual production palette now
uses its native secondary foreground for placeholders in Light mode, preserving
each original Dark value. This applies to Global Settings, Extensions, New
Playlist and the two Network fields. The controlled native-provider component
regression passes at 7.22:1. Native values are recorded separately in the final
journeys; this is not full accessibility certification.

The native accessibility interface exposed empty names on custom settings
buttons, including theme radios and Advanced Options. Each of the eight copied
button bases now binds Accessible.name to its existing translated text; the
Extensions custom-content Advanced button has an explicit translated name.
Original artwork/font bytes, geometry and preference behavior are unchanged.

Fifteen maintained component scripts pass 272 reported groups. After fixing the
last five copied bases, those five scripts pass another 73 reported groups
(the same groups repeated, not extra unique features). Network and New Playlist
component checks also pass. These use supplied state/offscreen rendering and do
not establish native persistence, picker behavior, Windows DPI or visual scores.

Fresh native Library/preferences fixtures pass metadata18, opening42 and
process9; the standard current subtitle target passes37: 106 cases, zero
failures/errors/skips, normal exits, no forced cleanup. They use unchanged
Corebf2940 and the recorded native plugins, with the earlier GUI17d870 staged.
These checks do not claim acceptance of the later GUI7015bc. Runtime identities
are preserved in results/native-regressions.json.

## Native settings replay

The actual app passes playing1652, paused1653 and restarted14 checks (3319
repeated assertions, not distinct features), with normal exits, no forced
cleanup, zero selected QML diagnostics and no owned fault dumps. Each playing
and paused run visits all 13 actual MainCtx-backed category bodies in Dark and
Light, verifies saved native theme/accent values and selected radio semantics,
checks Space activation without changing primary input/state/rate, completes
one normal plus 13 compact 900x650 focus cycles, and closes/reopens/Escapes.
The playing clock advances; the paused clock remains stationary. The next
process restores Light/Purple. Both the global and Extensions search fields use
real native colors with measured contrast 7.92:1 and 7.40:1 respectively.
The result includes a real System selection/provider check, not an OS theme
notification change. All three also reach the owned MainCtx-destruction probe
and exit normally after three live native-caption hit tests.

The same candidate then passes all five original Extensions journeys (190
checks) and five ordinary GUI cases: redesigned/traditional controls, Library,
local finite HLS, and bounded read-only Downloads playback. All exit normally;
a separate review finds zero selected QML/assertion diagnostics in all five
ordinary logs. The Downloads file size/mtime are unchanged; this is not a full
file-content hash. Personal filenames, raw logs and decoded frames remain private.
The ordinary GUI has no QA marker and both source runtime/package hashes are
preserved. All 1910 ordinary and five private frozen inputs still match.
Ordinary GUI SHA-256 is
7015bc0e19d8e01a3cd948ec494a49abaa021deec8366727af5afb724cf293ac;
Core remains bf29403169b936e2156bcfcd0f03a38fbbaad0d9b54fd0e7d9e843e4bdb5bc33.
Existing static-link/cross-host wrapper warnings remain in the incremental build
logs; no native Meson build or clean release build is claimed.

## Rejected attempts

R1 incorrectly treated the modal header as outside its content; the fixture now
accepts the actual sibling header and content. R2 exposed unnamed theme buttons.
R3 exposed the same defect in the copied Playback button base. R4 completed all
14 focus cycles, then failed because the private fixture requested globalAccent6
instead of the real generalAccent6. Each process exited normally, with zero
selected QML diagnostics. These remain failed runs. The standalone placeholder
negative baseline remains failed at 1.13:1. None is established as the cause of
the separately reported security trip.

## Reproduction and limits

Use the migration kit Common.ps1 environment in E:/Modding/vlc-juxtopposed.
The reviewed reproduction/ scripts describe this guest's configured incremental
WSL build and owned native tests, not a portable clean-release build. The Linux
legacy /mnt/c source alias resolves internally to E:, with no Windows C: mount.
Use one worker and preserve the build lock. Never overwrite a binary shared by
hard-linked runtimes. Stage fresh output/profile/portable directories on E:.

The maintained runner modules/gui/qt/tests/native_ui/run_extensions_journey.py
adds settings-playing, settings-paused and settings-reopen to the existing
Extensions modes. It uses an owned six-minute silent WAV and a private
instrumented GUI. Native keyboard focus is checked for modal ownership, visible
viewport center and a nonempty native accessible name. This does not measure
screen-reader speech, every accessibility role/state, focus-ring appearance,
all window sizes, or the OS DPI/monitor matrix. System selection saves and
retains the native provider; actual OS theme-change notifications remain open.

No private test code is included in the ordinary GUI. Actual 150/200% Windows
scaling, monitor movement, shell-native chooser, independent design fidelity
scores, remaining confirmed surfaces, final Windows11 coverage and a clean
licensed matched package remain open. Raw logs, binaries, dumps and personal
Downloads filenames/frames stay in ignored E: build output.
