# Initial paused caption - 27 October 2026

Core `0ab943f0`, ordinary GUI `4d754f60` and MP4 `df15e8ed` show the selected
English caption on the first paused fragmented-MP4 picture, before any seek or
selection action. With matched launch options, Core224 exported marker0 with
zero caption pixels; Core252 exports marker0 with1488. The caption stays held,
normal playback reaches MIDDLE, and the app exits0 with602 modules and no errors.

The decoder consumes subtitle packets already in its FIFO while paused. Its
empty-FIFO path still requests more input only for video. Thirteen software
startup controls, both31-function software/hardware timing suites, real-WASAPI
large-delay rows and twelve MP4 seek/selection cases pass with normal exits and
unchanged guards. The software before-control already passed and is not a
negative reproduction of the ordinary hardware/Qt race.

Temporary Core250 tracing is local evidence and was removed before Core252.
The previous RGB equality assertion remains failed: these exports establish the
same encoded marker and restored caption, not pixel-identical video. Sparse-cue
cost, other fragment/live layouts, secondary-screen idle capture, physical DPI
and all broader beta gates remain open. This is a nightly source checkpoint.

[checkpoint.json](checkpoint.json) retains identities, results, source copies,
compressed generated evidence and limits. Raw streams, binaries and private
profiles are excluded. Continue from [STATUS](../../../juxtopposed/STATUS.md).
