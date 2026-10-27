# Popup duration whole-row keyboard reveal — 21 October 2026

The slider remains the actual focus owner, while its whole row supplies scroll
bounds. This keeps the title, scale labels and timing note visible together,
including after feedback changes the host layout. Other controls retain their
existing reveal behavior.

The new viewport assertion fails on the pre-repair component (y847..941 versus
viewport y66..881), then passes all12 component cases on the repaired QML,
including640x420 and component scaling150/200%. This is not OS-DPI evidence.

The matching ordinary/private Windows builds pass seven native processes/313
checks with normal exits and no unexpected native/Qt errors; the deliberate
single locked-save error is retained. All three editor cases measure the full
row against the actual scrolling viewport. Existing real notification delivery,
restart boundaries, dependency/policy and active-media preservation checks pass.

One ordinary Windows session verifies genuine Tab navigation from a completely
offscreen row, complete title/ticks/note visibility, then Left777→277→0 with
restart feedback and paused primary00:00. Normal Ctrl+Q exit, independent config
readback and genuine Windows777ms hint pass; no private hook is present.

Review of the fresh native capture confirms the prior clipping defect is closed
in that capture. The minor title alignment refinement, native narrow/Light full
page, actual OS-DPI, concurrent settings persistence and final package remain
open. The launcher first rejected a nonexistent geometry fixture before starting
any process; it was corrected to the unchanged R3 fixture, with the rejected
preflight retained locally. Strict guards/error classifications were unchanged.

`verification.json` binds input manifests, compilation, native, ordinary,
component and negative-control receipts and the actual native capture. Ordinary
GUI `a56a6f456b82…`, private `7290c44fe050…`, frozen source-r4 under
`build-windows/beta-popup-duration-20261020`. Core/audio/library identities remain
unchanged. No complete beta acceptance is claimed.
