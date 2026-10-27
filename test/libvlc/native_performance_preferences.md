# Native Windows Performance preferences model

This isolates the native controller; page and media acceptance are separate runs.
The typed model maps the 12 native preference fields and rescan action, with
Advanced Options implemented as a QML disclosure. MainCtx owns the production
controller. Each consumer gate defaults false unless explicitly enabled by its
caller; these tests enable the mapping without claiming downstream behavior.

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
native process. It requires all 17 exact ordered passes, normal exit, no skipped
cases, no forced cleanup or temporary-directory cleanup warning, and an unchanged
source package Core/cache. The actual source runtime remains intact.

The tests use real native config, module inventory, file sharing rejection,
Windows priority readback/restoration and plugin generation. They do not mock
those boundaries. A saved but unapplied real-time request remains explicit and
can be cleared; it must not silently look like an ordinary OFF preference.
Only the owned test process changes scheduling and restores its initial class.
The real-time rule permits exact success or explicit refusal preserving state;
a granted path remains unverified on this account. No raised-priority media
stress, host-priority change, privilege grant or OS settings change is performed.

For the actual GUI and decoder/frame/clock runs, see
[native_performance_consumers.md](native_performance_consumers.md) and the
[page proof](../../doc/juxtopposed-references/native/guest-performance-page-20261017/README.md).
Full later-source regression, platform/DPI coverage and the clean licensed beta
package remain separate. The earlier 16-case checkpoint is preserved under
doc/juxtopposed-references/native/guest-performance-model-20261017.
