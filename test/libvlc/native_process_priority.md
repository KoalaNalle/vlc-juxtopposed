# Native Windows process scheduling

The Core process-priority API queries and updates only its own process. It
requires exact Windows class readback and attempts to restore the prior class
when a request fails or Windows grants a different class. It does not enable a
token privilege. Inherit/false leaves an embedding application's class alone.

Windows startup applies policy after the full command line and saved settings
have loaded. New explicit CLI fields take precedence; otherwise an explicit
legacy high/no-high request overrides saved policy. Legacy no-high preserves the
inherited class. With no explicit CLI request, saved modern settings precede
saved legacy high. Default inheritance changes nothing.

The base levels are -2 Idle, -1 Below Normal, 0 Normal, 1 Above Normal, 2 High,
and native default 3 Inherit. Real-time is a separate boolean. Core validates
the full native integer before narrowing; unsupported/overflow values produce
an error without changing the class. Integer choices intentionally avoid the
generic variable range's silent clamping to Inherit.

Build the explicit probe with its native recipe after the configured test
Makefile, using one job:

~~~sh
make -C "$build/test" -f Makefile -f "$repo/test/priority-native.mk" \
  -j1 PRIORITY_OUT="$freshBuildOutput" priority-startup-native
~~~

The standard test_priority_startup Autotools extra-program and Meson target
also build the probe. They are not automatically run outside an owned runtime.

~~~powershell
python test/libvlc/native_process_priority.py --package $package --core $newCore --probe-exe $probe --output-dir $freshOutput
~~~

The 21 fresh child processes initialize LibVLC without media, record actual
GetPriorityClass values and restore their initial class before teardown/I/O.
They cover saved/CLI legacy behavior, inheritance, flag-like string values,
all base classes, saved/new/legacy CLI precedence, invalid/overflow values,
and actual real-time permission/readback outcomes. A real-time check accepts
either the exact granted class or a reported refusal with the prior class
preserved; it does not call High a successful real-time request.

With an older package, omit --core and add --expected-missing-saved-high.
That seven-case paired negative must specifically reproduce saved high being
ignored and a flag-like string falsely raising priority; other legacy cases
must pass. The probe's class observations are independent Windows readback.

See Microsoft's [SetPriorityClass documentation](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setpriorityclass)
and [scheduling class definitions](https://learn.microsoft.com/en-us/windows/win32/procthread/scheduling-priorities).
These checks do not establish Qt checked persistence, the Performance page,
heavy playback under elevated scheduling, real-time permission on other
accounts, Windows 11 or complete beta acceptance.
