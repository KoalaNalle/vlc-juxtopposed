# Actual Playback preference consumers — 19 October 2026

**Current verified replay:** [R18](headless-replay-verification.json) passes18
cases with normal termination, no Qt diagnostics, no native errors and no
unexpected native warnings. Actual logs select only `wdummy` windows, `vdummy`
display and `adummy` output, with no hardware-decoder activity. The46 expected
avcodec text-subtitle probe warnings are followed by selection of `subsdec`.
[Runtime provenance](headless-runtime-provenance.json) and
[source inputs](headless-source-inputs.json) identify the executable and modules.

The [earlier fixture scope review](earlier-fixture-scope-review.json) corrects an
incorrect isolation assumption in R16 and earlier revisions: `avcodec-hw` is
obsolete in VLC4, and selecting a dummy display alone still allowed a normal
video-window provider. Their field assertions remain recorded, but software-only
and windowless claims are not accepted. R17 uses the real `--no-hw-dec` option,
sets the inherited window owner to `wdummy,none`, supplies output dimensions and
creates empty owned default configs. R18 reuses that exact executable and adds
strict module/error/warning checks to the runner. Source: `source-r18`.

**Earlier constructor replay:** [R16](time-shift-constructor-verification.json)
passes18 cases, normal exit/no Qt diagnostics. The actual Core constructor logs
show1 MiB/folderA on the current input, then2 MiB/folderB on the same player's
next input and native-instance reopen. Changing the saved values preserves the
paused current input/clock and does not reconstruct its time-shift owner. Empty
folder and -1 chunk size select the owned process's Windows TEMP directory and
the native50 MiB default. [Provenance](time-shift-runtime-provenance.json) includes
the unchanged file-logger module. This is **constructor consumption evidence**;
live-source pause-buffer disk writes, chunk rollover and cleanup are not accepted.
Source: `source-r16`; all R15 production model/helper objects are unchanged.
The same replay retains every earlier field's desired assertions.

**Earlier finish-action coverage:** [R15](finish-action-verification.json) passes all
sixteen cases, including setup/cleanup, normal exit0, no forced cleanup and no
Qt diagnostics. All production model/helper objects are rebuilt from the
explicit `source-r15` manifest. The added finish-action checks use only an owned
LibVLC instance's exit callback; they do not close a desktop application.

- Natural completion traverses all three finite queued inputs and sends one
  exit notification when “Quit VLC” is selected.
- Selecting “Quit VLC” while paused preserves the exact input/clock and sends
  no premature notification. It takes precedence over saved Stop after each
  item; a subsequent manual Stop sends the expected notification.
- Returning to “Stop” preserves the saved stop-after-item choice. A new native
  instance stops after the first queued input and sends no exit notification.

All prior track, quality, startup, rate, repeat/shuffle/stop and Audio policy
checks pass in the same run. [Runtime provenance](finish-action-runtime-provenance.json)
records protected modules and owned-fixture preservation. This covers nine
Playback and three Audio fields in their stated scopes; actual desktop and
final-package acceptance remain open. R14 was a rejected fixture compile using
an unavailable getter and an undeclared lock alias; it never executed. R15 uses
the actual exit/traversal observations and the native player lock.

**Earlier delay repair:** audio delay now reports its verified next-input timing and
does not leave an unnecessary restart warning. The same desired test object
fails exactly once on the original model and passes all13 cases on the repaired
model. Existing Audio defaults/PCM32, real Windows Audio4 and the Audio
component20 also pass, each with normal termination and no Qt diagnostics.
See [paired comparison](delay-paired-comparison.json),
[consumer result](delay-repaired-verification.json),
[defaults/PCM](delay-defaults-pcm.json),
[Windows output](delay-windows-audio.json) and
[component](delay-component.json). These suites overlap earlier results and are
not additive feature counts. Source: `source-r13`.

