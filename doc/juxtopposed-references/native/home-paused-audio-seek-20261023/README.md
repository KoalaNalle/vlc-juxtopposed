# Paused audio seek investigation, 23 October 2026

The ordinary app exposed a mismatch: seeking a paused WAV moves the thumb while
elapsed time stays at its previous value. It reproduces without a start-time
option. This investigation is evidence of an open issue, not a passing fix.

Eight native observations compare two outputs, initial/after-playing pause and
position/time seeking with a generated30s WAV. Every paused clock readback stays
at its old value through4s. All four actual WASAPI cases resume near the requested
9s target. Dummy resume samples differ and remain in the record; they do not
substitute for Windows output behavior. Each final worker stops/releases normally
and exits0 with no captured native errors/warnings or runtime identity drift.

No production audio, timer or controller code changed. The next step is to trace
actual timer seek/update/pause callbacks and their delivery to PlayerController
before choosing a repair. Stream/subtitle clocks and live/finite distinctions
must remain correct. Ordinary UI, other formats, physical outputs, Windows10
and whole-beta acceptance remain open.

Two earlier investigation failures are preserved: the global startup option did
not establish initial pause; the next dummy run failed its resume-target gate.
The final producer uses an explicit per-media startup option and records those
observations without accepting a failed seek as success.

- [Checkpoint and hashes](checkpoint.json)
- [Review and next step](review.json)
- [Eight native observations](native-observations.json)
- [Producer](investigation-producer.py)
- [Native log review](log-review.json)
- [Earlier dummy result](earlier-dummy-result.json)
- [Earlier dummy target failure](earlier-dummy-failure.txt)
- [Initial startup setup failure](initial-startup-setup-failure.txt)
