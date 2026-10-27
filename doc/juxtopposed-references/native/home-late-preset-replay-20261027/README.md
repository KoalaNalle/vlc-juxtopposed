# Ordinary late-cue preset replay - 27 October 2026

Core `fdfcfcd5`, ordinary GUI `8e92470c` and MP4 `df15e8ed` pass the older
generated late-cue journey at playback speed1.5. At held frame426/17.04s, the
30fps external cue is visible; explicit FPS Reset0 removes it at that same frame.
At normal timing it is absent on frame499 and appears on frame500/20.00s.

Applying Default and the named30fps preset updates actual settings without moving
the held25s picture. The second file uses its own caption and Default0; returning
restores the first file's preset and early cue. A normal cold restart restores
actual30fps, selected external Track3 and the early cue. The saved catalog remains
30fps and the separate native global default remains23.976.

Both apps exit normally with602modules, no error diagnostics and unchanged
source/runtime/fixture/producer/cache guards. [checkpoint.json](checkpoint.json)
binds the generated observations and exports to the exact source and binary
identities. It also preserves stale-tree, filename, slider and inherited-runner
scope qualifications. Raw profiles/logs and binaries remain local.

Broader preset failures, formats, physical display/DPI, the fullscreen idle
capture issue and the beta gates remain open. Continue from
[STATUS](../../../juxtopposed/STATUS.md).
