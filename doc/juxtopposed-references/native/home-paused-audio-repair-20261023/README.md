# Initial paused audio seek repair, 23 October 2026

Paused audio now updates both elapsed time and the slider before its first play.
Core pending seeks use the current input duration/live status instead of unset
output-clock metadata. A time request also clears an unavailable old position.
The actual output clock and Qt controller/helper/UI policy are unchanged.

The private diagnostic identifies the cause: input duration30s is known while
the delivered seek point has length0/time0 for a30% request. It finds no later
point overwriting the requested time. This diagnostic is separate from final
acceptance. Its initial link failure produced no executable and is retained.

The unchanged real-controller test executable fails3 cases with Core0404d1f6
and passes all7 with Core4422fbb0. Actual WASAPI covers initial/after-playing
pause, percentage/time requests, repeated/zero seeks, resume and a10s replacement
for30s media. The exact pending-seek function passes15 isolated policy cases;
original source fails10. These include live/unknown duration and stale metadata,
but supplied fields do not qualify a real internet stream.

Ordinary GUI21d55d1c/Core4422fbb0 verifies initial pointer15s/.5, Go to Time09s/.3,
normal resume/Pause and Quit0 without captured errors/Qt exceptions. The existing
bookmark/chapter native executable passes9 cases on the new Core. Six actual
video snapshots retain target colors/frozen pause/resume. Two bookmark and one
video historical window-close messages are verified at teardown; parser and
initial-size warnings remain visible. No warning-free build claim is made.

Source-r23 has2,120 actual copies. Core pre-build and unchanged native executable
source records retain their separate boundaries. A staging-helper preflight
correction, made before any replay, is explicitly recorded; original files remain.
The existing video producer matches tracked HEAD and is recorded separately.

This accepts selected paused-audio seeking, not the whole beta, all formats,
physical outputs, live service, Windows10, touch, DPI/accessibility, design or
packaging. Legacy Go to Time is functional evidence, not visual acceptance.
Earlier hover/performance and Queue receipts keep their original binary scopes.

- [Checkpoint and artifact hashes](checkpoint.json)
- [Review, source boundaries and limits](review.json)
- [Native original failures](native-before-process.json)
- [Native repaired result](native-after-process.json)
- [Original policy failures](policy-before-process.json)
- [Repaired policy result](policy-after-process.json)
- [Bookmark/chapter replay](bookmarks-replay-process.json)
- [Private diagnostic](diagnostic-process.json)
- [Ordinary process](ordinary-process.json)
- [Ordinary UI observations](ui-observations.json)
- [Video replay review](video-review.json)
- [Video picture results](video-results.json)
- [Source inputs](source-inputs.json)
- [Core pre-build sources](core-source-record.json)
- [Native executable replay source record](native-replay-source-record.json)
- [Preflight correction](preflight-amendment.json)
