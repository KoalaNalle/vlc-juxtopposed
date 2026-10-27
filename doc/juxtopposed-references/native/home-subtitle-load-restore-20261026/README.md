# Subtitle load recovery and before-cue presets - 26 October 2026

The selected ordinary runtime is Core `883f7e51` / GUI `03a963ad`. Twenty-four
native functions pass with the ordinary application simultaneously running and
optional diagnostic tracing disabled. This includes four attachment/preset
construction-order and delayed-cue rows, eight timing-context rows and malformed
attachment recovery. Native and ordinary binaries have separate identities.

A readable invalid subtitle now produces one delayed notice and a retained
actionable error. Choosing a valid replacement recovers the actual caption at
the unchanged paused clock; that replacement returns after restart. Superseded
load checks do not publish stale errors. The native reentrant-lock observer passes.
The negative ordinary run retains expected DVD probe diagnostics for its invalid
SRT; the other three ordinary runs have none.

Preset capture and restore use the selected track's known native order before
the first subtitle output exists. Previously a player paused before the first
cue could wait indefinitely. Ordinary save, cold 30 fps restore, Default and
named reapply all complete before the first cue; seeking while paused then shows
the actual late caption. Native output events still refine renderer role identity.

The r104 fixture reached Save but reused names across immediate/late rows. The
r105 test-only unique-name correction is preserved separately; production source
is unchanged. Earlier mixed-run and deterministic before-cue failures remain.
All four ordinary processes close normally with 602 modules and unchanged guards.

The error's accessible name has a separate supplied-state failing-before/passing-
after control. Ordinary error run r100 predates that accessibility change.
Narrow-window notices, online search/download and remaining beta gates are open.
[checkpoint.json](checkpoint.json) binds exact identities and immutable gzip
artifacts. Continue from [STATUS](../../../juxtopposed/STATUS.md).
