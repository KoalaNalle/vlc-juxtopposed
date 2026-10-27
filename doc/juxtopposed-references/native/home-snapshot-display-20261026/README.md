# Separate snapshot export and subtitle display - 26 October 2026

Core `8dc69606` retains hardware subtitle display while exporting a separate
software image. It avoids the unsupported GPU software-blend attempt; the same
native hardware suite drops from 104 such messages to zero. Hardware/software
each pass all 29 subtitle functions with normal exits and unchanged guards.

Actual windowed and matched primary-screen fullscreen snapshots preserve the
caption. The three candidate PNGs are pixel-identical 960x540 images with exact
frame marker 0 and 2597 caption pixels. The ordinary candidate processes exit 0
with 602 modules and no error diagnostics.

Secondary-screen idle capture remains unresolved. The earlier Core reproduces
caption loss before any snapshot is taken, so this observation is separate from
the new export path. Screen coordinates, the initial confounded comparison,
incomplete fullscreen input attempts, responsive thread evidence and diagnostic
helper cleanup are explicit in [checkpoint.json](checkpoint.json). No physical
display, OS-DPI, source-orientation, full visual or final-package claim is made.

Generated captures and structured evidence are compressed with exact sources and
producers. Raw streams remain local with hashes. This is a scoped development
checkpoint; continue from [STATUS](../../../juxtopposed/STATUS.md).
