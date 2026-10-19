# Interface media-name title activation — 19 October 2026

MainCtx now links the existing startup title consumer independently. The setting
controls both the native caption and the redesigned audio/video title strip.
Saving changes requires restart; current and same-instance next media retain
the startup policy. The full beta and the other Interface policies remain open.

- [Compilation](compilation-r1.json) and [provenance](provenance.json): ordinary
  GUI `d40f11aeefdfaac14b5842f539dd9bd644bc364e3d8a4a3dc8ec9fbfeaeea008`,
  private `4e09011ce1ee5d112ca26d453db3b50fb1722cfac982f787c7dc1d4193cc7e5f`.
  MainCtx is the only production input changed since the Audio activation tuple.
  Production1212/private39 inputs match at acceptance; ordinary has no private
  hooks. Core, FLAC, MMDevice, WASAPI and media library stay unchanged.
- [Actual app](actual-app-r1.json): initial52/Off45/restored46 =143 checks,
  three fresh monitor2 startup inspections and three normal exits. No Qt
  diagnostics or unexpected native errors. One genuine locked-config error is
  retained only with its passed rollback/retry assertion. Pointer save, keyboard
  restoration, truthful restart feedback, real dialog reopen and cold persistence
  pass. Editing preserves primary input, clock, volume, rate, mute and caption;
  playing time is bounded against wall time, paused time is settled and exact.
- Actual decoded audio and video captions are checked in On/Off/restored cold
  processes, including same-instance next video after saving the opposite
  default. Reviewed [On](title-on.png) and [Off](title-off.png) generated-media
  captures show `owned-clip.mp4` versus `VLC Media Player`. The restored capture
  also shows the new video name. These are title-strip pixels, not D3D video-image
  acceptance; the Qt capture's black video area establishes nothing about frames.
- [Ordinary Windows input](ordinary-windows-r1.json): real Ctrl+P, pointer title
  Off, Space On/feedback-clear, Off/normal Ctrl+Q, and a cold launch using the
  unchanged config/runtime. Cold strip/host caption hide the media name; the
  actual settings row remains Off without restart feedback. Both processes
  close normally, with no Qt/native errors. The recorded final-input mux warning
  has the previously reviewed removal scope. Native MP4 table/initial-size
  warnings remain in the actual-app receipt; no broader warning claim is made.

The unchanged Interface QML/component and preference-model checks retain their
earlier scopes; this activation changes no layout or model writer. Actual
pointer/keyboard/failure/restart evidence above qualifies the linked control.
Full visual, Windows DPI, accessibility/screen-reader, other Interface policies
and final-package checks are still open. Earlier R4/R7 and Audio activation
results retain their own exact GUI identities. No new dependency, security
change, commit, push or release is part of this work.

Local work/runtime: `build-windows/beta-interface-title-20261019/ordinary-runtime-r1`.
Frozen source and manifests are in that folder's `source-r1`.
