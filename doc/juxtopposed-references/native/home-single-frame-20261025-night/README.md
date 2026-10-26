# Single-frame paused startup repair - 25 October 2026 night session

The session continues toward the beta until 08:00 Helsinki on 26 October.
This receipt records a native checkpoint, not beta or ordinary GUI acceptance.

A diagnostic Core reproduces three failures where a late need-data request
arrives after EOF and bypasses the paused branch. Its fourth replay passes when
that request is absent. The regular Core now keeps paused inputs paused at EOF,
asks codecs to drain before waiting for buffering, and acknowledges their first
picture. No diagnostic logging is present in the accepted regular Core.

Four final single-picture replays pass with a held paused snapshot, frozen clock,
resume/EOF and normal stop. The previous buffering error is absent from those
replays. Earlier experimental and negative results remain preserved separately.

Core 88e7cd0e / GUI ecaeab25 / WASAPI e5560794 / MKV f46d8d15 passes twelve
native subtitle functions including setup/cleanup, thirteen bare-player checks,
eleven timing cases, twenty-four startup/subtitle/track cases and ten repeated
WASAPI startups. All accepted children exit normally. Three original-multimedia
dummy-output startup cases retain a buffering guard diagnostic despite passing
functional checks. Those are recorded; this is not an error-free GUI claim.

[checkpoint.json](checkpoint.json) binds binaries, three changed sources,
structured results and the verified 2,725-input freeze. Gzip retains original
report/XML bytes; raw personal logs and binaries remain in the ignored local
work tree. Ordinary player checks, the wider beta gates and Windows 10/hardware
limits remain separate. Continue using [STATUS](../../../juxtopposed/STATUS.md).
