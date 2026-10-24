# Native Performance media and window checks

Use the travel computer's E-only Common.ps1 environment. Run one native workload
at a time, after compilation finishes. All runtimes and outputs must be owned
portable directories on the same volume. Each output directory must be new.

## Actual media consumers

Run native_performance_consumers.py with --package, --video-fixture and
--output-dir. The video fixture must be a short owned synthetic video. Optional
--core, --avcodec and --postproc replace only the new runtime's detached links;
the source package remains unchanged. The real modules must include the
diagnostics in src/input/input.c, src/video_output/video_output.c,
modules/codec/avcodec/video.c and modules/video_filter/postproc.c.

The driver creates a small H.264 transport stream using VLC's x264 module,
then runs eleven bounded cases with actual decoded PNG output and normal exits:

- All five H.264 loop-filter policies and explicit/automatic decoding threads,
  including the native cap on a requested 32 threads.
- Clock synchronisation, jitter in milliseconds converted to microseconds,
  native cache-scaled reference averaging, and frame-dropping ON/OFF.
- Inline postproc quality taking precedence over its saved global quality.
- Explicit avcodec selection and normal fallback for incompatible/missing
  preferred decoders.

MPEG-TS can reopen its decoder when SPS/PPS supplies the complete format. The
driver requires every selected video decoder to be avcodec and checks every
observed policy value, rather than assuming exactly one decoder creation.

Optional --hardware-sample adds three short read-only Direct3D 11 cases:
Automatic, selected native device, and Off while retaining that device. The
travel machine uses the native module name d3d11_filters. These cases require
actual D3D11VA for enabled hardware decoding, decoded frames, normal exit and
unchanged source size/modification time. They are specific to compatible media
and this machine; they do not establish a hardware matrix. Keep the sample's raw
logs, paths and frame captures private.

## Actual owned settings window

Read the configured Qt Makefile and build the ordinary GUI serially. Add
modules/gui/qt/tests/native_ui/performance_journey.mk after that Makefile, with
QA_OUT set to a fresh absolute private output directory. Run owned-native-ui
with -j1. This opt-in recipe adds one QtTest translation unit to the ordinary
objects and strips debug sections for the travel HDD. Never install its DLL or
add its translation unit to ordinary production sources.

Run modules/gui/qt/tests/native_ui/run_performance_journey.py with --package
<owned-ordinary-runtime>, --qa-gui <private-output>/.libs/libqt_plugin.dll and
--output-dir <fresh-output>. The runner explicitly enables Qt regex JIT and
creates isolated portable data on E:. All pointer/key events and captures
target only this test program's own QQuickWindow.

The real MainCtx, compiled page and native services perform 51 editing checks
and 14 restart checks, including pointer-target assertions. These are not 65
distinct features. A Windows file lock rejects a priority edit and restores
the previous native class; retry, actual rescan, Advanced disclosure and saved
values across restart are covered. Only this owned process changes scheduling,
and it restores its initial class before other GUI work. Its settings popup
stays open at normal quit to cover actual shutdown lifetime behavior. Timeout,
forced cleanup or an abnormal exit fails even if all UI assertions passed.

The ordinary GUI must remain unchanged and contain no fixture. Component
Dark/Light/narrow/150% captures are additional presentation checks, not Windows
DPI or independent visual acceptance. See the
[checked page proof](../../doc/juxtopposed-references/native/guest-performance-page-20261017/README.md)
for exact identities, retained failures and remaining beta gates.
