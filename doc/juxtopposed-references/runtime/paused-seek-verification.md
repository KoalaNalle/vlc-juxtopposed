# Native paused-seek verification

Recorded 14 October 2026 (Helsinki). Source baseline: `7c54e2548b` plus the accompanying input/decoder changes. Qt 6.8.3, Windows x64 GCC 13 debug; incremental core build exited 0. The separate `build-windows/slice-package` contains the tested core. The user's working `build-windows/package` retains its preceding core.

## Faults and changes

Paused seeking used a saved pause date when finishing buffering, then wall time for subsequent PCR updates. The resulting apparent lateness caused repeated clock resets and rebuffering with traditional controls as well as the redesigned controls. PCR updates now use the same frozen date whenever the output is paused, including download rebuffering. Resume retains the existing clock shift.

An additional race occurred when buffering finished before a paused seek's first decoded picture arrived. That picture could miss the existing forced-first-picture path, retain the old visible frame and establish an output clock against wall time while paused. The decoder now includes the acknowledged output pause state in that first-picture condition. Subsequent pictures and normal playback keep their existing behavior.

## Evidence

- The new `test_paused_seek` in `src/input/test/es_out.c` simulates a 60-second pause without sleeping. With the original clock code, a post-buffering PCR was reported 59,600 ms late and the decoder's wait-state assertion failed (exit 3). With the fix, the ES-out test and input-clock test both exit 0. Existing preroll/rate scenarios also pass.
- [Native LibVLC regression](../../../test/libvlc/native_paused_seek.py): packaged VLC plays a red/blue/green fixture, pauses, repeatedly seeks forward and backward, verifies actual snapshot pixels, checks time remains frozen, resumes and stops. Final run passes. This small fixture also passes with the clock fix alone; it does not reproduce the slower AV1 first-picture race.
- Traditional Qt controls with the original core reproduce the reset loop on the supplied 3840 × 2160 AV1 video. The clock fix alone eliminates that loop but leaves a stale paused picture in this native case; resume also exposed a large late-frame burst. These intermediate failures were retained, not accepted.
- With both changes, the redesigned Windows player performs two paused keyboard seeks and a pointer seek. Settled captures show new target pictures while its Play action remains available. Resume changes it to Pause, advances the timeline from about 39.65% to 41.27% and displays subsequent video frames. The log records paused → playing → stopping → stopped, with no PCR lateness/reset-loop or large late-frame-burst errors. Stop returns to Home.

The interface shown inside the supplied walkthrough is video content. These captures establish playback behavior, not implementation of the library or preference screens shown in that video.

[Settled paused target frame](paused-target-frame.png) · [Native color-fixture results](color-fixture-results.json).

Local detailed evidence lives under ignored build output: `build-windows/seek-verification`, `seek-regression-before.log`, `seek-regression-final.log`, `clock-regression-final.log`, `seek-legacy-before-clean.log`, `seek-legacy-after.log`, and `seek-redesign-frame-fix.log`. Tested core SHA-256: `161d8f7f841b3b4fc15e27855ab7bb5ca0908732a3e1d6f1cebba56394d155d3`.

## Repeating the native fixture check

Generate an 18-second MP4 with three six-second color segments, then run the Python regression with a built package and the existing PySide6 test environment:

```sh
ffmpeg -f lavfi -i color=c=red:s=320x180:r=25:d=6 -f lavfi -i color=c=blue:s=320x180:r=25:d=6 -f lavfi -i color=c=lime:s=320x180:r=25:d=6 -filter_complex '[0:v][1:v][2:v]concat=n=3:v=1:a=0[v]' -map '[v]' -c:v mpeg4 -g 1 -q:v 2 paused-seek-colors.mp4
python test/libvlc/native_paused_seek.py --package BUILD_PACKAGE --media paused-seek-colors.mp4 --output-dir SEEK_RESULTS
```

The Python test uses the current LibVLC 4 declarations, including microsecond times and asynchronous stop. It needs a Windows package; the QML-only CI workflow does not build or run it.

## Remaining runtime gates

The subsequent helper failure is now traced to a Windows pipe-name collision and repaired. A stricter video-only check also found and repaired lost output-clock pause state on reset. See [the follow-up checkpoint](windows-helper-and-resume.md) for before/after evidence, the revised core hash and the remaining startup/dialog/AV1 gates. Preserve the preceding AV1 evidence as belonging to its recorded core. Fullscreen, chapter/bookmark lifecycle, DPI and full native visual acceptance belong to subsequent gates.
