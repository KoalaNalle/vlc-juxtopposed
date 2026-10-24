# Explicit Qt seek policy

`precise_seek_journey.cpp` is an opt-in Windows fixture compiled through
`sidebar_pins_journey.mk`, with `QA_SOURCE` pointing at this file and a fresh
`QA_OUT`. Keep its translation unit out of the ordinary GUI source list.

Use an isolated matching package, an owned portable Qt INI/native config, no
library or privacy wizard, software decoding and real Direct3D11 output. Supply:

- `VLC_OWNED_UI_BASE`: owned output directory containing settings and runtime.
- `VLC_OWNED_SETTINGS_CONFIG`: native configuration under that directory.
- `VLC_OWNED_UI_REPORT`: owned JSON report file.
- `VLC_OWNED_UI_LOG`: verbose file log checked by the parent after normal exit.
- `VLC_OWNED_UI_START_GATE`: initially absent inspection marker; inspect the
  actual window before creating it.
- `VLC_OWNED_SEEK_MEDIA`: owned 60-second, 25-fps nonfragmented H.264 MP4 with
  fixed ten-second GOPs, scene-cut insertion disabled, no B frames or chapters.
- `VLC_OWNED_SEEK_MODE`: `edit`, `cold-fast` or `cold-precise`.

Verify the fixture's 1,500 frames and keyframes at 0/10/20/30/40/50 seconds using
an independent media probe before launch. Seed `edit` with precise seeking On
and `short-jump-size=3`. The subsequent cold cases copy the accepted native and
Qt configuration from their predecessor, never the human's settings.

The fixture operates the actual Playback settings Toggle and all six production
PlayerController seek routes. Each request is serialized through the paused
native clock and newly displayed picture before the next one. Do not poll the
live file logger for completion: Windows buffering can hide a completed seek.
The parent must require exactly one MP4 seek between each named BEGIN/END pair,
with `preroll` for Precise or `alignment` for Fast. Do not treat the requested
timeline time alone as proof of decoded output. Fast targets use the indexed
keyframe; precise targets are intentionally between keyframes.

`edit` also blocks replacement of its owned native configuration, checks actual
Toggle rejection and unchanged policy/disk/media, exercises the old Fast seek
policy, releases the handle, retries through the same Toggle, and exercises the
new Precise policy. It leaves Fast saved. `cold-fast` verifies restored Fast,
performs a real seek and saves Precise; `cold-precise` verifies the other saved
choice and consumer. Every case resumes decoded playback and quits normally.

Require all checks, matching immutable source/runtime/media identities, normal
termination, Stop acknowledgment before Qt teardown, one plugin-cache load,
real D3D11 output, and no unexplained native/Qt diagnostics or forced cleanup.
Keep an intentional save-failure diagnostic separate and bound exactly if the
native store emits one. This qualifies explicit Qt policy on the tested MP4,
with fresh displayed video and clock recovery. It does not establish frame-exact
output, every container's seek behavior, physical DPI or visual design fidelity.

The current fixture also repeats Fast seeks within one keyframe interval.
After each displayed frame settles, require native time, Qt elapsed/display
time and slider position to agree with the actual landing. An early native
clock read can still be the previous frame: it is insufficient on its own.
The final playing seek/resume/pause checks cover transition back to a continuous
clock. Verify its separate MP4 log marker outside the serialized paused cases.

The private timer observer copies scalar point/event data only inside native
callbacks, which prohibit all player API calls. It queues to MainCtx before
reading native and Qt clocks and is removed at MainCtx destruction. Callback
sequence/seek epoch establish native order; label/media fields are sampled at
GUI delivery. The observer receives more playing updates than production, and
JSON double rounds the maximum-tick paused sentinel. Do not infer otherwise.

Save-denial checks require the actual focused Toggle and complete local error
inside the clipped settings viewport. Initial focus setup is allowed; there is
no explicit fixture reveal after failure or before retry. Component coverage
separately exercises pointer and Space, both themes and four scaled layouts.
