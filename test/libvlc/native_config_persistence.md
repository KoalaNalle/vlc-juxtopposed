# Shared native configuration persistence

The opt-in Windows console suite starts separate real LibVLC/Core processes.
It uses owned configuration files and a console command probe; it does not
operate the desktop or edit the user's settings. Run after building the matching
Core and ordinary Windows runtime. Keep generated files in a fresh directory
under this checkout's ignored `build-windows` area.

Build the probe with the configured MinGW toolchain in WSL (`build` is the
configured build root, `repo` the source checkout and `freshOutput` an owned
output directory):

~~~sh
mkdir -p "$freshOutput"
x86_64-w64-mingw32-gcc -std=gnu11 -DHAVE_CONFIG_H \
  -I"$build/win64" -I"$build/win64/include" -I"$repo" \
  -I"$repo/include" -I"$repo/src" \
  -I"$build/contrib/x86_64-w64-mingw32/include" \
  -Wall -Wextra -g -static-libgcc \
  "$repo/test/src/config/persistence_probe.c" \
  -L"$build/win64/lib/.libs" -lvlc \
  -L"$build/win64/src/.libs" -lvlccore \
  -o "$freshOutput/persistence-probe.exe"
~~~

Run from Windows using native Python and Windows paths. All four path arguments
must resolve inside this checkout's `build-windows` directory; output must be
fresh. The source package remains unchanged. Staging unlinks the Core hard link
before installing the explicitly selected test DLL.

~~~powershell
python test/libvlc/native_config_persistence.py --package $package --core $core `
  --probe $probe --output-dir $freshOutput --overlap-iterations 200
~~~

The suite checks unrelated-key preservation, same-key last successful commit,
both closing orders, acknowledged saves/no-op exits, unchanged live values,
mixed string/float/integer transactions, invalid batches, rollback/retry and
previous pending edits. It also covers first-file creation, resets, CLI overrides,
duplicate lines, BOM/CRLF, unknown long Unicode content and an unterminated final
line. Native errors are matched to exact deliberate failure cases; unexpected
errors fail the run. The larger overlap run inspects the real writer-lock events
for a common file identity and nonoverlapping critical sections.

The previous pending string is checked in memory during rollback; independent
string pending-exit coverage is not claimed. Windows replacement retries retain
the same lock and temporary, apply only to errors5/32, and stop after10 additional
attempts with10ms sleeps. Requested sleep time is not a wall-clock deadline.

The [21 October receipt](../../doc/juxtopposed-references/native/home-settings-concurrency-20261021/README.md)
binds Core/probe/source identities, rejected candidates, the independent Win32
replacement reproduction and the accepted38-check/27-process run with600
concurrent commits. It separately records Qt backend tests, selected actual GUI
journeys and ordinary two-window tests. Console success alone does not qualify
shared Qt profiles/geometry, simultaneous playback, physical audio routing,
Windows10, visual/DPI behavior or the final beta package.