The model excludes `audio-desync` from its restart fingerprint; the native
input constructor consumes it for each new input. The actual current paused
input, clock and category delay remain unchanged. Startup-only Audio policies
and the linked live volume-step exception retain their own assertions. The
page now distinguishes immediate volume steps, next-input delay and defaults
that require a new player. Ordinary Audio R7 and matching private origin UI R4
compiled; desktop acceptance remains open behind the human firewall question.
Later fullscreen-consumer source has its own separately identified candidate.

The subsequent [audio-delay consumption check](delay-consumption-verification.json)
passes thirteen cases and proves a third Audio field: saving125ms preserves the
current paused0ms offset, while the same player's next input and native-instance
reopen use125ms. Source: `source-r11`. This also establishes a feedback defect:
the page/model still require restart for this next-input-only setting. Its
desired restart-warning regression and paired repair are in progress; that
feedback was not accepted by that earlier consumption receipt. The later paired
repair above closes the scoped model/component defect.

The latest [Audio/startup receipt](audio-startup-verification.json) passes
twelve cases with normal exit0, no forced cleanup and no Qt diagnostics. It
retains the eight Playback fields below and adds two Audio startup consumers.
With automatic track selection, saving Finnish preserves the current and
same-player next English track; a new player selects the Finnish track. Saving
Audio Off likewise preserves the current/same-player next selection, while a new
player decodes video with no selected audio. Input identity and stable paused
clock remain unchanged during the writes. Saved file bytes and all owned media
are preserved.

Source is `source-r10`; [provenance](audio-startup-runtime-provenance.json) also
pins the unchanged accepted Audio model/device objects. These checks cover eight
Playback and two Audio fields. They overlap the older receipts rather than
adding independent counts. The Audio on/off hint is being corrected separately
to agree with the verified restart behavior.

**Earlier scoped result:** R9 passes ten cases, including setup/cleanup, with
normal exit0, no forced cleanup and no Qt diagnostics. Eight Playback fields
have native behavioral coverage; actual desktop and final-package checks remain
open. See [combined receipt](finite-playback-verification.json) and
[runtime/object provenance](finite-runtime-provenance.json). Source: `source-r9`.

The expanded R8 replay was rejected. Its
seamless-transition observer used state events that VLC deliberately omits
between tracks, and the HLS paused-clock comparison caught a late update after
a fixed200ms delay. That observation does not establish that saving a preference
caused the update. The current fixture uses the documented current-media events
and establishes a stable paused value plus a no-write control before retaining
the exact post-write comparison. R9 passes these checks. This accepts the
**stable paused baseline**; it does not establish zero-latency publication of
the final position immediately after Pause. Earlier receipts below retain their
historical scopes and overlap this later result.

Finite1.2-second owned WAV inputs additionally prove:

- Repeat current returns to the same native identity at least three times.
- Repeat all traverses identities0,1,2 and wraps to0.
- Stop after each item stops after identity0 without starting another queued item.
- Shuffle uses native RANDOM order and visits all three identities exactly once.
  A valid random permutation may equal the original order; none is prescribed.

The observer uses the first playing event plus current-media notifications
during native seamless playback, as documented by `include/vlc_player.h`.
It no longer expects repeated global PLAYING events at gapless transitions.
All generated short WAVs, Matroska and HLS inputs remain unchanged. R8's source
and incomplete/rejected run remain preserved separately. The runner now saves
a failed verification even if its XML is incomplete. No Core/GUI production
repair was needed for the observer or clock-baseline corrections.

**Native default Audio/Subtitle track and local adaptive quality checks pass. Actual app
field interaction, subtitle appearance and final-package acceptance remain open.**

The latest [main-startup receipt](main-startup-verification.json) passes six
cases, including setup/cleanup, normal exit and no Qt diagnostics. It uses
VLC's genuine main-playlist initialization through the native interface API,
without loading any interface module or creating a window. Five fields now
have scoped behavioral proof:

