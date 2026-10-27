# Native application-Quit regression

This opt-in fixture exercises the real `DialogsProvider` and primary VLC player
inside an owned Windows Qt window. Never add its translation unit to the
ordinary GUI sources. Build from the configured Windows Qt build directory:

```sh
make -f Makefile -f "$repo/modules/gui/qt/tests/native_ui/sidebar_pins_journey.mk" \
  QA_SOURCE="$repo/modules/gui/qt/tests/native_ui/quit_stop_journey.cpp" \
  QA_OUT="$owned_output" -j2 owned-native-ui
```

Run the reusable Windows Python runner with a complete matching package,
the private DLL, a JSON mapping of source paths to SHA-256 values, an owned
long audio/video fixture, and a new output directory:

```powershell
python modules/gui/qt/tests/native_ui/run_quit_stop_journey.py `
  --package <matching-package> --qa-gui <private-DLL> `
  --inputs <matching-source-manifest.json> --media <owned-long-media> --expected-media audio `
  --output-dir <new-owned-output> --mode playing --startup-gate
```

With `--startup-gate`, inspect the actual native window before creating the
output directory's `continue` marker. `--fullscreen` enters fullscreen inside
the fixture through the player after playback is established. Use
`--expected-media audio`, `video`, or `av` to require real MMDevice/WASAPI,
Direct3D11, or both backends respectively. Idle and rapid startup cancellation
do not require a renderer to have opened. No user media or normal VLC settings are
modified. The runner stages hard links to immutable package files, unlinks the
GUI replacement target, and uses isolated settings and cache directories.

Cases are `idle`, `playing`, `paused`, `rapid`, `repeat`, and `restart`.
Rapid Start and Quit run without processing the Qt event loop between them;
the cached GUI state must still be STOPPED. This does not guarantee that the
native player's asynchronous state is still STOPPED: both states are recorded.
Repeat issues six Quit calls in the same turn. Restart registers its native
listener before the provider's listener, deliberately queuing a new Start
before the provider's stop finalizer. That restart must occur after Quit was
requested. Use long, valid media so natural completion does not substitute for
the intended trigger.

Acceptance requires the appropriate pre-quit state, native STOPPED at interface
owner teardown, normal process exit, unchanged source/media, no Qt diagnostics or native
errors, and stop acknowledgment before Qt teardown in the final flushed log
(except idle, which needs no stop request). Buffered live file-log output is
not a real-time completion signal. The fixture detaches its observer at MainCtx
destruction and binds queued native events to that same owner. A video window
may retain QApplication beyond interface teardown and primary-player deletion;
`aboutToQuit` must only update the report after observer removal.
The runner does not kill a timed-out VLC;
it records the owned PID and leaves the failure available for inspection.

This tests application Quit only. It does not qualify arbitrary interface
unloading, every video backend, multiple physical audio outputs, shared Qt
profile ownership, actual Windows DPI changes, or the complete beta package.
