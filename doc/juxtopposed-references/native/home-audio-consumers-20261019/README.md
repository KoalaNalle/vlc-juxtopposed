# Audio consumers and live volume-step feedback — 19 October 2026

Later Audio on/off verification confirms that this is a startup policy for a
reopened player. Its old "next media" hint contradicted the restart note; the
hint now says "Include audio when playing media." The actual Audio component
passes [19 groups](component-r6.json), retaining all earlier checks. The current
ordinary GUI compiles as
`45cee48bab4ad02effe13c9c9487a0078c2df9e7fffb66d73bf574d1ff4e1ebc`;
[current compilation/source record](compilation-r6.json), local `source-r6`.
This is still compilation/component evidence, with desktop acceptance pending.
The R5 records below retain their earlier identities and scopes.

**Scoped native and component verification passes. The ordinary app compiles;
its playback and desktop journeys are pending. This is not beta acceptance.**

The Audio consumer audit found that the linked authored volume-step setting is
live: the slider reads its model value, and Core volume actions inherit the
policy on each invocation. The Audio page nevertheless described every supported
field as a startup default and counted this field in its restart warning.

The model now excludes `volume-step` from the restart comparison only when its
live consumer is linked. The page then says the change applies immediately.
Other Audio defaults retain their restart behavior, and an unavailable volume
step continues to say it is unavailable. The ordinary MainCtx consumer gates
remain disabled until their actual app interaction checks are accepted.

## Evidence and limits

- [Windows consumers](windows-consumers.json): four passing QtTest cases,
  including fixture setup/cleanup, normal exit and no Qt diagnostics. The actual
  preference writer saves an enumerated active endpoint, 40% startup volume and
  a 2.5-point step. A fresh native Core playlist opens real MMDevice/WASAPI,
  decodes and plays the generated silent WAV, and reports the chosen endpoint
  and volume. While paused, saving 65% leaves the current volume, input and clock
  unchanged. Native step actions consume both 2.5-point and changed 5-point
  policies; reopening applies 65%. Two real WASAPI output sessions are logged.
  This establishes backend behavior, not pointer/keyboard UI or physical audibility.
- [Paired comparison](comparison.json) and [negative control](negative-control.json):
  the identical desired test object links to the original and repaired model
  objects. The original returns the expected test failure for its false restart
  warning; the repaired model passes all four cases. Both complete the real
  Windows output case. The negative exits with the ordinary test-failure code1,
  without forced cleanup, and is not counted as a passing whole run.
- [Existing Audio defaults and PCM](defaults-pcm.json): 32 cases pass, with normal
  exit and no Qt diagnostics, using the same repaired model object. These cover
  existing validation, persistence, locked-write recovery, filter preservation,
  real saved equalizer PCM and the output visualization callback. The AMEM cases
  are separate from the real WASAPI sessions above.
- [Audio component](component.json): 18 existing/extended groups pass against the
  actual category and shared host with supplied controller contracts. The new
  group distinguishes immediate step help, startup-volume help and unavailable
  state. The fixture now registers the production player preview required by
  the newer shared host. Component scales are not Windows DPI acceptance.

All native runs preserve the checked source runtime binaries and human VLC
configuration. Protected Core, MMDevice and WASAPI identities are recorded in
the receipts. No security setting or dependency was changed.

## Build and remaining work

The earlier R5 ordinary GUI compiled as
`44ab133418c40acb8b0b529c061fc5198efd36c0d0cfd6f2967377bd91e7bb62`.
[Compilation record](compilation.json) pins its unchanged production inputs and
Qt Quick SDK. The local full Qt source checkpoint, native manifests, paired
objects and rejected attempts are under
`build-windows/beta-audio-consumers-20261019/source-r5` and its parent.

The first private fixture build needed the complete compositor type. The R4
restart test then exposed a fixture precondition error: changing startup volume
also chooses MMDevice, which legitimately leaves an output-policy restart
requirement. R5 establishes that managed baseline before comparing volume-step
behavior and repeats the paired before/after check. No assertion was weakened.

The earlier ordinary Favorite app remains behind its human-handled firewall
prompt. No action on that security prompt occurred. Device/step/fixed-volume
desktop journeys, enabling the appropriate MainCtx consumers, the remaining
Audio fields, complete settings audit, ordinary regression and final package
acceptance remain open. The later playlist-origin private GUI has now been
rebuilt against this Audio source; its guarded R2 runner remains unexecuted
while the human firewall question is pending. No commit,
push or publication occurred.

## Later timing repair and compilation

The combined Playback/Audio console fixture establishes that `audio-desync` is
read by each new native input. Saving125ms preserves the paused current input,
clock and0ms offset; the same player's next input and a reopened instance use
125ms. The model now excludes that next-input policy from restart feedback, and
the Audio help states the same timing. R12 reproduces the false restart warning;
R13 reuses its exact desired test object with the repaired model and passes all
thirteen cases. The unchanged Audio defaults/PCM32 and real Windows Audio4 also
pass against the repaired model, with normal exits and no Qt diagnostics.

[Audio component R7](component-r7.json) passes20 groups, including the corrected
Audio on/off, immediate-step, next-input-delay and restart-only distinctions.
[Ordinary compilation R7](compilation-r7.json) identifies GUI
`6d655265b8d1de0e4fd3405335581bf4e8383e49902971fe66f62a3e79598008`.
The full Qt source checkpoint is `source-r7`. Matching private origin UI R4
compiled as `58d6381bb28f7d4a50c0506f6aba5ed48a830b27c3b9500fb4f68504744a3e3a`.
Neither app build has run its startup/playback/UI acceptance while the human
firewall question is pending. Later fullscreen-consumer source is a separate
candidate. Protected runtime identities and the scope boundaries above remain.
