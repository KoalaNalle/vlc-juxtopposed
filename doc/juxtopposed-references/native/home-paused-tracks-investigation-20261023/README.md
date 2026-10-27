# Paused video/subtitle native investigation, 23 October 2026

This investigation reproduces two open playback failures below the QML menu.
It accepts no repair or whole workflow. The ordinary application remains on
GUI716862a1/Core4422fbb0/WASAPI4e4e, with the selected menu-input repair intact.

The original multi-track container fails paused startup with active audio,
including dummy audio, one decoder thread and disabled subtitles. No-audio
controls produce a paused picture. The plain remux succeeds with WASAPI, as do
three additional remuxes, including one retaining every original track. All
1500 compressed video packet payloads match; video timestamps shift21ms.
This points toward container timing/layout, rather than audio or track count
alone. The exact cause still needs a matched timestamp control and tracing.

| Case | Audio | Subtitles | Threads | Initial paused picture |
| --- | --- | --- | --- | --- |
| multi-spu-noaudio | none | on | 6 | available |
| multi-nospunoaudio | none | off | 6 | available |
| plain-noaudio | none | off | 6 | available |
| multi-spu-wasapi | WASAPI | on | 6 | missing |
| multi-nospuwasapi | WASAPI | off | 6 | missing |
| multi-spu-wasapi-thread1 | WASAPI | on | 1 | missing |
| multi-spu-dummy | dummy | on | 6 | missing |
| multi-nospu-dummy | dummy | off | 6 | missing |
| plain-wasapi | WASAPI | off | 6 | available |
| remux-all-wasapi | WASAPI | on | 6 | available |
| two-audio-no-subs-wasapi | WASAPI | off | 6 | available |
| one-audio-two-subs-wasapi | WASAPI | on | 6 | available |

The original-container subtitle cases render the middle cue after a25s request;
remuxed subtitle controls lack that cue too. All six subtitle cases lose text
after a5s backward request while paused. Explicitly selecting the
external track also leaves no visible paused cue; resume shows its first cue.
The public add-subtitle API's select flag leaves the manually selected Finnish
track active, so explicit track-ID precedence is a further investigation point.

The55 qualified native PNGs include positive and missing-text controls. Direct
inspection confirms ENGLISH MIDDLE CUE, an empty gray frame after backward seek,
and EXTERNAL FIRST CUE after resume. Native output-clock readback can stay stale
while paused; this does not claim exact video PTS or replace prior seek evidence.

Twelve processes stop/release and exit normally0 with no identity drift. Every
run retains a window-close diagnostic at vout teardown; five also retain the
startup buffer-deadlock error. Cache/fallback/size/lateness warnings are retained.
The native window provider is named window and the display is vdummy. No
windowless, clean-log, physical-audibility or whole-beta claim is made.

Initial snapshot calls queue successfully but five files do not arrive in the
bounded paused observation. Files appearing later after resume are excluded
from initial-paused evidence. Earlier producer setup/report-finalization errors
are preserved separately; no production code changed to make this matrix run.

The first publication checker caught an overgeneralized audio-only conclusion
because plain-wasapi succeeds. Its failed producer is preserved; partial raw
copies were checked unchanged before completing this corrected record.

Next: timestamp-preserving versus shifted remux controls, then a private decoder
wait/pause trace and separate subtitle packet/cue/selection tracing.
Keep the ordinary Core unchanged until paired evidence supports a repair.
The full13-category/32-surface beta roadmap remains open.

- [Checkpoint and artifact hashes](checkpoint.json)
- [Comparison matrix](matrix.json)
- [Video packet identity and timestamp comparison](packet-comparison.json)
- [Findings, source boundaries, failures and limitations](review.json)
- [WASAPI native observations](multi-spu-wasapi/investigation.json)
- [WASAPI source log](multi-spu-wasapi/native.log)
- [Middle cue after25s request](multi-spu-wasapi/seek-middle-paused.png)
- [Missing cue after5s backward request](multi-spu-wasapi/seek-first-paused.png)
- [External first cue after resume](multi-spu-wasapi/external-after-resume.png)
- [No-audio positive startup](multi-spu-noaudio/initial-paused.png)
- [Dummy-audio comparison](multi-spu-dummy/investigation.json)
- [Frozen source inputs](source-inputs.json)
- [Rejected publication assumption](failed-publication/review.json)
