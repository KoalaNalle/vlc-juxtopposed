# Windows Audio settings activation — 19 October 2026

Scoped Windows 11 acceptance of the actual Output device and fixed startup-volume
editors. MainCtx now links the separately tested Windows-device consumer.
Receiver passthrough remains independently unavailable. The fixed-policy radio
also requires the startup-volume consumer; an unmanaged output cannot select a
policy whose number/consumer is unavailable. Remember remains independent.

## Evidence

- [Component replay](audio-component.json):20 groups pass. The desired initial
  unsupported-provider assertion failed against the previous QML, then passes
  after the fixed-radio capability repair. These component/layout scales are
  not Windows-DPI or actual playback proof.
- [Compilation](compilation-r1.json) and [provenance](provenance.json): ordinary
  GUI `3857fa66519cc84299ee60f7cb066b89c3335f427c59df4ef711fa86e74e3d83`,
  private GUI `de7506005fca99b0504b1e1fff8325f8d726ace50b7fb57f2a8205a06dba4bb7`.
  Production1212/private-run38 inputs matched the files at acceptance. Core, repaired FLAC
  packetizer/demuxer, MMDevice, WASAPI and media library retain their recorded
  identities. The compilation receipt's pending status describes compilation
  time; the actual-app/ordinary receipts below establish subsequent acceptance.
- [Actual app](actual-app-r2.json): five startup-inspected monitor2 processes,
  stale31/paused83/cold-reopen59/playing95/unmanaged22 =290 checks, all normal
  exits, no forced cleanup, no Qt diagnostics or unexpected native errors.
  Paused/reopen/playing logs independently require real `mmdevice` and `wasapi`
  selection, decoded audio and played buffers from owned stereo48k silence.
- Actual editing covers enumerated endpoint/System default, prepared numeric
  defaults, invalid126%, fixed/Remember policy, genuine locked-save rollback and
  retry, dialog reopen and cold explicit-device65% consumption. Restoring cold
  defaults clears restart feedback. A settled paused clock is exact; playing
  time is bounded against elapsed wall time. Input, audio-output owner, current
  device, volume, mute and rate remain unchanged during saved-default editing.
- [Ordinary Windows input](ordinary-windows-r1.json): no private hooks. Actual
  Ctrl+P, pointer/keyboard typing55%, fixed/Remember/Tab/Space, device popup
  Down/Return, Escape/reopen and Ctrl+Q. Independent config readback confirms
  fixed0.55, Remember Off and the explicit endpoint. Visible paused00:00 and
  volume remain unchanged; normal exit, no Qt/native errors. The one retained
  mux warning is normal final-input removal (`src/stream_output/stream_output.c`),
  consistent with the earlier settings warning review.

## Negative controls and limits

Three intentional native errors are retained: one invalid synthetic saved
endpoint and one config-sharing failure in each of paused/playing. The first
runner rejected the endpoint error despite31 passed UI checks. R2 classifies
exactly that one message only with the two named passed negative-control checks;
other errors or duplicate matches still fail. The compiled binaries are
unchanged. The RAII config-sharing lock is present in the compiled private
object. Earlier rejected artifacts remain in the ignored work folder.

No physical unplug, receiver passthrough, audible signal, Windows10, screen-reader,
complete visual/DPI, all-settings or final-package acceptance is claimed. Real
endpoint IDs/friendly labels and hardware-bearing captures stay in ignored
artifacts; public receipts use the endpoint hash. Generated media and copied
monitor geometry are unchanged. Earlier R4 settings/fullscreen and R7 tooltip/
video results retain their exact binary scope rather than being re-labelled as
results on this GUI.

Local runtime: `build-windows/beta-audio-device-activation-20261019/ordinary-runtime-r1`.
Source checkpoint: `source-r1`; runner checkpoint: `run-source-r2`. No new commit,
push, release, dependency installation or security-setting change accompanies
this activation. The full beta roadmap remains active.
