# Paused video startup probe

This opt-in Windows LibVLC 4 probe checks that a selected video track supplies a
snapshot while playback remains paused, that four paused clock samples stay
equal, and that resume and another pause (or the end of a very short file) work.
It also supports audio-only controls by checking the selected video track list.
An existing video-output resource does not establish that a video track exists.

Generate the fixtures with `generate_paused_video_startup.sh OUTPUT_DIRECTORY`
in a development environment that already provides FFmpeg and FFprobe. It
produces a multi-track H264/AAC/SubRip Matroska file with long cues, a plain
video, an audio-only file, a single-picture video, a delayed-video file, and a
timestamp-preserving remux. These are synthetic test files.

Use a fresh output directory and an isolated, fully staged Windows runtime;
`native_http_runtime.stage_runtime` and `environment` provide the established
staging and environment policy. Python must provide PySide6 for PNG inspection.
The probe itself does not copy packages, install dependencies, regenerate plugin
caches, kill processes, or close other VLC instances.

Create a config JSON such as:

```json
{
  "name": "multitrack-wasapi",
  "media": "C:/absolute/fixtures/multi-language.mkv",
  "audio": "wasapi",
  "video": true,
  "extraOptions": []
}
```

Run `native_paused_video_startup.py --config CONFIG --runtime PACKAGE --output
OUTPUT`. Repeat with exactly the same probe, config and fixtures on the old and
candidate Core. Other useful controls set `audio` to `dummy` or `none`, disable
subtitles with `extraOptions: ["--no-spu"]`, or use the generated audio-only file
with `video: false`.

Inspect `result.json`: a normal process exit means that the probe finished and
released its native objects. The separate `passed` field requires every recorded
behavioral assertion. This distinction preserves failing old-build evidence.
Capture the parent PID, terminal status and runtime/fixture/source identities.
A snapshot returned only after resume must not count as a paused-start picture.
Retain warnings and errors, including the existing empty-buffer diagnostic and
native window-close messages at teardown; do not interpret `passed` as an
error-free log or whole-beta acceptance.

This probe does not qualify paused subtitle rendering, physical audio output,
hardware decoding, real network services, Windows 10, desktop GUI behavior,
accessibility or display scaling. Those checks remain separate.
