# GUI and WebVTT integration, 29 October 2026

This receipt qualifies particular builds and generated-media journeys. It does
not establish complete beta readiness, every subtitle format, or a final package.
The older receipts remain frozen.

The fresh ordinary GUI build completed normally after the Qt dependency recovery.
Only the menus and recent-history objects were compiled; the new target-only
debug-stripped plugin is `ccc6b01fc03a` (107,583,195 bytes). Its PE runtime-contract
checks passed, and the original shared artifacts and tested fallback were restored.
The separate WebVTT build produced `38eec724ba22`; all 79 actual compiler inputs
were within the 281 frozen dependencies, and its protected inputs stayed unchanged.

The actual native player/decoder/pixel test passed all three candidate functions:

- Paused FPS changes at 50, 29.97, 23.976 and 0, explicit seek and Reset, preserving
  the held decoded video picture and player time.
- The first default-to-50 FPS edit during playback, with the expected cue change
  and bounded forward video/clock progress.
- A subtitle source opened with an inherited 50 FPS setting.

Each candidate process exited 0, with setup/function/cleanup passing and no skips.
The logs identify the real WebVTT demux and decoder. The native harness logs one
window-close notification during teardown in both original and candidate runs;
this is disclosed rather than treated as an error-free ordinary-app result.

The original-plugin controls did **not** reach the intended FPS-specific failure
assertions. The held test failed because its normal reference caption was missing;
playing and inherited runs failed their initial track-role setup. Their raw Qt
reports are retained as failures, and none is counted as the intended negative
control. The previous eight-case source-level WebVTT receipt supplies its own
separately qualified original/fixed controls.

Three ordinary app runs exited 0 with all preservation checks passing and 603
modules loaded. R6 exercised distinct Continue Watching/Watch Again lists, generated
playback, pause and Record availability. R7 checked external WebVTT startup and
timeline interaction; synthetic Go-to-Time entry was inconclusive and is not
accepted as a precise-seek result. R8 used the new GUI/WebVTT plugins, a copied
generated profile, an explicit recording destination, repeat, and a 12-second media
start offset. Its displayed elapsed time is therefore relative to that offset.

R8's visible playback-settings editor changed the held cue at 50 FPS. The alternate
track synchronization editor showed that same value; its Reset restored normal
timing and the playback-settings editor then displayed 0 FPS. Captures identify
the actual fields, cues and paused elapsed time. These observations are distinct
from the stricter native frame/clock measurements on the native test's Core tuple.

Record acquired the active checkmark and generated one MP4. Native Stop returned
Home and disabled/cleared Record. Manual Record-off while repeat was active did
not produce a confirmed clear state and remains open. The file's audio and video
decode without FFmpeg errors; this does not establish exact recording boundaries,
complete tails, subtitle replay, or conversion readiness. FFprobe reports the
stored subtitle-related stream as data; its VLC replay still needs verification.
Two `cannot add this stream` messages occur inside the rejected AVI format probe,
before successful MP4 selection with all three streams accepted. The launcher
retained its diagnostic assertion failure; the VLC child itself exited normally.

The integrated ordinary tuple is Core `f09c45fe4075`, Qt `ccc6b01fc03a`, WebVTT
`38eec724ba22`, media library `e834a41a700b`, avcodec `545f715418c4`. It does not
inherit the separate duration-repair codec's qualification. Native pixel tests
use their separately recorded fixed Core and standalone controller executable.

During these journeys, dismissing playback settings by an outside click was
observed to toggle playback underneath. The maintained settings component test
then reproduced that exact input-delivery failure. A later source change makes
the popup modal without dimming; the same component suite passes afterward.
**That source change postdates these GUI binaries and needs a fresh native build
and ordinary before/after check.** It is not silently included in this receipt's
accepted binary identity.

`summary.json` records case-level results, pixel measurements, preservation checks,
recording inspection and scope limits. `copy-verification.json` binds the byte
copies. Only generated-media captures, Qt reports, producers and scoped build
receipts are copied here. No private profiles, personal media, human configuration
hashes, raw playback logs, recordings or executables are published in this receipt.
