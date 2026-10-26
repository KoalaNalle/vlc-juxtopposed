# Hardware subtitle snapshots - 26 October 2026

Regular Core `883f7e51` with GUI `8ef1ade1` repairs the saved-caption omission
on this Windows 11 D3D11VA/D3D11 path. The beta session continues. This receipt
does not establish other hardware, HDR, rotation, DPI or final-package behavior.

The old ordinary snapshot contains no caption pixels even though captions are
visible on screen. The native before probe reproduces the omission with actual
hardware decoding and D3D11 display, while its software positive control and
hardware no-subtitle negative control pass. The software snapshot converter had
received no graphics context and a partial blender format. It now receives the
actual picture's context and full format, with the RGBA output codec identified.
The same three native cases pass after the repair with normal process exits.

The ordinary app starts paused at 1.5x with its first caption visible. Four
actual Shift+S output PNGs contain the expected captions: initial paused FIRST,
MIDDLE after a paused seek to 30 seconds, playing LAST, and paused MIDDLE on the
next embedded-ASS source. Each is 960 x 540 with more than 1,000 bright caption
pixels. The preceding failed ordinary PNG has zero. On-screen captions return
after the snapshot redraw; paused clocks remain held. Quit exits normally with
unchanged source, fixtures, binary/cache guards and 602 staged modules.

Fifteen native subtitle functions, 24 startup/seek/track cases and eleven
delay/boundary cases pass the repaired Core. These scopes remain distinct from
ordinary visual/design acceptance. The first direct hardware alpha-blend attempt
still logs unsupported YUVA/RGBA-to-DX11 messages before its now-working software
fallback. Converter-creation errors are gone from this ordinary run. Standalone
native D3D11 probes log window closure during normal teardown. All these scoped
diagnostics are retained; no error-free logging claim is made.

[checkpoint.json](checkpoint.json) binds full identities, before/after sources,
reports and actual output images. Gzip preserves original bytes. Only generated
owned media was used. Continue from [STATUS](../../../juxtopposed/STATUS.md).
