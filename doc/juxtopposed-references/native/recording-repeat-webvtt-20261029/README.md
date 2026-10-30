# Recording completion, repeat and embedded WebVTT — 29 October 2026

The ordinary Windows 11 player reproduced a stale Record checkmark after playlist
repeat. Manual off before repeat worked, but the old input ended without a Core
recording-off notification. The next input was not recording while Qt still
remembered that recording was enabled; an off request against that already-off
input did not produce a new notification.

The repair publishes recording-off when the recorded input reaches its final
STOPPED state, after it has closed and before opening its successor. It emits
nothing if manual off already cleared that input. It does not force a global
playback stop or continue recording across inputs.

## Results

- The same real-input test executable gives **five intended failures on the old
  Core and seven passes on the repaired Core**. Cases cover manual off while
  paused, EOF, same/other-media succession, replacement while paused, Stop and
  absence of a duplicate notification after explicit off. Successor cases also
  start and stop a new recording. These are actual player callbacks and Core
  state, without a substituted controller or private test hook.
- Ordinary R13 held an already-open Record menu across the repeat boundary. Its
  checkmark cleared automatically, a new recording could be enabled, and manual
  off cleared the checkmark while video playback continued. R10 preserves the
  earlier ordinary failure for comparison.
- Both R13 outputs pass full decoding of their available audio/video data. This
  does not prove complete source coverage or exact tails: the first output is
  about 50 seconds of a 60-second input. This receipt does not determine how much
  of that difference comes from recording-start, keyframe or buffering behavior.
  Recorded cue timing remains open.
- R11 replays the earlier generated recording with external subtitle discovery
  disabled. Its embedded `wvtt` track is available/selectable and the actual
  WebVTT decoder renders `EXTERNAL LAST CUE` during playback. FFprobe reports
  `wvtt` as generic data; that label alone is not evidence of invalid subtitles.
  Selecting that track on the held frame did not immediately show the caption;
  paused selection refresh and precise timing are not accepted by this receipt.

All ordinary VLC children exited 0 with 603 modules and passed preservation
guards. R10/R13 observers exited 1 because their strict diagnostic assertions
retained three/four rejected AVI-probe errors, respectively; successful MP4
selection followed. R11's observer exited 0 with no captured error diagnostics.
The native test reports seven dummy-window close diagnostics for either Core;
these teardown logs are distinct from the clean ordinary replay's diagnostics.

The first native candidate run repaired the notification failures but failed two
later waits: the fixture assumed the public StartPaused setting also applied to
seamless successors. Explicit per-media start-paused options corrected the fixture
without weakening the assertions. Its initial source and failures remain under
`native/`; the final executable was rerun against both Cores.

## Evidence

- [Verification](verification.json): runtime identities, GUI events and scope.
- [Native results](native/): raw final before/after case output, summaries and
  the initial fixture-setup failure. [Maintained test](producers/recording.c).
- [Core build](core-build/): normal incremental make, restoration and the one
  changed source among the prior build's 59 recorded Core-source entries.
- [Test build](test-build/): 107 compiler dependencies, compile/link commands.
- Ordinary captures: [before](gui-r10/observations/),
  [embedded replay](gui-r11/observations/), [repair](gui-r13/observations/).
- [A/V decode](output-decode.json), [embedded track probe](recorded-webvtt-probe.json)
  and [copy checksums](copy-verification.json). Raw profiles/logs and binaries
  remain in ignored local run directories.

Repaired Core: `4d3952cedf6d6eccc747cf4545d29a7e1283e2968e8e15093c6c754299959d8d`.
Ordinary GUI: `7c7db5cb675edc20b3366120dc55282b09d2db2dc731b7c6ed8c1815e0706624`.
WebVTT: `38eec724ba220769389ba0aac6fb8f2cd56306f32ccff7dba3763674ce6dba1d`.
Source `src/player/input.c`: `fff61ed602e032b16837233157db38f61a20b29b8e827fd974fa796f96b1cdc1`.
Final native test: `be504dd278dd0a701e060b512635485bed2d37e525094522cb7b799bc2654c79`.

The shared build tree and tested fallback were preserved; its Core remains the
old `f09c45fe4075` binary. The repaired Core lives in the fresh candidate/run
directories. This receipt does not close full recording output, broad media,
multi-instance, physical-device, design/DPI or final-package beta acceptance.
