# Media-library history clear result — 17 October 2026 guest

The native media-library wrapper discarded the boolean returned by the actual
database `clearHistory()` operation and returned success even when that operation
failed. It now propagates `VLC_EGENERIC` on failure. The public control-query
comment is corrected; the API signature and module imports are unchanged.

The **same Windows executable and companion runner**, with only the actual
`libmedialibrary_plugin.dll` changed, reproduce one failing check before repair
and pass all seven checks afterward. An owned SQLite trigger rejects a real
UPDATE in a fresh native media-library database. This is explicit database fault
injection, rather than a mocked media-library result or an actual damaged user
profile. Both processes terminate without a timeout or forced cleanup; the
intentional original test failure exits 1, the repaired test exits 0.

The checked local/network history rows, positions, play counts and favorites
remain unchanged on rejection. The caller receives the error. After removing
the trigger, retry clears the seeded local/stream progress and global history;
favorites, the two-entry named playlist and an explicit playback preference
survive. Repeated empty clear succeeds, and native shutdown/reinitialization
preserves the cleared history and retained data. The fixture requires explicit
userdata and starts only with empty history. This does not claim whole-database
byte identity, power-loss durability, OOM coverage or every database error.
See [native-database-pair.json](native-database-pair.json).

The final tuple passes native Library metadata/opening/process 18/42/9 and
Subtitle preferences 37 with no failures/errors/skips or forced cleanup. Default
redesigned/traditional video/audio and local HLS pass normally with decoded
frames, Direct3D 11/WASAPI and zero QML/write/visual-callback/PCR errors. Fresh
native stages match the ordinary core/GUI/SoXR/media-library identities:
[native-stage-identities.json](native-stage-identities.json).
Core remains `603866…cc4a`, GUI `fe6113…71ea`, SoXR `4934e6…ad16`; media library
is `fd70207d3067a8ce38e1cb9e8a3df6630da78bc39a0eff4402cd08484a916e90`.
See [verification.json](verification.json) for full identities.

The one-job module and native fixture builds pass. The test is explicitly
opt-in in Autotools and Meson because its companion runner controls fresh data,
the rejection trigger and the process protocol. The Python runner is included in
source distributions. Meson syntax parses; no native Meson build was run.
Reproduce with a matching owned portable runtime:

```sh
make -C <configured-build>/modules -j1 libmedialibrary_plugin.la
make -C <configured-build>/test -j1 test_modules_medialibrary_history.exe
```

Stage that test executable, matching DLLs and the compiler's test-only libssp DLL
where required, then run the controlling Windows helper:

```powershell
python test/libvlc/native_history_clear.py --package <owned-runtime> --output-dir <fresh-output>
```

The old and new DLLs, frozen recipes and raw receipts remain under
`build-windows/beta-privacy-20261017`; databases, logs and profiles stay outside
Git. Ordinary baseline and prior accepted runtimes remain preserved. This is an
incremental module checkpoint, not a clean complete beta package.

The [original Privacy reference and nine-field audit](../../figma/20261017/privacy/README.md)
are now archived. This native result is groundwork for a real Privacy page.
Existing GUI clear callers still need busy/error/retry handling and coordination
with Home visits and active/retiring inputs; the other Privacy controls and their
semantics remain explicit open work. No complete Privacy UI, native visual/DPI,
Windows 11, full beta or publication acceptance is claimed.
