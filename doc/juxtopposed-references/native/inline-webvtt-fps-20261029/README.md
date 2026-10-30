# Inline WebVTT subtitle FPS — 29 October 2026

This receipt qualifies the recorded Core/native-renderer cases and one ordinary
Windows 11 GUI journey. It does not close the beta, every WebVTT feature, TTML,
secondary tracks, recording boundaries, OS-DPI/design acceptance or packaging.

## Identities and native observations

Core `4a2865915e34`, Qt `7c7db5cb675e`, WebVTT `38eec724ba22`; exact identities
are in the verifications. The maintained `test/src/player/subtitle_fps.c` calls
the actual player FPS/delay/seek/pause APIs and snapshots the actual renderer.
Its direct cross build passed with 109 compiler dependencies, no compiler output
and stable inputs; executable SHA256 is recorded in `test-build/verification.json`.
It is separately registered as the opt-in `test_src_player_subtitle_fps` target.
That later Makefile registration does not alter the previously built executable.

The generated 60-second marker video has a black lower half. The VTT cue contains
10/20-second inline timestamps and lime `:past` / magenta `:future` styles. The
independent reference files use rational 25/FPS timestamp conversion and normal
FPS; their fixture producer and input video identity are preserved. No personal
media is included. Video bytes remain ignored in the local fixture directory.

The R2 matrix has fifteen new native processes and three calibrated R1 cases.
Held 50, 29.97, 23.976 FPS, two transition locations, a positive two-second delay,
inherited 50 FPS, Reset and a fresh first change during playback passed. Every
candidate colour-mask comparison has Jaccard similarity 1.0. Held frame markers
are identical; time spread is below 50 ms. Playing clocks increase normally.

The original WebVTT DLL `e6d599aabf0e` is a qualified negative control: with the
same Core and other runtime modules, its initial/Reset images match normal timing
exactly but its 50-FPS image fails the intended retimed colour comparison.
All native children exited 0 and completed; only the disclosed dummy-output
window-close teardown diagnostic appears. Input and both runtime guards passed.

## Preserved calibration failure

The frozen R1 observer exited 1 because it incorrectly required past-green text
before any word had completed. Its three native children succeeded. R2 corrects
that expectation without rewriting R1. Current text can match neither pseudo-
class and stays white; future text is magenta and completed text is green.
See the [W3C WebVTT pseudo-class definition](https://www.w3.org/TR/2026/CRD-webvtt1-20260520/#the-past-and-future-pseudo-classes).
The controls ensure nonempty contrasting reference phases before accepting matches.

## Ordinary GUI R16

Using default D3D11 video output, paused at source second 8 (visible clock zero),
Playback settings > Subtitles > Subtitle frame rate 0 to 50 changed ALPHA from
white to green and BETA from magenta to white; GAMMA stayed magenta. The held
marker and visible clock did not move. Tracks > Subtitle > Synchronize displayed
50 FPS; its Reset returned the original colours and 0 FPS. Reopening playback
settings reflected 0.000 FPS. Captures and accessibility exports are in `ordinary`.
These are observed UI facts, separate from the native exact-frame assertions.

Normal Ctrl+Q closed the owned app: child/observer exit 0, 603 modules, no captured
error diagnostics and all preservation guards passed. Personal configuration,
prior generated profile and fallback were preserved. The ordinary verification
omits raw personal logs and profile contents.

## Reproduction scope

Frozen producers document the exact run and are not commands to rerun unchanged.
Adapt fresh output paths and revalidate their inventories. The native driver's
arguments are video, external subtitle, empty output folder, FPS, mode
(`reference`, `hold`, `inherited`, `playing`), target seconds and category delay
in microseconds. Use a matching staged Core/plugins and isolated environment as
in the runner. Generated reference masks, not the API getter alone, establish
rendered timing. The executable and test profiles do not belong in a release.
