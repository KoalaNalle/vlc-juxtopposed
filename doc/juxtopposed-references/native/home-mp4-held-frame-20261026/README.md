# MP4 edit-list held-frame repair - 26 October 2026

MP4 plugin `4f6dcef6` with Core `ccf72399` and GUI `4d754f60` bases edit-list
eligibility on presentation time. Decoder preroll for reordered video no longer
moves the paused frame when a large subtitle offset is applied. The unchanged
embedded and inherited-secondary regressions fail the prior plugin and pass this
candidate, including actual WASAPI and hardware-video resume.

All 31 hardware subtitle functions, four main startup rates and thirteen
audio/rate cases pass separately. Three startup late-picture warnings remain
recorded in the full hardware suite. Four generated MP4 layouts pass after the
repair. Fragmented MP4 seeking still fails identically before/after; an independent
FFmpeg seek decodes that file correctly. That separate defect remains open.

The ordinary app qualifies inherited and independent secondary delays/Reset and
normal resume. Three native exports are pixel-identical with marker 301. It exits
0 with 602 modules, zero error diagnostics and unchanged guards.

[checkpoint.json](checkpoint.json) preserves exact tuples, source copies, before
failures, all matrix cases and qualification limits. Raw streams stay local.
Continue from [STATUS](../../../juxtopposed/STATUS.md); beta gates remain open.
