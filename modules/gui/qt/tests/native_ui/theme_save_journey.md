# Checked appearance and legacy controls

`theme_save_journey.cpp` is an opt-in Windows fixture compiled through
`sidebar_pins_journey.mk` with `QA_SOURCE` pointing at this file and a fresh
`QA_OUT`. It must never enter the ordinary Qt plugin source list. Its MainCtx,
General preference service, SPrefsPanel combo and CheckableListMenu are the
production implementations.

Use a staged matching package with a private GUI DLL copy, an owned portable Qt
store, no media library and no privacy wizard, and set:

- `VLC_OWNED_UI_BASE`: the owned output directory containing the runtime/store.
- `VLC_OWNED_UI_REPORT`: an owned journey JSON path.
- `VLC_OWNED_UI_START_GATE`: an initially absent owned marker path. Inspect the
  app window before creating the marker. An empty value runs without that gate.

The fixture waits for the real dialogs host/MainCtx, commits Dark, constructs the
actual legacy Interface panel, blocks replacement of its own INI, and sends Up to
the theme combo. It checks error reporting, accepted Dark state in the model and
widget, exact disk preservation and successful retry after releasing the handle.
It then performs the equivalent rejected/retried pointer choice through the real
exclusive menu and checks both widgets follow the committed choice. No privacy
controls are operated and the panel's Apply action is never called.

The panel closes as Cancel and restores its original Dark choice after the
successful Light checks. That intentional rollback must not be confused with an
unchanged older process overwriting a newer saved choice during shutdown. The
two ordinary process closing orders and cold restart qualify that separate case.

Require all15 checks, normal process exit, no native/Qt diagnostics in file log
or stderr, matching immutable source/runtime hashes, and no forced cleanup.
This fixture is idle; it does not qualify playback, audible output or design
fidelity. The 21 October results and frozen source hashes are published under
`doc/juxtopposed-references/native/home-shared-qt-20261021`; raw owned builds,
drivers and logs remain in `build-windows/beta-shared-qt-20261021`.
