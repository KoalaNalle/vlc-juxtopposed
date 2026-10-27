# Attachment and preset integration - 26 October 2026

Core `883f7e51` / ordinary GUI `b2a96829` is the selected scoped runtime.
Twenty-three native functions pass, including both construction orders and a
next-input control that deliberately keeps the preceding Qt preset cache alive.
Earlier failures are preserved. Attachments load the saved files; automatic
selection follows this media's saved preset, while explicit attachment Save
chooses its current default. Saved native track identities wait for their tracks.

The initial r93 full run had one failed row while isolated rows passed. The r94
fixture waits for Default restore to settle before editing/capture, then asserts
actual native delay/FPS before saving. Production code is unchanged between those
two native runs. Both results and source snapshots are retained; this fixture
correction is distinct from the production integration repairs.

Ordinary save, duplicate-name retry, Default, named reapply, cold restart and
next-source attachment selection pass on the recorded tuples. Finnish track,
-0.375-second category delay and 30 fps return after restart. The next episode
uses its own attachment and zero delay. Normal exits preserve all guards, load
602 modules and report no diagnostics. Only generated media was used.

The production preset naming form now reveals its name and Cancel/Save actions
after expansion. A failing before viewport check and passing normal/short/150%/
200% supplied-state layouts are retained. Actual ordinary entry and Cancel keep
the saved preset and paused clock. These component scales are not OS DPI tests.

Earlier occluded first captures remain evidence. Explicit activation before the
r96 initial capture shows the Finnish caption at the unchanged paused 0.08-second
position without Play or seeking. This qualifies that run, not all startup cases.

[checkpoint.json](checkpoint.json) binds exact identities and original gzip
artifacts. The [preceding local attachment receipt](../home-subtitle-attachments-20261026/README.md)
retains its separate scope. Readable-but-undecodable subtitle feedback, online
search, remaining timing combinations and the beta gates are unfinished.
Continue from [STATUS](../../../juxtopposed/STATUS.md).
