# Windows helper startup and resume checkpoint

Recorded 14 October 2026. The incremental core and Qt targets build successfully. The revised core and Qt plugin are staged in `build-windows/slice-package`; the user's preceding `build-windows/package` is retained.

## Concurrent helper creation

Windows named pipes use a global namespace. VLC's pipe emulation used `AnonPipe_<counter>`, and every process started its counter at zero. Another player or the preparser helper could therefore collide with an existing pipe and report Windows error 231. The failed fresh player launch was a confirmed pipe-name collision, not a missing helper DLL.

Pipe names now include the process ID and the full 64-bit counter, using bounded formatting. [win32_pipe.c](../../../src/test/win32_pipe.c) holds pipes open in the parent and creates the same numbered pipes in a simultaneous child. The original core fails with error 231 while another instance holds those names. The revised core passes with multiple VLC instances open. The test is registered for desktop Windows in both build systems.

A fresh traditional-control launch with the revised pipe code opens the actual color fixture. Paused pointer seeks show blue and green frames, then playback resumes and reaches end-of-media. Its log has no helper-creation failures or PCR reset-loop errors. The Python LibVLC test now points `VLC_LIBEXEC_PATH` at the packaged helper executable because Python lives outside the package.

## Resume after a paused seek

A stricter video-only check found another output-clock fault: resetting while paused discarded the pause date, even when buffering subsequently created a reference against that frozen date. Resume could not shift the new reference, skipped ahead and dropped a burst of late frames. The reset now retains the date while paused; normal unpaused resets keep their existing behavior.

- The new `test_clock_paused_seek` in `test/src/clock/clock_start.c` simulates 60 seconds without sleeping. Before the change it schedules the next frame at 140,000µs instead of 60,140,000µs and exits 3. Afterward it returns the expected timestamp and the full clock-start test exits 0.
- ES-out, input-clock and concurrent-pipe regressions exit 0 with the revised core.
- The strengthened [native LibVLC test](../../../test/libvlc/native_paused_seek.py) verifies forward/backward paused frames, frozen time, resume without skipping, and actual red then blue resumed frames. The preceding core fails with “Resume skipped ahead past the target.” The revised core passes with no late-frame burst or PCR-reset loop. [Results](resume-color-fixture-results.json).

Tested revised core SHA-256: `1d3c3b8e9794d512fd0abd8e9d4bc86a7d892a9a1cc8b6cb3447908875bc9463`. Local detailed logs: `output-clock-start-before.stderr.log`, `output-clock-start-after.stderr.log`, `seek-regression-with-resume.log`, `input-clock-with-resume.log`, `pipe-fixed-gui.log`, and `seek-verification/api-resume-before` / `api-resume-final` under ignored `build-windows` output.

The common LibVLC test header now avoids unavailable Windows `alarm()`/`setenv()` functions. Windows test runs use an external timeout. The tracer-dependent `test_src_clock_clock` executable builds but cannot find its builtin tracer in this dynamic Windows configuration; it is not counted as a runtime pass. The independent clock-start test above does run.

## Qt dialog paths and remaining native gates

`WindowDialog.qml` qualifies Basic `DialogButtonBox`, `ToolbarEditorDialog.qml` qualifies Basic `ItemDelegate`, and the search popup anchors to its known icon button while its parent is being constructed. The Qt target rebuild exits 0 and all six existing component suites pass. These suites do not exercise the entire native toolbar editor.

The new media-library launch renders the shell and initializes the library, then Windows presents a Firewall permission prompt. Native input checks wait for the user to dismiss that system prompt; no Firewall permissions have been changed. Toolbar-editor interaction, complete cold/warm launch/shutdown and the final AV1/traditional-control pass with this latest core remain pending. Milestone 1 is still open. The wider design/native/DPI/visual gates remain in the coverage ledger.
