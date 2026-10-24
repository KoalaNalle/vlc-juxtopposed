# Paused subtitle delay regression

`native_paused_subtitle_delay.py` uses the generated fixtures from
`generate_paused_subtitle_tracks.sh` and an explicitly selected Windows LibVLC 4
runtime. Run it with the existing Python/PySide6 test environment and an empty
output directory. The parent must isolate configuration, record runtime/source
identities and wait for normal process completion.

The first and middle English cue masks are measured from the same format at
five and 25 seconds. At 20.5 seconds, a +1.2-second delay requires the first cue;
zero and -1.2 seconds require the middle cue. The worker checks both initial and
settled paused snapshots, changes the live delay through zero, positive,
negative and zero again, and requires the actual held frame marker to remain
unchanged. A separate playing journey crosses the boundary before pausing.
Its actual marker must lie in the interval used to derive the expected cue.

Caption matching retains the exact mask hashes and additionally records the
pixel difference against each expected reference. Up to 0.5% of the caption's
union pixels may differ: changing the fixture's video color changes a few
blended glyph-edge pixels (one in SubRip and three in mov_text in the recorded
unchanged controls). This allowance uses the caption area, requires over 1,000
union pixels and permits no translation or resizing. Empty captions and the
different first/middle fixture cues remain failures. The original strict-hash
results are retained separately from results produced by this revision.

Configuration `sub-delay` uses 100 ms units. The LibVLC getter/setter use
microseconds. Both readback and rendered behavior are checked. WASAPI selection
is established by the native module log; a no-audio mode permits an additional
reported-time conservation assertion. White caption masks and the encoded video
frame marker are independent of a potentially stale paused public audio clock.

Behavioral assertion failures are preserved in `result.json` while normal
teardown still exits zero; inspect `passed` and every `checks` item, not just the
process exit. Execution failures exit nonzero after attempted normal teardown.
No forced cleanup, private GUI hook, physical audibility, desktop interaction,
full subtitle workflow or full beta acceptance is established by this worker.


`--seek-mode position` uses LibVLC's precise normalized-position entry point,
matching the progress-bar route; time seeking remains the default. An optional
19-second boundary checks a -1.2-second delay bringing the middle cue forward.
Expected paused cue masks are derived from the requested boundary minus each
live delay. The playing control always crosses 20.2 seconds and derives its
expectation separately. Encoded frame markers still qualify actual paused
conservation independently of the requested timestamp.
