# Subtitle notices and accessible recovery - 26 October 2026

The floating notice now stays within its host width and wraps its complete text.
A failing before control produced a 1872-pixel notice in a 400-pixel window.
Production QML, actual SubtitleLabel/LabelExt, and supplied theme/notifier state
pass narrow/ordinary widths and a shorter message. The full alert name is checked.
These component dimensions do not establish Windows OS DPI acceptance.

On ordinary GUI `195471d3` / Core `883f7e51`, the detailed subtitle-load error
wraps inside a 602x421 dialog; its full name is exposed through UIA, with Save and
Cancel visible. The transient notice is separately captured after an actual
input reopen in a 962x541 player. Actual narrow-toast wrapping remains to combine.
Both processes close normally with 602 modules and unchanged source/runtime/
fixture/cache guards. Expected DVD probe errors from the malformed SRT remain.

[checkpoint.json](checkpoint.json) binds immutable source, layout, capture and
process artifacts. The preceding [load-recovery receipt](../home-subtitle-load-restore-20261026/README.md)
retains actual replacement and cold caption return on its own tuples.
Continue from [STATUS](../../../juxtopposed/STATUS.md); beta acceptance is unfinished.
