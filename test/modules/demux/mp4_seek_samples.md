# MP4 sample lookup regression

This test extracts the actual `STTSToSampleChunk` helper and its comparator from
`modules/demux/mp4/mp4.c`, then compiles it with a small field-compatible harness.
It exercises first/later chunks, all 1,500 indices of a CFR track, variable DTS
runs, a nonzero base smaller than its chunk count, exact boundaries, track end
and high absolute sample indices. It does not replace whole-demux playback tests.

Prepare in an ignored directory using a local Python 3 interpreter:

```text
python test/modules/demux/mp4_seek_samples.py --output-dir build-windows/mp4-seek-samples
```

Compile the generated `mp4_seek_samples.c` with a C11 Windows compiler, keeping
`mp4_seek_under_test.h` beside it. For example, use the existing MinGW compiler
with `-std=c11 -O2 -Wall -Wextra -Werror`; name the resulting executable
`mp4_seek_samples.exe` in the same directory. No dependencies are installed by
the preparer or runner. Then run:

```text
python test/modules/demux/mp4_seek_samples.py --output-dir build-windows/mp4-seek-samples --run
```

Require 1,624 checks, zero failures, normal exit and empty stderr. The JSON binds
the source/helper/harness, runner, executable and output hashes. To reproduce an
old-source negative, prepare a separate directory with `--source OLD_MP4_C`,
compile it separately, then run with `--expect-failure`. The 21 October original
fails 1,121 checks; never turn that into a positive parser/playback claim.

The associated native Quick fixture is described in
`modules/gui/qt/tests/native_ui/precise_seek_journey.md`. Native and ordinary
before/after MP4 observations, including open UI defects, are recorded under
`doc/juxtopposed-references/native/home-precise-seek-20261021`.
