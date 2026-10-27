# Recording subtitle overlap and final text packets

27 October 2026. Source `ac762a8bbbbd88b7f4f52cde1d2128ed1d65127b`. This extends the scoped
[recording-start repair](../home-recording-start-20261027/README.md).
Three concrete defects were observed in generated-file recording:

1. A subtitle beginning before the selected video keyframe was discarded even
   when its known duration extended into the recording. The buffered recording
   path now preserves only that remaining finite interval. Expired/unknown-length
   earlier subtitles remain dropped; audio/video packet times stay unchanged.
2. The MP4 shutdown drain sent queued plain-text samples directly to the writer,
   skipping the normal conversion to length-prefixed MP4 text. Matroska/SRT text
   consequently lost its first two characters. The regular drain now uses the
   same conversion as normal writes; the fragmented route stays as before.
3. An inserted zero-length subtitle clearer could leave the MP4 edit duration at
   the last caption's start. Valid later text existed in the file but normal
   playback excluded it. For that subtitle case, the edit includes the clearer's
   timestamp, retaining the final text interval.

## Verification

The original eleven direct recording-start checks and eleven new finite-interval
checks pass (22/22). They cover remaining duration, unchanged payload/flags, future/
expired/unknown captions, audio/video exclusion, PTS fallback/composition offset
and invalid/overflowing times. They exercise production code, not a decoder/mux
integration by themselves.

The native baseline r461 reproduces overlap loss in both MP4/mov_text and
Matroska/SRT, plus truncated final Matroska text. Its recorded source/binary
inventory did not enumerate the muxer; the observed output is preserved without a
retroactive source/ending-guard claim for that old mux. Its mux hash was measured
after the run and is labeled as such. Fresh fixed inventories include the mux
sources and both replacement DLLs.

Intermediate r463 restores overlap and length-prefixed text, but the Matroska
file's edit still cuts off the later cue. A diagnostic packet dump with edit-list
handling disabled exposes that valid cue at10–14seconds. That diagnostic option
is not used for acceptance. After the duration repair, final native r465 retains
both exact labels with normal decoding: MP4 overlap0–6.080 and later10.080–14.080;
Matroska overlap0–6.000 and later10.000–14.000. Audio/video decode successfully;
start differences are21.146ms/6ms and duration differences below70ms. Every native
ending source/runtime/base/fixture/producer/worker guard passes.

The ordinary GUI441/Core265 test r466 uses record464
`9f4a3f03` and
mux464 `42e53a5e` without private hooks.
One Record tile is preseeded only in the isolated test profile; this is not a
customizer-gesture test. The visible Record control starts while paused, the play
control runs the source, Record stops the capture and the player pauses normally.
The saved file decodes811video frames and audio, with6ms start/77.687ms duration
difference. All three full caption labels survive at0–6,10–14and20–25seconds.
The actual stop happened around41seconds of source playback, followed by pause
around47seconds; no exact26-second recording claim is made. The player exits0
with602plugins, zero checked errors and all ending guards. Native config and the
complete player-profile catalogue remain identical before/after.

## Scope still open

The first usable video keyframe still determines recording start. This restores
buffered finite captions crossing that boundary; it does not reconstruct a cue
already consumed before recording was enabled or invent an indefinite duration.
Other codecs/containers, subtitle-only/overlapping-cue combinations, live recording,
repeated rapid start/stop, errors/retry, final-package and broader beta gates remain
separate. Native dummy-video runs log window closed at teardown; that fixture
diagnostic is preserved separately from the ordinary run's zero checked errors.
Immediate UIA states may lag the screen; settled observations identify final pause.
No full visual score, DPI, screen-reader or independent-review acceptance is claimed.

All inputs are generated. Media and binaries remain ignored; the receipt retains
their identities, commands, decoded results, snapshots, failures and producers.
`checkpoint.json` binds compressed artifacts to original/compressed SHA-256 and
every artifact was decompressed and verified. No firewall rules, security/privacy
or Defender settings changed. This is a frozen scoped record, not beta completion.