| Field | Native behavior verified |
| --- | --- |
| Default audio track | Saved index selects the actual stream; paused input is preserved; same-player next input and cold reopen apply the new index. |
| Default subtitle track | Same checks with two embedded SRT streams; this does not prove cue appearance. |
| Preferred video quality | Local adaptive240 cap selects240; raising the limit allows the available360 rendition on next input and cold reopen. |
| Start playback | Saved Off starts paused. Saving On preserves that paused input and clock, then starts the same player's next input normally. Cold reopen agrees. |
| Default playback speed | Saving1.5 leaves the paused input and same player's next input at1.0, with restart feedback. A new native player uses1.5 and clears that feedback. |

The fixture source is `source-r7`, with
[runtime/object provenance](main-startup-runtime-provenance.json). The source
runtime, generated media, human configuration and reused production objects
remain unchanged. No Core or ordinary GUI code changed in this continuation.

R5's new startup case initially tried to edit a live transport policy before
creating a player, so the model correctly rejected it as unavailable. R6
compiles but was not run: source review showed that its bare test playlist
bypassed the real main-playlist startup configuration. R7 uses the native
application startup path and its actual instance ownership. The model's guard,
policy timing assertions and native behavior were retained. Older passing
receipts below keep their earlier scopes and are not additional coverage.

The generated owned Matroska file contains one tiny FFV1 video, two silent PCM
audio tracks and two embedded SRT tracks, with English/Finnish language tags.
The production `PlaybackPreferencesModel` saves index1 for both categories.
A fresh native Core playlist decodes audio and video and actually selects both
second tracks; these are selected native track identities, not configuration
readback or copied player overrides.

While paused, explicit choices of index0 clear the saved per-ID overrides and
preserve both current selected tracks, input identity and exact paused clock.
Starting another input on the **same native player** then selects both first
tracks. Cold reopening the native instance also selects both first tracks and
preserves the saved configuration bytes. This verifies the page's next-input
promise for these two fields, separately from restart persistence.

The [earlier track receipt](tracks-verification.json) records three passing QtTest cases,
including setup/cleanup, normal exit0 and no Qt diagnostics. Protected source
runtime and human configuration remain unchanged; the generated fixture is
read-only during the test. Dummy audio/video sinks create no window or audible
output. The check does not establish subtitle cue rendering, font fidelity,
desktop interaction or physical sound.

The exact fixture source and compiled inputs are preserved under
`build-windows/beta-playback-consumers-20261019/source-r3`. R1 was a private
fixture compilation failure from an unmaterialized Qt byte-string expression.
R2 passes the earlier cold-reopen-only scope and remains saved separately.
R3 adds the same-player next-input assertions without weakening the paused or
cold-reopen checks.

The [later combined receipt](tracks-quality-verification.json) passes four
cases, including fixture setup/cleanup, normal exit and no Qt diagnostics. Its
second fixture is an owned local HLS master with 240p and 360p renditions. VLC's
native highest-quality adaptive logic isolates the saved resolution cap from
bandwidth variation. Saving240 opens and decodes the 240p rendition. Saving720
leaves the current paused240 input and exact clock unchanged; another input on
the same player then opens/decodes360, the largest available rendition below
the limit. Cold reopen also decodes360 and preserves saved configuration bytes.
This does not claim HTTP throughput, an external service, upscaling or picture
fidelity. All generated manifests/segments remain unchanged.

The combined fixture source is `source-r4`. Its production model/store objects
are the identical accepted R3 artifacts; only the extended test was compiled.
[Additional provenance](quality-runtime-provenance.json) pins the unchanged
adaptive/Matroska/decoder/subtitle modules and reused objects. The earlier
three-case result overlaps this combined result and is not additional coverage.

FFmpeg/FFprobe already installed in the home build distro generated/inspected
these owned fixtures. No dependency, security setting, user media, commit or
publication changed. The ordinary Favorite app's Windows firewall question
remains pending; no direct UI operation resumed.
