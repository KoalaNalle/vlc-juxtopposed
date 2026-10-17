# Native Windows Performance preferences model

This is a controller checkpoint, not complete Performance-page or beta acceptance.
The typed model maps the 12 native preference fields and rescan action, with
Advanced Options intended as a QML disclosure. MainCtx/QML mounting is pending.
Each consumer gate defaults false. Codec/frame/clock acceptance remains separate.

In the configured MinGW/Qt build, read the generated Qt Makefile followed by
modules/gui/qt/tests/native_library/performance.mk; pass a fresh absolute
PROPOSAL_OUT directory and invoke performance-preferences-native with -j1.
The maintained ordinary Autotools target is test_qt_performance_preferences.exe;
Meson exposes the executable without registering an unisolated integration run.

On Windows, use the E-only Common.ps1 process environment and run:

~~~powershell
python test/libvlc/native_performance_preferences.py `
  --package <owned-portable-runtime> `
  --core <matching-libvlccore.dll> `
  --cache-generator <matching-vlc-cache-gen.exe> `
  --test-exe <fresh-.libs/test_qt_performance_preferences.exe> `
  --output-dir <new-owned-output-directory>
~~~

The driver creates a fresh immutable-link portable runtime, detaches only its
own replacement links, regenerates the plugin cache, and runs the quiet owned
native process. It requires all 16 exact ordered passes, normal exit, no skipped
cases, no forced cleanup or temporary-directory cleanup warning, and an unchanged
source package Core/cache. The actual source runtime remains intact.

The tests use real native config, module inventory, file sharing rejection,
Windows priority readback/restoration and plugin generation. They do not mock
those boundaries. The model's consumer flags are enabled only to exercise its
mapping; no claim is made about downstream codec/frame/clock behavior or visuals.
Only the owned test process changes scheduling and restores its initial class.
The real-time rule permits exact success or explicit refusal preserving state;
a granted path remains unverified on this account. No raised-priority media
stress, host-priority change, privilege grant or OS settings change is performed.

The GUI page, full later-source regression, themes/DPI, Windows 11 and final clean
licensed beta package remain pending. See the checked checkpoint under
doc/juxtopposed-references/native/guest-performance-model-20261017.
