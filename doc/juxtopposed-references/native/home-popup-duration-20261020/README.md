# Popup duration progress — 20 October 2026

The [baseline receipt](baseline-777.json) proves a genuine track-change
notification reaches Qt's Windows platform tray handler with the saved777ms
argument. This uses ordinary checkpoint GUI `56a4a7620f15…`, generated silent
audio, actual Windows output and a fresh isolated profile. The relevant current
tray/MainCtx source identities match the receipt. Computer Use inspected paused
00:00 playback on monitor2 and closed the app normally through Ctrl+Q. No Qt
diagnostics or native errors occurred; no private hooks or raw screenshots.

This establishes the actual platform delivery argument, **not** OS notification
visibility, precise expiration, the settings editor or a final-package result.
The popup-duration setting remains gated. No Windows notification preference,
firewall or privacy setting was changed.

Next: actual editor keyboard/pointer input, invalid values, save failure/retry,
persistence/reopen/cold startup, zero and10000ms boundaries, current-versus-saved
policy and tray/track-popup dependencies. Clarify that zero requests the system
default and Windows controls display timing. Keep the authored slider and native
notifications rather than introducing an independent app timer.
