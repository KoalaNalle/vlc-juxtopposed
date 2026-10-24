# Home cache, icons and audio checkpoint — 18 October 2026

The native cache and original Videos/Music icon checks pass. The audio stream
teardown race has a tested repair on the home Windows 11 machine. **The original
Windows 10 AUDIOSES access violation is not yet proven resolved.** This is an
uncommitted development checkpoint, not beta acceptance or a published package.

## Native Qt checks

The patched QtQuick archive is linked into a fresh ordinary GUI and a private
instrumented GUI. Sixteen native processes pass 3,297 checks, including the
232-check cache/icon/window case. They use monitor 2. Captured VLC logs and late
Qt stderr contain zero warning/critical/fatal diagnostics; all exits are normal.

The cache checks decode real pixels, release and purge unused images, inspect
the actual expiry timer, preserve referenced images and leave an unused-image
timer armed for application cleanup. Pixel comparison normalizes the decoder's
RGB32 result to the fixture's ARGB32 format. The original Videos/Music SVG bytes,
native resource entries and icon-slot geometry are checked against the saved
approved exports. No additional Figma read was used.

A paired negative links the same private test object against the saved
pre-fix QtQuick archive. It fails the timer-purge assertion and captures the late
`QObject::~QObject: Timers cannot be stopped from another thread` warning.
The installed patched archive remains unchanged. This separately verifies the
cache repair and the late-warning capture; it is an expected negative result.

## Audio repair and limits

The original Windows 10 event identifies AUDIOSES 10.0.19041.5794 + `0x1358f`.
Microsoft symbols and that same mapped binary in a separate saved diagnostic
dump place the offset in `CAudioRenderClient::RuntimeClassInitialize+0x47`.
That diagnostic dump was captured at a fatalized Qt warning: **its stack and
registers are not the original audio exception context.**

Source inspection found MMDevice's stop callback deleting the stream/client on
the caller while its audio thread could still render or wait on the stream's
buffer handle. An actual-module controlled negative observes stop overlapping
the active worker in all eight cases. This establishes a real lifetime defect;
its causal connection to the historical Windows 10 event remains an inference.

The repair queues stop to the existing MMDevice audio thread and waits for
acknowledgement. That thread completes its current work or wait before stopping
the backend, releasing queued blocks and deleting the owner/event. The outer
thread also processes queued stop when no device session is active. The change
keeps the WASAPI render-service acquisition/release on its audio thread.
`mmdevice.h`, WASAPI and the shared Core binary are unchanged.

Current-source results:

| Check | Result |
| --- | --- |
| Actual-module native concurrency probe | 32/32; timer, playback, buffer wait, idle dispatch; exact queue release and callback thread identity |
| Actual WASAPI lifecycle in one retained player | 16/16 decoded/played/stop/reopen; eight per-player missing-endpoint fallbacks |
| Ordinary Qt playback | 5/5; redesigned, traditional, library enabled, generated live HLS, read-only local video |
| Repeated generated live HLS through ordinary Qt/WASAPI | 20/20 normal exits; audio decoder and WASAPI loaded; zero Qt diagnostics |
| Supplied Yle live video rendition | Decoded frame and normal exit; no audio decoder/stream opened, so audio is not accepted by this check |

No accepted process required forced cleanup. The HLS server exited normally.
The supplied Yle check initially required audio too and its combined gate failed;
that original failed result is retained beside a separate video-only assessment.
Intermediate audio lock-only/reference-count experiments are not the current
implementation or accepted production tuple.

The maintained [native audio tests](../../../../test/libvlc/native_mmdevice_stop.md)
explain their build/run commands and coverage. Their controlled idle case tests
the actual dispatcher, not the entire COM session setup. Device hot-unplug and
the exact Windows 10 failure remain unverified. A normal Windows 11 replay alone
does not establish that the original intermittent exception was repaired.

## Candidate and next work

The ignored candidate is
`build-windows/beta-native-cache-home-20261018/ordinary-runtime-r3`.
Its ordinary GUI has no private test markers. Current binary/source identities
and scoped outcomes are in [verification.json](verification.json). Private logs,
test runtimes and the retained diagnostic dump stay outside Git. User media and
home profiles are preserved; no dependency or security-setting changes were
needed. No commit, push or release occurred in this goal.

Next, validate this production tuple on the original Windows 10 configuration
and capture the exception context if the AUDIOSES failure recurs. Keep the
historical crash open until comparable evidence resolves it. Real device-loss
coverage is also outstanding. Then continue the existing beta plan's remaining
design, accessibility, DPI and clean licensed packaging gates; this checkpoint
does not broaden their acceptance.

A local `windows10-audio-recheck-20261018.zip` comparison kit is ready under the
ignored work directory (28,711,538 bytes, CRC and SHA-256 verified). It verifies
28 matching base-package files and overlays the same ordinary GUI with either
the saved original or repaired audio DLL in a fresh owned runtime. It includes
owned HLS fixtures, test instructions and the corresponding source/patch;
no personal media/profile or dump is included. Both branches passed Windows 11
kit self-checks (16 device cycles and two HLS runs each; repaired controlled
probe32). These kit checks are separate from the main 20-run acceptance batch
and do not supply the pending Windows 10 evidence.

The user confirmed on 18 October that the Windows 10 machine is currently
unavailable and requested this comparison be prepared for later. The exact
platform check is deferred; the historical crash remains open. Other home
development may continue.
