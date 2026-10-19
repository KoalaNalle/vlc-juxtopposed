# Fullscreen controls — 19 October 2026

Morning app replay: [four actual-window cases](actual-app-verification.json)
pass138 checks across audio/video and On/Off, with four normal exits, no Qt
diagnostics and no native errors. [Exact provenance](actual-app-provenance.json)
binds ordinary GUI `fd7f8af9b480…`, private GUI `8be8eff6e4d8…` and local
`source-r6`. Hidden controls reject input and return focus to the title controls;
windowed return preserves the paused input/clock/rate/volume/mute. Generated-only
On/Off captures were inspected. This is functional visibility/focus proof, not
video-frame, original-design, screen-reader, OS-DPI or final-package acceptance.

The runner now holds each startup for fresh desktop inspection, without test
input or a timeout while permission handling may be pending. Removing the extra
`VLC_PLUGIN_PATH` avoids loading autorun modules twice: each replay logs601
modules. Native media warnings remain recorded for review. The ordinary settings
acceptance flag remains disabled: its activation and actual editor check are
the next task, deferred when the user requested checkpoint publication/shutdown.

Rejected R2/R3 video fixture attempts are retained. Their visible-only lookup
and hidden-state lock assumption were corrected by locating the existing owner,
sending the normal reveal signal, then acquiring its visibility lock. Production
code was unchanged during these fixture repairs. The following earlier records
retain their original compilation/component scopes.

The existing “Show controls in fullscreen” option now has a consumer in the
embedded main-window player. MainCtx snapshots `qt-fs-controller` at startup,
matching the Interface service's restart contract. The shared toolbar state
machine exposes a separate bottom-controls policy; title/exit controls retain
their existing visibility behavior. Hidden bottom controls reject input during
their fade, and navigation routes avoid them. The option does not add a
controller to detached video windows.

## Scoped results

- [Toolbar policy](toolbar-policy.json): 41 offscreen cases pass with normal
  exit and no reported Qt warnings. The actual production state machine covers
  the existing auto-hide/timer behavior and the fullscreen option combined with
  every auto-hide mode, forced visibility, hover/keyboard activity and visibility
  locks. A supplied MainCtx/Player boundary is used.
- [Interface storage](native-interface.json): 15 console cases pass on Windows
  11, normal exit and no Qt diagnostics. The production service validates types,
  persists and reopens the saved option, retains restart feedback through reload,
  and refuses the option in detached-video mode. Existing failed-save,
  Windows-locked-save rollback/retry and other Interface contracts also pass.
- [Settings component](settings-component.json): ten existing groups pass,
  including the new option's pointer toggle and embedded-video capability
  dependency. The message observer forwards the original worker handler and
  records no Qt messages through process exit. Controllers are supplied fixtures.

MainCtx's ordinary acceptance flag remains disabled until actual app checks
verify the consumer. These results do **not** establish native fullscreen focus,
pointer input, title-bar escape, detached-window behavior, OS DPI, visual fidelity
or final-package acceptance. The [ordinary GUI compilation](compilation.json)
passes against unchanged production inputs, with SHA-256
`af626db9545ccf6f98151dfb38c8be38755899e2894c3c094325b58fd7309380`. No startup/playback/UI run is accepted.

The first native fixture build failed because a QByteArray expression needed
explicit materialization under Qt's string-builder mode. R2 fixes only that
fixture expression and passes. The failed compile is retained separately.

Work and immutable source checkpoint:
`build-windows/beta-fullscreen-controls-20261019/source-r2`. Source/binary hashes
and preservation checks accompany the receipts. All tests leave the existing
desktop VLC/firewall prompt untouched. No Core/audio module, security setting,
dependency or publication changed. Full beta acceptance remains open.

## Prepared current-source app replay

The later [matching GUI pair](gui-pair-compilation.json) also compiles, with
1,212 unchanged production inputs and35 unchanged private inputs:

- Ordinary GUI: `fd7f8af9b480f4e3b1dd8b4df22ae3fe7e58b655d13914b55658f405e6d6346b`.
- Private test GUI: `a1d1a254bda35c6eaabb1268b15ffe37b1bb15a4caf7d2d2e679cece4ed9110e`.

`source-r3` preserves the tracked Qt patch, all untracked Qt source and both
manifests. The new private journey covers fullscreen audio/video with the option
On/Off, input rejection while hidden, focus transfer, windowed return and exact
paused-input preservation. It is **compiled but unexecuted**. Private hooks are
absent from the ordinary GUI. Its cold staged runtime files match that binary;
no application was launched from the staged folder.

The ignored `run-app-checks-r1.py` prepares separate fullscreen, origin and
Favorite groups. Its guard requires the actual human dismissal reply and a
fresh desktop observation before a marker may be created. That earlier prompt
was subsequently dismissed by the human and its absence freshly observed.
R4's app results above supersede the earlier unexecuted status; these earlier
compilation/staging records still do not establish final-package acceptance.
