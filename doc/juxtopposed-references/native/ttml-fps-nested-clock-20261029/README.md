# TTML FPS, nested output and mixed clock/frame timing — 29 October 2026

This receipt qualifies the identified source/native/ordinary scopes. All beta
gates remain open. It does not establish every TTML profile, declared frame/tick
parameter, secondary-role combination, recording boundary, physical DPI or package.

## Problems and changes

The original TTML demuxer did not expose rate control. A 50-FPS getter left its
rendered cue unchanged. Its serializer also repeated resolved absolute parent
and child times, so reparsing added parent offsets again and hid nested captions.
Generated parallel and sequential documents failed against a nonempty flat cue.

The demuxer now retains its parsed tree/change points in source time and scales
emitted timestamps. Rate changes seek the containing interval, mark discontinuity
and permit refill after subtitle EOF. Invalid rates and overflowing transforms
are rejected before state changes. Time/position and slave deadlines use the same
presentation timeline. The filtered subtree is constant until the next change:
its presentation interval is emitted once on body, while descendant timing and
sequential-container attributes are removed. Encoder serialization keeps its prior
entry point and behavior.

A separate default-30-FPS mixed-clock fixture exposed double-counting of remaining
frames during parent addition. Only whole frame-seconds are now carried into the
base; remaining frames are added once by Convert. A uint64 intermediate preserves
the sum before carry. This does not add support for arbitrary declared frame rates.
Nested timing expectations follow the [W3C TTML2 timing-container rules](https://www.w3.org/TR/ttml2/#timing-attribute-timeContainer).

## Build and native evidence

The normal two-worker plugin build produced `e980c858d139` (full SHA256 in the
verification), with 271 actual compiler dependencies inside the frozen inventory.
All owned shared plugin/object/dependency artifacts were restored; fallback and
protected inputs stayed unchanged, lock released and no forced cleanup used.
R1 compiled/linked but its check rejected equivalent absolute paths containing
`..`; that failed result and restoration are preserved. R2 fixed canonical path
comparison and passed. R3 rebuilt after the independently reproduced frame fix.

The unchanged maintained player snapshot driver calls real Core APIs and the
actual decoder. `before-r1` qualifies flat and explicit-retimed references, then
reproduces rate and nested failures. `mixed-clock-before-r1` uses the intermediate
plugin `bbc20e97a6ca`: at source 10.5 seconds the mixed-offset caption is absent,
while the independently timed BETA reference has 618 magenta pixels.

The final `after-r3` matrix has 22 successful native processes: held 50/29.97/
23.976 FPS, inherited FPS, a positive two-second delay, first change during
playback, Reset after retimed EOF, nested parallel/sequential captions and mixed
fractional-second/frame offsets. All compared colour masks match references with
Jaccard similarity 1.0. Held frame markers match exactly, time spreads stay under
50 ms, and playing clocks advance. The empty mixed-caption reference is supported
by a nonempty positive reference for the same source. Only the disclosed dummy-
output window-close teardown diagnostic appears; other-error and input/runtime
guards pass. These cases use generated media only.

The maintained `test/modules/demux/ttml_fps.c` directly compiles the production
demux, parser and serializer. Its final six cases/215 checks pass: containing-cue
seek, immutable index/rates/Reset, invalid controls leaving state unchanged, EOF
revival/ending position, actual XML parser roundtrip and mixed-frame carry.
The source build has 288 guarded inputs. Unused descriptor callbacks are deliberate
failing stubs; this source test makes no decoder claim. Actual rendering is covered
separately by the native plugin matrix. The later opt-in Makefile registration is
distinct from the qualified direct build. No toolchain dependencies were installed.

## Ordinary GUI R17

Core `4a2865915e34`, Qt `7c7db5cb675e`, WebVTT `38eec724ba22`, TTML `e980c858d139`;
other exact runtime identities are in the ordinary verification. The selected
avcodec `545f715418c4` does not inherit the independent codec26 duration result.

Using default D3D11 video output, source second 8 (visible elapsed zero) initially
shows ALPHA in green. Playback settings > Subtitles > 50 FPS changes it to magenta
BETA while the video remains paused at the same marker/clock. Tracks > Subtitle >
Synchronize shows 50. Its Reset restores green ALPHA and 0; reopening playback
settings reflects 0.000 FPS. The immediate post-click accessibility snapshot was
stale at 50; the fresh settled capture verifies 0. Both observations are preserved.
These visible facts are separate from exact native frame/time assertions.

Normal Ctrl+Q closed the owned app: child/observer exit 0, 603 modules, no captured
error diagnostics and all preservation guards passed. Personal settings and prior
generated profile stayed unchanged. Raw personal logs/profile contents are omitted.

## Reproduction

Frozen producers preserve the specific run and must be adapted to fresh outputs
and revalidated inputs. The opt-in source test takes the maintained fixture folder;
the Core driver takes video, subtitle, empty output, FPS, mode, source seconds and
category delay in microseconds. The generated black-bottom marker video is kept
ignored locally; its identity and generator are in the linked inline WebVTT receipt.
No executable, private media or test profile belongs in the ordinary package.
