# Standalone production-callback test recipe

Use the established configured Windows x64 build and compatible Core import
library/runtime. No new dependency or shared-tree `make` is needed. Choose a
fresh ignored output directory. The recorded commands and exact flags are in
`actual-compile-commands.json`; adapt paths to the verified current environment.

The maintained test is `test/modules/demux/webvtt_fps.c`, with its five fixtures
under `test/modules/demux/fixtures/webvtt-fps/`. It directly includes the actual
demux implementation and links the unchanged production WebVTT text parser.
Only its ES callbacks are supplied by the test. Decoder/encoder/muxer descriptor
callbacks are deliberately unreachable failure sentinels.

For a MinGW cross build, use arrays/structured arguments equivalent to:

```sh
# Set these to verified absolute paths; OUT must be a fresh owned directory.
CC=x86_64-w64-mingw32-gcc
SRC=/absolute/source/root
BUILD=/absolute/configured/win64
CONTRIB=/absolute/configured/contrib/x86_64-w64-mingw32
OUT=/absolute/ignored/fresh-output
SSP=/absolute/existing/gcc/libssp.a

flags=(-std=gnu11 -DHAVE_CONFIG_H=1 -DMODULE_NAME=webvtt_fps_test
  -D__MSVCRT_VERSION__=0x700 -D_WIN32_WINNT=0x0601 -DWINVER=0x0601
  -D__USE_MINGW_ANSI_STDIO=1
  -I"$BUILD" -I"$SRC/include" -I"$BUILD/include" -I"$CONTRIB/include"
  -I"$SRC/modules/demux" -I"$SRC/compat/stdbit" -I"$SRC/compat/stdckdint"
  -O1 -g -mms-bitfields -Wall -Wextra
  -Werror=implicit-function-declaration -Werror=incompatible-pointer-types
  -fstack-protector-strong -fdata-sections -ffunction-sections -static-libgcc)

"$CC" "${flags[@]}" -MMD -MF "$OUT/parser.d" -c \
  "$SRC/modules/codec/webvtt/webvtt.c" -o "$OUT/parser.o"
"$CC" "${flags[@]}" -MMD -MF "$OUT/test.d" -c \
  "$SRC/test/modules/demux/webvtt_fps.c" -o "$OUT/test.o"
"$CC" "$OUT/test.o" "$OUT/parser.o" \
  "$BUILD/src/.libs/libvlccore.dll.a" "$BUILD/compat/.libs/libcompat.a" "$SSP" \
  -static-libgcc -Wl,--gc-sections,--nxcompat,--dynamicbase,--high-entropy-va \
  -o "$OUT/test_webvtt_fps.exe"
```

For the original negative control, compile the same test with a single additional
argument whose literal value is
`-DWEBVTT_DEMUX_SOURCE="/absolute/frozen/preimage/webvtt.c"`.
The demux include search directory above resolves that saved source's unchanged
relative parser header. Use a separate output directory and retain its failures.

On Windows, stage the resulting EXE beside the matching preserved
`libvlccore.dll` in a fresh private directory; the recorded runtime contained only
these two files. Use a hardlink for an immutable matching Core on the same NTFS
volume, and never overwrite an existing runtime's linked DLL. Run the EXE with
the absolute Windows fixture directory as its sole argument.

The recorded runner sets Windows error mode, bounds each owned child to30s,
retains stdout/stderr and confirms no child remains. Candidate acceptance requires
eight case records, zero failed checks and exit0. The original negative control
must fail the refill/rate cases while passing stream/invalid-rate controls; an
unexpected crash or timeout is not an accepted negative control.

Freeze sources, fixtures, compiler, headers, import/static libraries and Core
before running. Match every generated `.d` dependency against that frozen map,
then verify inputs/runtime identities afterward. Avoid cross-invocation raw
DrvFS device-ID assumptions. Keep source identity, packet-callback proof and
future real decoder/pixel/plugin tests as distinct scopes.
