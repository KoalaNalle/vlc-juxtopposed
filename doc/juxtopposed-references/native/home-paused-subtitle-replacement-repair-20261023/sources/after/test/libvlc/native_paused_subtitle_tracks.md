# Paused subtitle replacement regression

This opt-in Windows x64 LibVLC 4 test exercises real subtitle selection while
paused. It checks selected IDs/languages, English/Finnish caption masks, explicit
external selection, an already selected track, repeated changes, sampled paused
clocks, resume and normal teardown. An eleven-bit red/black marker in the top
40 video rows identifies the actual decoded frame before and after each change.
Only snapshots available while paused qualify. It is not an implementation-only
simulation or an elapsed-time estimate of which frame was displayed.

Use an isolated, fully staged runtime and Python with PySide6. The parent should
use `native_http_runtime.stage_runtime` and `environment`, record its child PID,
exit status and source/runtime/fixture identities, and preserve logs. The probe
installs nothing, does not regenerate caches and does not forcibly stop processes.

Generate the colored source using `generate_paused_subtitle_seek.sh BASE` where
FFmpeg/FFprobe already exist. Then run:

```text
bash generate_paused_subtitle_tracks.sh BASE/colored-long.mkv TRACK_FIXTURES
python native_paused_subtitle_tracks.py --fixture-root TRACK_FIXTURES --runtime PACKAGE --output FRESH_EMPTY_OUTPUT --media frame-marker.mkv --audio wasapi
```

The output folder must not exist before fixture generation. The generator adds
frame markers to 1,500 video frames at 25 fps, extracts independent marker
references 0/125/126/625, creates an external three-cue SubRip file, and produces
Matroska SubRip, Matroska ASS and MP4 mov_text variants. Repeat with media values
`frame-marker-ass.mkv` and `frame-marker.mp4`, and audio values `none` and
optionally `dummy`. Create an empty probe-output directory for every run.

The same probe, media, options and parent must run against the unchanged and
repaired runtime. A behavior failure still exits normally after writing
`result.json`; check its `passed` and per-contract `checks`, not just process
exit 0. English and Finnish masks must differ, repeated selection must restore
the matching mask, and the external caption must differ from both and survive
resume. Review representative native images to confirm their text. White-pixel
thresholds and marker coordinates are specific to these generated fixtures.

The normal-pause marker is recorded, rather than assuming the host paused on an
exact scheduled frame. Every subsequent selection must retain that exact marker.
The same rule applies after the explicit seek. Three samples per observation
check stable paused clocks. With audio disabled, reported paused time must also
remain exactly equal across selections. WASAPI absolute `get_time` can remain
stale after seeks; its picture-conservation proof comes from actual marker pixels.

Cold initial captions and automatic selection from `add_slave(select=true)` are
recorded but do not qualify this selected-replacement result. They need separate
repairs/checks. Native diagnostics remain in the result, including known dummy
window teardown errors; a behavior pass does not mean error-free logs. Desktop
controls, physical audibility, untested formats/policies, Windows 10 and the full
beta require separate acceptance. Timer callbacks only copy data; they never
call media-player APIs or write reports from the callback.
