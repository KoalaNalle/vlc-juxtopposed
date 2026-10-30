# Playback settings dismissal — 29 October 2026

The actual ordinary Windows 11 player passed the repaired popup's input-ownership
checks. Dismissing settings with an outside click preserves both paused and
playing state. A later video click works normally, Escape preserves the paused
clock, and reopening permits subtitle-tab interaction. The VLC child exited
normally with 603 modules, no captured error diagnostics and every preservation
guard satisfied. This is one generated fixture/profile and window topology,
not full design, DPI, multi-instance or package acceptance.

The production change makes `PlaybackSettingsMenu.qml` modal without dimming.
The maintained component test fails on the previous source at
`outside-click propagated to playback` and passes after the repair. That earlier
component observation is preserved verbatim in `verification.json`: its
`nativeBinaryIncludesRepair: false` and pending-native wording describe the
earlier component stage, not this later ordinary run. The new native build and
ordinary observations establish the integration separately.

## Evidence and identities

- [Verification](verification.json) binds four relevant sources, five runtime
  DLLs, the build manifest, compiler closure and terminal process checks.
- [Observations](observations/) contains eight accessibility states and their
  target-window images, from paused startup through dismissal/reopen checks.
- [Build plans](build/) record QML cache/resource generation, the four selected
  compilations, player archive/link and restoration. The builder's frozen
  `built-not-desktop-qualified` status predates these ordinary observations.
- [Copy verification](copy-verification.json) binds every copied evidence file.
  [Producers](producers/) preserve the launcher and receipt collector.

The Qt DLL is `7c7db5cb675edc20b3366120dc55282b09d2db2dc731b7c6ed8c1815e0706624`.
Core remains `f09c45fe4075a02836807f00cab03163cdba1ec79e37ba35a0328f7cb6d5462d`,
WebVTT `38eec724ba220769389ba0aac6fb8f2cd56306f32ccff7dba3763674ce6dba1d`.
The build qualified 937 unique actual compiler inputs, restored the original
shared Qt/objects/resources/archive and preserved the full tested fallback.

Local one-use build: `build-windows/beta-record-feedback-20261028/ordinary-build-r9-20261029/`.
Local ordinary run: `build-windows/beta-resume-20261029/gui-r9/`.
No recording was made by this run. Record-off during repeat and recorded WebVTT
replay remain separate unfinished checks; this receipt supplies no result for them.
