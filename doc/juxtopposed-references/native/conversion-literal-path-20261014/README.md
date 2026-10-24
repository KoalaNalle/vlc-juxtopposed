# Native literal conversion paths — 14 October 2026

The repaired controller successfully converts into a legal percent folder even
when its owned parent inherits sout-file-format=true. The same eleven-case
test and strict helper were used before and after the one-line production
change. The baseline passes the original ten cases and fails the new literal
path case. The repaired build passes all eleven cases and exits zero.

| Variant | Actual native result | Evidence |
| --- | --- | --- |
| Corrected baseline | 10 passed, 1 failed; exit 1; 3251ms | results/before-results.txt and before-verification.json |
| Repaired controller | 11 passed, 0 failed, 0 skipped, 0 blacklisted; exit 0; 4453ms | results/after-results.txt and after-verification.json |

The original file access chain explicitly disabled overwrite but inherited its
format setting. config_ChainParse creates that bool with VLC_VAR_DOINHERIT, and
the file sink calls vlc_strftime on the whole staging path when it is true.
src/text/strings.c uses local time and native ISO C strftime: %% becomes a
literal percent and %Y becomes the year. The baseline native log at lines
924–938 records the literal 100%% %Y path rewritten as literal 100% 2026, then
the sink cannot create the file in the rewritten random staging folder. The
new Succeeded assertion fails at test line 271. This is an observed native
path failure, not an assumed Windows shell expansion.

source/production.patch adds access.option("no-format") in the common chain
builder. Both encode and validation paths retain no-overwrite, native quoting
and atomic no-replace publication. The repaired native log records both file
chains using no-overwrite,no-format and opening the literal folder: encode at
line 929 and validation at line 1097. Full sanitized logs and line-numbered
excerpts are retained under results/.

The new case uses an independently owned real VLC preference parent with the
format bool true, constructed before the actual controller so it outlives
worker cleanup. The fixture makes a literal percent folder and a separate
formatted-year sibling containing a sentinel. It requires the real Succeeded
state, exact literal output URL, FLAC magic, unchanged source/sentinel hashes
and inherited preference, no staging directory or sibling artifact, then
ordinary published-file LibVLC playback. That playback requires more than two
FL32/44100/mono callbacks, stopped state, 88200 +/- 100 samples and peak between
0.3 and 0.5. This is a native sample-count/amplitude gate; exact PCM equality is
not claimed. The literal output and raw PCM were not separately retained.

Removing the added slot and QDateTime include reconstructs the original test
source byte for byte. The remaining cases preserve validation/no-overwrite,
real FLAC/playback, inherited input/runtime choices, corruption, destination
race, running cancellation/reuse, primary queue/paused clock and worker shutdown.
The helper requires all eleven ordered PASS names, one exact 11/0/0/0 totals
line, no rejected status, exit zero and its existing module/core/source/output
gates. Both runs have fresh userdata, an exclusive selected VLC_LIB_PATH,
cleared inherited VLC_PLUGIN_PATH and the same owned 80-second process bound.

build/ contains the actual source phase receipts, build script and MOC/CXX/link
logs. The corrected baseline rebuilt the test MOC, C++ test and old controller;
the repaired build reused that unchanged test object and compiled only the new
controller before linking. Root reported successful build exits and preserved
the saved executable identities. Existing missing-field-initializer warnings
and libtool host-path diagnostics followed by Continuing are retained. Native
execution is established separately by the strict reports. Automake was built;
this proof does not establish a Meson build.

history/ preserves an earlier setup mistake. Filename endswith routing also
matched test_conversion_controller.cpp and skipped the new C++ slot. Its
printed source receipt exposed the mismatch before any native execution. Its
successful build and old test hash are setup history only, with no accepted
baseline result. The corrected integration receipt and repair script preserve
the fix. The pre-build proposal manifest is historical and accurately records
that no native run had occurred when that proposal was frozen.

source-runtime-manifest.json records the source transition, unchanged test and
helper, saved executable hashes and identical core/plugin identities. Actual
selected module names in the log resolve to the exclusive package inventory;
no live process memory-map proof is claimed. The retained source.wav and
converted.flac hashes belong to the unchanged realFlacAndActualPlayback case,
not the new percent-folder output. They are synthetic evidence, and the media
files themselves are not copied here.

evidence-index.json pairs every original ignored file hash with its separately
hashed publishable copy. Profile, checkout and WSL prefixes use generic labels;
the literal percent text, source/runtime identities and original line numbers
are preserved. No executable, DLL, userdata or media is included in the bundle.

This accepts the scoped native controller repair. A fresh packaged GUI check
was pending when this native bundle was frozen. The conversion form remains
a functional shared-style adaptation because no original Figma form was found.
Broader video/format/profile completeness, full picker/provider/window lifetime
and original-design fidelity remain separate. The previously retained desktop
1323000-frame exact PCM proof also remains a different fixture and result.

## Reproduction

Use Windows x64 Python 3.11 or newer and the existing configured MinGW x64
Qt 6.8.3 build. Keep the exact eleven-case C++ source and helper identities in
the manifest. Apply only fixtures.patch to the preserved base for the negative
baseline, compile the unchanged old controller with the new test, save the
executable, then apply production.patch and rebuild the controller with the
same fixture bytes. Do not weaken assertions or count a stale object as a new
build. build/executed-build.sh.txt is the actual owner-run build sequence with
generic path labels; adapt paths before executing it.

From the checkout root in PowerShell, with the exact runtime and saved
executables already present, use fresh output folders:

```powershell
$conversionCheckout = (Resolve-Path .).Path
python "$conversionCheckout\test\libvlc\native_conversion_controller.py" `
  --package "$conversionCheckout\build-windows\slice-home-route-package-20261014" `
  --test-exe "$conversionCheckout\build-windows\literal-conversion-before-corrected-test.exe" `
  --output-dir "$conversionCheckout\build-windows\literal-before-FRESH"
python "$conversionCheckout\test\libvlc\native_conversion_controller.py" `
  --package "$conversionCheckout\build-windows\slice-home-route-package-20261014" `
  --test-exe "$conversionCheckout\build-windows\literal-conversion-after-test.exe" `
  --output-dir "$conversionCheckout\build-windows\literal-after-FRESH"
```

The baseline must fail only the new literal-path case; it is intentional
negative evidence. The fixed run must pass the unchanged strict eleven-case
gate. New source/runtime identities produce new evidence and do not extend
these frozen results.
