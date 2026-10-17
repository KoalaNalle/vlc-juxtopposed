# Native Network window journeys — Windows 10, 17 October 2026

The actual VLC window now passes 31 editing/failure/retry checks and 11 restart
checks in two normally terminated processes. These include repeated target and
window-ownership assertions; they are not 42 separate product features. The
[raw journeys](journeys.json) and [verification receipt](verification.json)
record the exact scope and identities.

The test opens the real shared settings popup, clicks the Network category and
checks that compiled QML binds MainCtx's actual NetworkPreferencesModel. Owned
pointer/key events select Normal and Custom, edit native buffering and enter a
masked synthetic proxy password. A real Windows file lock rejects persistence,
keeps the native value unchanged and permits retry after unlocking. Advanced
closes the QML modal and opens the actual native expert preferences QWidget.
A second application process confirms saved buffering/password presence and an
empty password editor. Both processes quit normally. Logs contain no QML
diagnostics and exactly the expected locked-file error.

The [native capture](network-native-dark.png) and
[restart capture](network-reopen-dark.png) come from the owned QQuickWindow at
1500 × 1000 and observed device-pixel ratio 1. These are actual native services
and compiled pages, with no supplied controller. This is one Dark/current-monitor
variant; it does not establish 150/200% Windows DPI, other themes, independent
visual scoring, every Network control or playing/paused independence.

The private test GUI links exactly the ordinary production objects/resources
plus one additional test TU and QtTest. The ordinary GUI remains unchanged at
`e429f1476bf91d336bc263ee06bcce1351076c384dae54998e066f96980a3643`;
the private GUI is
`7ddcb1f649115fb86b0d258c6ae10dc0b74bbe8025169e348362171bddd6a562`.
Both use Core
`49d4c62a6e0be9aa586ca82a7448783de191e2024387352c42688f993784bd7e`.
The fixture source matches the compiled TU byte for byte. All 2,318 selected
baseline inputs match except an appended test-only `EXTRA_DIST` registration;
ordinary production C++/QML/resource inputs are unchanged. No QA hooks enter
the ordinary package. Build attempts with missing QtTest include flags and an
incorrect test-only Root accessor are retained as compiler failures; neither
changed production code. The corrected private build and portable runner pass.

The maintained [recipe and runner](../../../../modules/gui/qt/tests/native_ui/README.md)
are opt-in and never installed. Use fresh E: outputs and one worker on the
travel computer. Preserve each private build while a runtime hard-links it.

The [Network implementation checkpoint](../guest-network-20261017/README.md)
retains proxy/TLS/controller/ordinary evidence and an unresolved intermittent
subtitle-role stall. These window journeys do not explain that stall or accept
the complete player regression, full beta, Windows 11 or final clean package.
