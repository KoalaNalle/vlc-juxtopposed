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

To qualify automatic external selection separately, pass
`--external-selection automatic`. The newly added ID must become the only
selected subtitle, its caption must match the later explicitly selected external
caption, and its actual paused picture must remain the same. With audio disabled,
reported paused time must also remain equal. A normal exit with these checks
false records a behavior failure.

Use `--external-selection preserve` for `add_slave(select=false)`: the existing
selected IDs, caption, picture and no-audio time must remain unchanged after
loading. Both modes subsequently exercise explicit selection and resume. The
optional `--external-policy language` clears string-ID preferences before
loading; `disabled` explicitly unselects subtitles. Default `ids` retains the
last explicitly selected Finnish ID. These variants check that an explicit
load-and-select request and a load-only request each retain their intended
meaning in the presence of earlier selection preferences.

The fixture generator also creates `external.ass` and `multi-language.mks`.
Use `--external-file external.ass` to exercise an ASS sidecar, or
`--external-file multi-language.mks --external-track-count 2` for a two-track
subtitle-only Matroska source. The latter has English and Finnish tracks; the
test uses an English language preference and expects the first new external
track to be English. The main media remains the chosen `--media` file.

Pass `--check-missing-external` to request a deliberately missing subtitle before
opening the valid file. This opt-in check preserves selected IDs, caption and
actual picture, and also exact reported paused time with audio disabled.
Missing-file diagnostics are expected and retained. This option does not claim
error-free logs or that asynchronous `add_slave` can report loading failure
immediately in its return value. The default probe does not insert this request.

Run the identical maintained worker, generated fixtures and options against the
unchanged and repaired runtime. For automatic selection, absence of an explicit
source choice on the unchanged build must be recorded as a behavior failure.
For subtitle-only Matroska, review main-frame/time conservation and resumed
playback as well: the selected-ID flag alone does not prove safe source timing.
