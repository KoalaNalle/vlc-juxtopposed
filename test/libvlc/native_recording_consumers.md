# Native Recording media and window checks

Use the guest's E-only environment and run one build or native workload at a
time. All runners require a fresh output directory outside an immutable owned
portable runtime. Replacement DLL links are detached only in the new runtime;
the source package is preserved. Run the native model driver separately.

`native_recording_consumers.py` requires `--package`, `--video-fixture` and
`--output-dir`. Supply an owned synthetic video. It limits fixture generation
to two seconds with a single x264 encoder thread, records both native policies
through the actual LibVLC API into Unicode/space folders, verifies their actual
backends, and decodes both files to PNGs with normal exits.

Optional `--core`, `--x264` and `--mkv` select newly built replacements. With
`--stream-sample`, supply a read-only Matroska audio/video file and optionally
`--sample-start` (default 60 seconds). Eight bounded native stream cases check
all tracks, each exclusion, subtitle/audio/video-only output and explicit CLI
precedence. An owned SRT cue supplies a subtitle packet. The driver checks the
actual Matroska track metadata and packet bytes, normal exits and unchanged
source size/time. Keep offered media paths, raw logs, clips and captures private.

`native_recording_encoders.py` takes the same required arguments, optional Core
and x264 replacements, and an owned synthetic video. Seven actual H.264 jobs
check unset Medium, saved Ultrafast/Fast, zero-latency tuning, clearing to None,
explicit encoder-profile precedence and CLI precedence. Readback from the open
x264 encoder verifies effective CABAC, B-frame, lookahead and thread parameters.
Each bounded two-second output must decode to a PNG and quit normally. A short
deadline cannot be used for an unbounded long source.

For the graphical journey, read the configured Qt Makefile followed by
`modules/gui/qt/tests/native_ui/recording_journey.mk`. Set a fresh absolute
`QA_OUT` and build `libqt_plugin.la` then `owned-native-ui` with `-j1`. Save the
ordinary DLL separately before staging the private `.libs/libqt_plugin.dll`.
The private DLL adds one opt-in QtTest TU to the production objects; it must
never ship in the ordinary package.

Run `modules/gui/qt/tests/native_ui/run_recording_journey.py` with `--package`,
`--qa-gui` and `--output-dir`. It uses the real MainCtx/model/compiled page and
owns all targeted windows. Editing and a separate restarted process verify
native persistence, save rejection/retry, folder chooser cancellation/selection,
discovery policy, the real Stream wizard and the native expert-preferences route.
The private test requests Qt's widget folder chooser, so it does not accept the
Windows native file-dialog implementation. An owned renderer selection without
receiver playback tests preservation; a live cast receiver remains separate.
These checks do not establish a complete codec, network, DPI or platform matrix.
