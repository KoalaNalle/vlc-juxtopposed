# Compact subtitles and cold-start regression - 26 October 2026

The actual602x421 player now shows the full wrapped failure notice and accessible
error details, and supports paused replacement recovery. Fixed redesigned
windowed controls reserve video space so the compact two-row bar no longer
covers captions. Paused seek, playback, fullscreen/return, explicit auto-hide
policies and a traditional-controls route have separately scoped observations.

The new native cold-origin test is deliberately still failing with D3D11.
Software passes six startup cycles. Hardware fails both registration orders,
including an older Core; a second capture without seeking also has no caption.
Selection can precede a valid current clock. The failed fallback attempts were
removed, exact production source restored, and diagnostic runtimes excluded.
The wider hardware suite also fails two paused-time preservation assertions
in attachment and replacement journeys. Their software cases pass; precise
hardware clock/frame deltas remain to isolate.

[checkpoint.json](checkpoint.json) binds all before/after captures, runtime hashes,
native results, setup corrections and remaining issues. The first hardware
snapshot includes a caption but retains blend/preview diagnostics. Older scoped
passes do not establish final-package, OS-DPI or full beta acceptance.
Continue from [STATUS](../../../juxtopposed/STATUS.md).
