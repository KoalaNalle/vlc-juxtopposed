# Windows Library startup policy — 16 October 2026 guest

The real Windows entry point now respects saved media-library OFF. It no longer
injects `--media-library` before user arguments. The existing core default remains
OFF, including the serializer's `#media-library=0` representation. Saved ON and
explicit CLI overrides continue to work. No core/LibVLC default was changed.

The maintained `modules/gui/qt/tests/native_library/startup.py` stages an ordinary
package and an optional rebuilt launcher in a new portable runtime. Each case
has owned config, userdata and logs. It checks actual module loading, database
creation, unchanged config bytes and confirmed normal process exit.

On the retained original launcher, saved OFF and commented default OFF both
incorrectly load the media-library module and create a database. The other three
cases pass. Rebuilding `bin/vlc.exe` from the one-file launcher repair makes all
five cases pass, with zero timeouts or forced cleanup. Saved ON and explicit ON
still initialize the real library; explicit OFF still overrides saved ON.
The maintained nine-case actual sender/receiver process regression also passes
with the rebuilt launcher, zero failures/errors/skips and a confirmed normal
exit; its checked profile is unchanged. See [process.json](process.json).

See [original.json](original.json), [fixed.json](fixed.json) and the source/build
identities in [verification.json](verification.json). The ordinary package was
preserved; the rebuilt launcher is staged under the ignored
`build-windows/beta-runtime-20261016/launcher` directory. Build logs and raw native
logs remain in owned ignored output. The retained toolchain emitted its existing
Windows function-pointer warning and libtool launch-wrapper conversion errors;
the actual linked Windows executable passed the tests.

This is bounded Windows 10 native startup-policy acceptance. It does not accept
the first-run GUI or complete Library/beta surface. In particular, first-run
setup must account for the native OFF default, rather than silently enabling
the library through a command-line override. Full GUI, visual/DPI, live timeline,
other unfinished categories and coherent final package verification remain open.
