# Paused subtitle-seeking probe

This opt-in Windows LibVLC 4 probe checks coarse video segments, subtitle pixels,
frozen paused clocks, resume, another pause and normal native teardown. It uses
an isolated, fully staged runtime and Python with PySide6. Staging and environment
isolation are provided by `native_http_runtime.stage_runtime` and `environment`.

Generate synthetic fixtures with `generate_paused_subtitle_seek.sh DIRECTORY`
where FFmpeg and FFprobe are already available. The generator creates 60-second
H264/AAC Matroska files with red, green and blue ten-second segments, English and
Finnish SubRip tracks, delayed first cues, short cues separated by gaps, and
overlapping SubRip and ASS cues. The overlap fixture contains a long 0–59.9s cue
and shorter 10–12s and 30–32s cues. At 31s both captions must appear; at 15s and
35s the older long caption must remain.

Create a scenario JSON such as:

```json
{
  "name": "overlap",
  "media": "colored-overlap.mkv",
  "language": "eng",
  "audio": true,
  "steps": [
    {"name": "both", "target": 31, "color": 0, "subtitle": true,
     "referencePicture": "references/normal-overlap-31s.png"},
    {"name": "older-long", "target": 35, "color": 0, "subtitle": true,
     "referencePicture": "references/normal-long-35s.png"}
  ]
}
```

Run `native_paused_subtitle_seek.py --scenario SCENARIO --fixture-root FIXTURES
--runtime PACKAGE --output FRESH_OUTPUT`. Media and optional reference paths are
relative to `FIXTURES`; create the empty output directory first. `audio: true`
uses Windows WASAPI, while `false` disables audio. Color values 0, 1 and 2 mean
red, green and blue. Repeat on the unfixed and candidate runtime with identical
fixtures, options, probe and independently recorded references.

Reference PNGs must be 960×540 captures from normal playback through the same
captions, using the same renderer/runtime configuration. The probe compares
white caption masks with a Jaccard threshold of 0.985. Review the reference text
visually before using it. A presence-only check cannot prove that every
overlapping caption appears. Gap checks require at most ten white pixels;
caption presence requires more than 1,000. These thresholds are specific to the
synthetic colored fixtures, not arbitrary media.

`result.json` separates behavioral `passed` from normal process completion.
Capture the parent PID, terminal status, runtime/source/fixture identities and
the reference provenance. Only snapshots available while paused qualify.
Output-clock readback may lag a seek, so color proves only a coarse segment,
not an exact frame timestamp. Process I/O and CPU include probe/PNG/font reads
and filesystem caches. Timings include a fixed settle period and snapshot work;
they are not seek-latency or physical-storage benchmarks. Long active cues may
still require replay from their beginning even when prefix indexing reads only
block headers.

Retain native warnings and errors with the results. The probe installs nothing,
does not regenerate plugin caches, and never forcibly terminates VLC or other
processes. Stop during a pending seek, fast-seek policy, desktop track changes,
initial subtitles, external subtitles, Windows 10 and full beta acceptance
require separate checks.
