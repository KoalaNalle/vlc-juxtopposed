# VLC Juxtopposed: first publishable beta plan

17 October 2026. **Target: a feature-complete Windows beta of Juxtopposed's design, committed in Git with a reproducible runnable package. Status: incomplete; this document is a plan, not beta acceptance.**

The user's latest request supersedes the earlier goal of one useful action per surface. Preserve the [milestones 0–8](juxtopposed-vertical-slice-plan.md) and their completed evidence, but use the beta gates below as the completion criterion. Every confirmed design surface and action must be reachable and functional. A legacy dialog link, supplied controller, placeholder data or a single successful example does not finish a redesigned surface. Supported-format limitations must be explicit and consistent with the advertised feature.

## Release boundaries agreed with the user — 17 October 2026

This clarification takes precedence over older broad parity requirements below.
The user resumed development under this revised scope; the beta goal is active.

| Release | Scope |
| --- | --- |
| Beta | Bring Juxtopposed's confirmed redesign to life with real behavior and relatively few bugs. Complete the designed surfaces/actions, fidelity, persistence and ordinary playback journeys; fix regressions and deliver a coherent tested package. |
| V1.0 | Complete coverage of vanilla VLC features in the redesigned application. Discuss further feature placement and behavior with Jux before settling those details. |
| V1.5 | Explore additional features beyond vanilla parity, such as YouTube and possibly Twitch extensions. These are candidates, not committed specifications. |

For beta, a feature explicitly present in the confirmed design still needs real
behavior; underlying native work necessary for that behavior remains in scope.
Do not add vanilla-only screens, workflows or exhaustive feature-parity gates to
beta merely because VLC already provides them. Retain existing working native
routes and avoid regressions. A menu entry alone does not imply a newly designed
dialog when no original dialog is specified. Preserve missing references as
questions for design discussion rather than inventing new beta geometry.

The earlier disc suggestion is a V1.0 parity/usability discussion item: whole-disc
DVD/Blu-ray folder playback, ISO handling, Explorer integration and AutoPlay.
Operating-system ISO mounting is a separate possible enhancement, not an assumed
missing VLC capability or beta requirement. If the original redesign includes an
Open Disc entry, keep that entry functional using the existing native route;
additional disc UX is not automatically a beta blocker.

Beta quality means the designed workflows work reliably, with no known critical
crash, data-loss or broken normal-flow defects in the accepted paths. Document
minor known issues and supported-format/platform limits. This does not require
claiming every possible VLC feature or media/hardware combination bug-free.
No designer outreach or new extension implementation is authorized by this plan.

The [Recording model foundation](juxtopposed-references/native/guest-recording-model-20261017/README.md)
passes native13 with real Unicode config/INI persistence, file-lock rollback/
retry, typed track/x264/cast defaults and CLI precedence. All12 native-tested
inputs match; the registered Autotools target resolves and Meson syntax parses.
MainCtx/QML construction and actual recording, discovery and Stream consumers
remain pending; default consumer gates stay false. The native recording toggle
does not force re-encoding when OFF; its original label was raised as an optional
product clarification. No forced encoder mode is implemented.

The [Performance page checkpoint](juxtopposed-references/native/guest-performance-page-20261017/README.md)
mounts all14 original fields/actions in MainCtx and the settings host, with the
original19 SVGs. Native model17, actual window51/14, software media11, hardware3,
component Performance6/Network7/Controls8 and ordinary playback5 pass normally.
The first window passed its actions but crashed on shutdown and remains rejected;
equivalent ASCII option validation replaces its Qt regex, and both corrected
window builds pass with regex JIT enabled. Drop frames truthfully requests restart,
and a saved but unapplied real-time request can be cleared. Coreb283af/ordinary
GUId36894 are the recorded incremental tuple; all2212 frozen inputs match.
Full regression, themes/accessibility/DPI/platform and the clean licensed beta
package remain open. Next: original Recording12 and Extensions7, then remaining
designed consumers/journeys and quality gates. Do not add vanilla-only scope.

The [Windows priority consumer](juxtopposed-references/native/guest-process-priority-20261017/README.md)
now applies saved policy after config/full CLI parsing. Paired before7 reproduces
ignored saved high and a flag-like string falsely raising priority; new21 passes
all base classes, precedence, inheritance and invalid/overflow rejection.
Exact real-time is not granted on this account; prior class is preserved with
an explicit error. Only quiet owned children change class and immediately
restore it. Core0a2ad3 with unchanged GUI753feb passes ordinary5. Sixteen scoped
hashes/standard target resolution/Meson syntax match. The later Performance page checkpoint adds the typed model, checked persistence
and compiled UI; full category/platform and beta acceptance remain separate.

The [plugin Rescan backend](juxtopposed-references/native/guest-plugin-rescan-20261017/README.md)
now passes native16 with real Unicode generation, Core parser/cold-cache reload,
playing/paused independence, checked atomic publication, locked-file retry,
cancellation/deadline/reaping and concurrency guards. A real narrow-argv Unicode
bug is fixed, and an older Core rejects the new tool before cache writes.
Coreea9253/generator6c50e5 with unchanged GUI753feb passes ordinary5. All19 scoped
inputs match; Autotools target resolution and Meson syntax pass. The old
hardlink-cleanup false positive and subsequent Unicode failure remain rejected.
The later Performance page checkpoint links this backend in production GUI and
exercises the action. This earlier proof remains scoped to the rescan foundation.

The [automatic encoder preference](juxtopposed-references/native/guest-encoder-preference-20261017/README.md)
now has real automatic-selection fallback and explicit-profile precedence.
Chain7 and global7 export every exact 88,200-sample PCM; the older producer
provides a targeted ignored-preference negative. Actual Conversion12 verifies
the start-time snapshot/next-job change, automatic PCM validation, publication,
cancellation and primary-player independence; strict FLAC9 passes. Ordinary
Core602632/GUI753feb/transcodeb1a422 passes five generated/local-HLS/Downloads
smokes normally. All2176 selected inputs match; no private fixtures ship and
earlier runtimes remain preserved. The later Performance page checkpoint adds
its other native media consumers. Recording12, Extensions7 and broader beta gates
remain open. Native
video/subtitle encoder coverage and complete later-tuple regression are pending.

The [HTTP consumer repair](juxtopposed-references/native/guest-http-consumers-20261017/README.md) now passes five actual CLI policy/authentication cases,
twelve TLS/proxy cases, actual PAC9 with a targeted old-consumer negative,
object-policy16 and all six HTTP unit executables. Core6823ce/GUI8cf68a pass
native Network17/Controls19, resolver45, buffering/TLS1.2 and1.3 each50, two
actual settings-window processes31/11 and four generated/local-HLS playback
cases. A corrected bounded Downloads check passes D3D11VA and normal quit;
the invalid earlier test invocation is retained as failed. Source a5b1cf5e86
also fixes a real HTTPS CONNECT logger-object crash. Raw credential logging is
removed and authority scoping is enforced. This focused checkpoint leaves the
subtitle stall, broader native/visual/DPI/Windows11 and full-beta gates open.

A later owned ordinary-HTTP transport audit confirms that this older tuple
ignores CLI proxy/direct overrides and drops the path before PAC lookup. The
initial password negative incorrectly required preemptive authentication without
a 407 challenge. A corrected 407 replay authenticates through the legacy backend,
but exposes the synthetic password in its debug log; the whole application does
support that authentication fallback. This corrects the earlier broader password
failure claim. The 45 resolver/static-proxy checks do not cover these consumer
behaviors. Subsequent consumer repairs and native transport checks are in
progress; do not treat this older tuple as acceptance of them. The ordinary
smoke's direct argument alone does not prove system-policy avoidance on this
older Core. The separate subtitle stall remains unexplained.

The [owned native Network window journeys](juxtopposed-references/native/guest-network-window-20261017/README.md)
now pass 31 editing/failure/retry checks and 11 restart checks in two normal
processes. They exercise the actual MainCtx/controller, compiled page, pointer
and key events, native file-lock rejection/retry, masked password persistence
and the real Advanced QWidget route. Captures cover Dark at observed 100% scale.
The opt-in private GUI adds one test TU/QtTest to the same production objects;
the ordinary GUI remains unchanged. Full theme/DPI/media interaction, the
intermittent subtitle-role stall and the full beta remain open.

The [guest history-clear checkpoint](juxtopposed-references/native/guest-history-clear-20261017/README.md)
repairs discarded database failure results; seven actual native SQLite rejection/
retry/persistence checks pass, with ordinary playback and final-tuple 18/42/9/37.
The [fresh Privacy audit](juxtopposed-references/figma/20261017/privacy/README.md)
archives all nine authored rows and original assets with one context request
including its screenshot. Controls, Network, Performance, Recording and Extensions references are cached;
two planned/two reserved Figma calls remain. The source mounts eleven category
bodies; Recording and Extensions still lack bodies. Mounted categories still
have remaining native consumer/visual acceptance gates.
The [Privacy policy implementation](juxtopposed-references/native/guest-privacy-policy-20261017/README.md)
adds independent position-storage permission, shared exclusions, coordinated clear,
Home/Open Network consumers, metadata sharing and explicit unavailable Last.fm
states. A first-operation media-library clear crash is repaired. History40, controller14,
real SQLite controller9, QML8 and final ordinary4/Library18/42/9/Subtitle37 pass.
Last.fm service, actual native Home/dialog interaction journeys,
desktop/DPI and complete Privacy/full-beta acceptance remain open.

The [Library-off history/resume checkpoint](juxtopposed-references/native/guest-history-library-off-20261017/README.md)
now separates history access from browsing/indexing without changing the saved
Library flag or roots. It passes history40/52, controller17, SQLite controller9+9,
six actual decoded-history cases, ordinary4, native 18/42/9/37 and 4/25/8. The
decoded pair also repairs whole-media resume percentages being interpreted in
clip-relative coordinates. Home visit persistence permits real failure/retry.
All 1280 final frozen inputs match; final clean packaging and broader Privacy/
native desktop acceptance remain open. The [original Controls reference](juxtopposed-references/figma/20261017/controls/README.md)
contains all 16 authored fields/actions and 18 unchanged original SVG assets.
The [Controls implementation](juxtopposed-references/native/guest-controls-20261017/README.md)
now mounts those rows with native consumers and the user's requested wheel-link
toggle, ON by default. Shared Mouse/Remotes edits need restart; OFF restores
independent remote preferences that apply live. Native input89, controller19,
widget/dialog14, authenticated HTTP29 and QML8 pass. The rebuilt ordinary GUI
passes four playback/local-HLS cases. Final-tuple Library18/42/9, Subtitle37 and
player controller4/25/8 pass normally without failures/errors/skips or forced
cleanup; all 2,001 frozen inputs match.
Full native page/browser interaction, independent visual/DPI and full beta
acceptance remain open. The original
[Network reference](juxtopposed-references/figma/20261017/network/README.md)
is cached with its ten fields/actions, screenshot and nineteen SVGs. The
[Network implementation](juxtopposed-references/native/guest-network-20261017/README.md)
now mounts them with checked persistence. Real Windows proxy45, actual file/HTTP
buffering and GNU TLS1.2/1.3 each50, native controller17 and QML7 pass. The new
ordinary runtime passes four playback/local-HLS cases; Controls19/14/89/29,
Library18/42/9 and Subtitle37 pass too. The initial player subtitle-role replay
stalled after five cases. Thirteen focused8-case replays pass, but its cause is
unresolved: the failed aggregate remains failed and complete player regression
acceptance stays open. All2318 selected inputs match. Native full-page, visual/DPI,
Windows11 and clean final packaging remain separate gates.

The [guest fixed-rate audio clock checkpoint](juxtopposed-references/native/guest-soxr-clock-20261017/README.md)
closes the specific export timestamp discontinuity found during WAV/PCR work.
SoXR fixed conversion now keeps an exact sample clock and preserves unanchored
PCM; no-clock chains perform fixed conversion before their normally transparent
final resampler. The adaptive playback branch is unchanged. Sixteen loaded-module/
chain cases, lifecycle 11, Audio preferences 32, Waveform 9, strict PCM/FLAC tails,
ordinary playback and final-tuple native 18/42/9/37 plus 4/25/8 pass. The full export
has zero gaps/overlaps with every sample. Broader A/V sync, SDI hardware, complete
settings/surfaces, native visual/DPI, Windows 11 and final clean packaging remain open.

The [guest WAV/PCR checkpoint](juxtopposed-references/native/guest-wav-pcr-20261017/README.md)
repairs missing WAV packet duration and partial-read arithmetic. Nine actual
demux cases go from all failing to all passing; the same clean export runner
changes only WAV and loses its PCR error while retaining every sample. Waveform,
strict PCM/FLAC boundary counts, native 18/42/9/37 and 4/25/8 regressions and ordinary
GUI/local HLS checks pass. Diagnostic helper code is absent from production.
A separate resampler output timestamp discontinuity is now evidenced and stays
open; zero PCR errors does not accept all A/V sync. Complete surfaces/settings,
native visual/DPI, final clean package and Windows 11 gates remain open.

The [guest audio-visualization checkpoint](juxtopposed-references/native/guest-audio-visualization-20261017/README.md)
repairs generic audio-filter callback ownership and prevents no-clock export/
waveform chains from opening inherited visualizations. Native playing-output
visualizer callbacks/restarts work; Audio preferences pass 32 cases and waveform
passes nine, including configured visualization, with complete PCM/peaks and no
callback/write errors. The new ordinary tuple and native 18/42/9/37 plus
4/25/8 controller regressions pass normally. A stalled intermediate configured
probe required explicit owned-process cleanup and remains diagnostic only.
The independent short stereo-to-mono PCR-match failure stays open, along with
full native visual/DPI, settings/surfaces and final clean beta/Windows 11 gates.

The [guest file-output checkpoint](juxtopposed-references/native/guest-file-output-20261017/README.md)
repairs zero-byte blocks discarding later payload. An identical native test goes
from five failures to all eight passing; full headless transcode writes go from
twelve errors to zero with an exact 16,000-sample output. Native AudioWaveform
passes eight cases, and 30-second PCM regressions retain exact sample counts,
source-rate byte equality and all 300 peaks. Core/GUI and the ordinary package
are unchanged. Missing-visual-callback and PCR-match errors remain open; this
module result does not finish waveform GUI, conversion codecs or full beta.

The [guest teardown follow-up](juxtopposed-references/native/guest-teardown-20261017/README.md)
repairs six deferred callbacks writing to destroyed targets. A freshly rebuilt
ordinary GUI passes default redesigned/traditional finite playback (seven QML
diagnostics before, zero after), local HLS and two short local MKV samples, all
normal exits. Component checks and fourteen QML parses pass. This is incremental
Windows 10 acceptance of those reproduced warnings; full-file playback, native
visual/DPI, waveform/transcode triage and complete beta/package gates stay open.

The [guest live-clock checkpoint](juxtopposed-references/native/guest-live-clock-20261016/README.md)
now passes native and ordinary-runtime checks for relative live clocks, paused
seek-to-zero and adaptive pause/buffering order. An identical tightened test
executable fails against the old core and passes against the repair. The new
GUI/core/launcher tuple passes native 18/42/9/37 plus 4 timeline, 25 timer and
8 subtitle-controller cases; three tightened and five earlier fresh-process live
journeys also pass, all normal exits. Five startup cases, component checks and
eleven QML parses pass. Default redesigned/traditional finite playback and local
HLS pass after actual Qt/QML resource rebuilding. The teardown follow-up above closes its reproduced
delayed QML callback warnings. Full native visual/DPI, public stream/protocol,
remaining surfaces and coherent final beta-package/Windows 11 gates remain open.

The [guest startup-policy checkpoint](juxtopposed-references/native/guest-startup-20261016/README.md)
repairs the Windows launcher's saved-Library-OFF override. Its real-process
original/fixed pair exposes two failures before repair and passes all five cases
afterward, including saved ON and CLI overrides. The core default remains OFF;
fresh first-run Library setup and complete GUI/visual/package acceptance stay
open. Guest build/tool readiness is recorded under E:\Codex\migration-20261016;
the complete beta goal continues on the E: checkout without publication permission.

The latest [16 October live-service checkpoint](juxtopposed-references/native/live-services-20261016/README.md)
plays the supplied Yle TV2 HLS stream and downloads real MusicBrainz/Cover Art
Archive artwork during a fresh library import. Native metadata/opening/process/
subtitles pass 18/42/9/37 and maintained Library UI passes 11 groups. Directory
comparator/provider-order repair and HTTPS requests are included; optional
library enrichment now allows ten seconds after a measured 6.672-second public
transfer. This document belongs to the user-authorized publication checkpoint;
verify that commit's remote CI separately. Live timeline labels and wider service
coverage remain open; this is not a release.

The [guest popup checkpoint](juxtopposed-references/native/guest-popup-20261017/README.md)
reproduces the immovable Network header and implements shared movement for
Network/New Playlist/Conversion/Global Settings within the owning window. The
same component test fails original Network movement and passes 96 assertions
after repair, covering 1/1.5/2 component scale, focus/modal guards, resize bounds,
close/reopen; all four focused dialog suites also pass. Rebuilt ordinary GUI
playback/local HLS passes without QML errors and with normal exits. Actual
desktop pointer, Windows DPI/multiple monitors and independent visual acceptance
remain open. Short local MKV samples use D3D11VA on the GTX 1080 but retain late
frames; sustained smooth playback is not accepted by those startup/seek samples.

The preceding [16 October metadata checkpoint](juxtopposed-references/native/library-metadata-20261016/README.md)
enables Library metadata/artwork after native lifecycle, scope, fallback and
thumbnail-store checks. Final metadata/opening/process/subtitle suites pass
18/42/9/37 cases with zero failures/errors/skips and normal parent exits.
Four monitor-2 launches verify embedded tags/cover, indexed album artwork and
restart persistence; the original portable profile is restored byte for byte and
checked personal configuration is unchanged. Native network scope uses owned
local-only providers, so public-service coverage remains separate. The reproduced
private-provider assertion is repaired. A first subtitle save assertion failed;
old/new isolated checks and subsequent unchanged full replays pass, with cause unconfirmed.
Computer Use is available again on monitor 2.

The preceding [16 October morning checkpoint](juxtopposed-references/native/library-opening-20261016/README.md)
accepts the bounded Library opening/folder slice: single-instance/file-manager
policy, explicit append/replace, recursion and Windows hidden files. Maintained
native opening tests pass 42/0/0/0, actual sender/receiver processes 9/0/0/0, and
the unchanged subtitle regression 37/0/0/0. The same opening executable fails
three external hidden-policy cases against the earlier runtime and passes the
matched repair. That checkpoint enabled three Library consumer flags; the latest
metadata checkpoint above enables the fourth. Actual monitor-2 checks cover saved Top alignment,
embedded English/Finnish selection, shortcut feedback duration/OFF and folder
behavior, with normal quits and checked personal configuration preserved.

The opening, metadata and live-service work is included in this source checkpoint;
its remote CI must be checked separately from baseline acceptance. The actual
Windows launcher overrode saved media-library OFF at that checkpoint (repaired
by the later guest startup-policy checkpoint above),
and the earlier transient isolated parser window lacks a retained diagnosis to
compare with the now-repaired private fixture assertion. Full Library/beta
acceptance still requires public-provider coverage, full design/media
acceptance, other unfinished categories and the clean licensed package.

The [15 October General checkpoint](juxtopposed-references/native/general-settings-20261015/README.md)
adds the real native General page, shared modal ownership and live theme/accent
propagation. Ordinary native backend tests pass 29/0/1; the actual app restores
Light/Purple and Unicode Errors-only logging after restart, and three normal quits
preserve the checked existing user configuration. Maintained General/shared-host
tests and the actual GNU-make resource-remake oracle pass locally. General was the
only enabled category at that checkpoint. The later bounded checkpoints below
supersede its Interface, Light-volume and backdrop status; full Reset, broader
design acceptance and final packaging remain open.

The following [Light volume checkpoint](juxtopposed-references/native/light-volume-20261015/README.md)
closes the original Light background/full-fill selection gap with a rebuilt native
pointer/keyboard/mute/wheel/theme-switch journey and seven maintained component
groups. Complete Light states, System events and full design acceptance stay open.

The [settings backdrop checkpoint](juxtopposed-references/native/settings-backdrop-20261015/README.md)
accepts native Home blur/shadow and preserves translucent tint over separate native
video. The original shader oracle and maintained 12/24/23 General/host/software
checks pass. A real GNU-make pair reproduces and repairs stale incremental QML
metadata; the final ordinary build opens its actual settings types and quits
normally. Hardware-video blur, performance/DPI/full fidelity, geometry drift and
the remaining beta gates are unfinished.

The [independent settings shadow follow-up](juxtopposed-references/native/settings-shadow-20261015/README.md)
keeps the original shadow over real native video in Light/Dark and preserves Home
blur. Its first ordinary build includes the new QML type, original 39 native
shader assertions and maintained 12/24/23 checks pass, and the app quits normally
with checked human settings unchanged. All four preceding backdrop-head CI jobs
pass; this checkpoint's four CI jobs subsequently passed too. Full video blur
remains an open gate.

The [Windows geometry follow-up](juxtopposed-references/native/window-restore-20261015/README.md)
fixes the reproduced restart shrink by restoring the saved client rectangle after
native frame initialization. Exact original/fixed restart oracles, both title-bar
modes, native maximize/fullscreen restoration and two ordinary app launches pass.
Other DPI/missing geometry/startup states remain separate. All four jobs on
geometry head `ba175cfc69` pass, including 23 commands in each Windows job and the
two Ubuntu remake workers. This does not finish the beta.

The [native Interface checkpoint](juxtopposed-references/native/interface-settings-20261015/README.md)
adds the 18-value page, a shared checked store and registered native Interface
tests (14/0/0). Private original General desired/policy/fault replays against the
actual production objects pass 29/0/1, 31/0/1 and 3/0/0; these are not newly
registered General suites. Hidden native screen-policy checks pass 6/0/0. The
same supplied-boundary toolbar component oracle fails twice before a single
state-target repair and then passes all 25 cases. Interface/General/host/software
checks pass 10/12/24/23; four normal desktop quits preserve human configuration
and cover decorations/Unicode-path restart, validation rollback and Light/Dark.
Eight Interface settings remain explicitly unavailable until native consumer
checks pass. Skin picker/parser, tray, remaining categories, DPI/full fidelity and
the final beta package remain open. All four jobs on Interface head `04189f5271`
pass (25 Windows commands, two Ubuntu workers); this bounded checkpoint does not
accept the full Interface surface.

The [native global Playback checkpoint](juxtopposed-references/native/playback-settings-20261015/README.md)
adds all 19 authored values with two unavailable capabilities, the expanded
time-shift fields and accurate native live/restart behavior. Its registered
isolated playlist/player fixture passes 23/0/0, including four fresh-player rate
rows and three locked-save/retry rows. Three actual app journeys verify saved
speed/repeat after restart, current-versus-next speed, live queue changes,
visible lower-field validation and paused red/blue seeks, then quit normally
with checked human files unchanged. A strict old/new validation oracle becomes
15 passing component groups; General/Interface/host/software retain 12/10/24/23.
An intermittent earlier Home glyph-painting issue remains open despite the final
clean launch. Native picker and broader media/precision/thumbnail/resume/privacy,
full Figma/DPI and beta package gates remain open. All four jobs on Playback head
`9282165c2a` pass, including 26 ordered Windows commands and both Ubuntu workers.
This is bounded Playback acceptance, not the complete beta.

The [native Audio/Video checkpoint](juxtopposed-references/native/audio-video-settings-20261015/README.md)
adds authored separate pages and shared checked native float persistence. Its
ordinary Windows build succeeds; native Audio 31 and Video 23 pass without
failures/skips, with float-store 9, Interface 14, Playback 23 and controller 5/11
regressions. Original General objects replay against the actual store at 29/0/1,
31/0/1 and private required fault 3/0/0. A same-object native Audio pair reproduces
the minimum-value rounding defect and verifies its repair without widening integer
bounds. Actual PCM, snapshots, five picture filters and locked-save recovery are
covered within the recorded fixture scope. Audio/Video components pass 17/15;
three desktop journeys quit normally and preserve human files, restoring custom
EQ and gray/black/gray defaults, preserving current playback and showing lower
validation errors. Nine Audio controls, wallpaper, eight remaining categories,
actual picker/Reset UI, full hardware/media/DPI/Figma fidelity and clean beta
packaging remain open. New-head CI is pending with 28 Windows commands.

The user authorizes Figma inspection and asks that MCP calls be conserved. MCP access was restored on 14 October with a Professional Full seat: page inventory and high-fidelity Home, New Playlist, subtitle-timeline and its original settings-toggle context/screenshots now read successfully. The [saved MCP inventory](juxtopposed-references/figma/20261014/README.md) identifies the single Page 1 and canonical nodes; metadata alone does not establish visual acceptance. The separate saved browser-site block remains unresolved and must not be circumvented. Root owns MCP access checks and shared integration/build/Git/package work; reuse cached references before requesting more context.

Original design and artwork belong to [Juxtopposed](https://www.youtube.com/@juxtopposed). Source references are the [community design](https://www.figma.com/community/file/1691465999134370980/vlc-redesign), [accessible design copy](https://www.figma.com/design/zdNUYRVOH9lEtIxMYvh8DV/VLC-Redesign--Community-), unchanged manual exports and the supplied local walkthrough. Preserve designer artwork bytes, provenance and the existing CC BY 4.0/Inter/VLC license materials.

## Evidence baseline and status rules

The [coverage ledger](juxtopposed-references/coverage-ledger.md) contains 32 surface rows. The [advanced manifest](juxtopposed-references/advanced/manifest.json) preserves 13 exact-time original frames and the video SHA-256 `70b0ccfa8276acd8fbdf876b96233ebde13aab922acb4f618ef7c31c422aff08`. [Home](juxtopposed-references/home/manifest.json), [library](juxtopposed-references/library/reference-notes.md), [settings](juxtopposed-references/settings/implementation-checkpoint.md) and [queue](juxtopposed-references/queue/manifest.json) provide additional references. Walkthrough screens are reference media, not implemented application chrome. Editorial zooms do not establish canonical dimensions.

The latest specific checkpoints take precedence over older summary status:

- [Runtime controls](juxtopposed-references/runtime/player-controls-verification.md): actual floating controls, paused seeking, Back/retained player and fullscreen pass within the recorded fixture scope.
- [Home desktop](juxtopposed-references/home/desktop-checkpoint.md): real video/audio history, See All, search/clear, sorting, view switching and retained audio return. The [latest separate Home native helper suite](juxtopposed-references/advanced/new-playlist-checkpoint.md) passes all 11 checks, including real audio-only roots/events/restart and blocked-store/clear handling. The [later actual audio-only folder journey](juxtopposed-references/home/audio-folder-desktop-20261014/README.md) passes native Add Folder, two-track folder/player routing, play/pause/Back and card/contents after normal quit/restart in the existing profile. Fresh-user first-folder setup and complete visits/reset/opt-out/error journeys remain open.
- [Music](juxtopposed-references/advanced/music-player-checkpoint.md): real source-folder model, metadata, waveform, timer and desktop seek/return/media replacement; actual artwork blur, lyrics and native DPI/other formats remain open. A separate dark VLC fallback is prepared; native acceptance is still needed.
- [Subtitle presets](juxtopposed-references/advanced/subtitle-preset-checkpoint.md): native persistence/selection/rendered output, actual normal-lock controller and appearance/duration regressions pass. Integrated footer Save/apply/restart is unproved; a settings popup navigation/input problem is under investigation.
- [New Playlist](juxtopposed-references/advanced/new-playlist-checkpoint.md): original 9 native passes preserved; latest suite has 10 native passes, alongside 14 earlier broader component/source suite passes. Eight actual desktop captures establish title/image/media pickers, reordered repeated audio/video/video save, copied Home cover/route after restart, and repaired individual/all-unknown `--:--` durations on Qt build `88452491…ed99c6`. Native Other/`UNKNOWN` classification is preserved. Active-primary preservation, desktop drop/empty/cancel/errors and DPI/light/full fidelity remain open.
- [Network](juxtopposed-references/advanced/network-dialog-checkpoint.md), [waveform](juxtopposed-references/advanced/waveform-checkpoint.md), [video transform](juxtopposed-references/settings/video-transform-checkpoint.md), [subtitle appearance](juxtopposed-references/settings/subtitle-appearance-checkpoint.md), [subtitle duration](juxtopposed-references/settings/subtitle-duration-checkpoint.md) and [audio drain](juxtopposed-references/advanced/audio-drain-checkpoint.md) retain their separate native/component limits.
- Folder-bootstrap and unknown-duration corrections are integrated/built with the specific helper and desktop evidence above; those passes do not finish all Home or playlist journeys. The selected-local-UTF-8-SRT cue service is mounted in the native controls and passes twelve native controller cases; embedded cues, renderer expiry and the complete desktop timeline journey remain open. The FLAC conversion controller passes eleven native cases, including literal percent-character paths. Its separate [desktop proof](juxtopposed-references/native/conversion-desktop-20261014/README.md) records actual pickers, completion and an independent full PCM match on the preceding GUI build. The form is a bounded shared-style adaptation without a confirmed original Figma form; all profiles and complete Convert/Save remain open.
- The [leaf-route proof](juxtopposed-references/native/navigation-leaf-20261014/README.md) and [mounted navigation journey](juxtopposed-references/native/navigation-desktop-20261014/README.md) repair nested property forwarding and verify Home/Videos/Playlists, real empty/mixed playlist contents and restored second-playlist selection. All seventeen local component/source suites pass. Broader Music/Network history, rapid generations, native pointer restoration, reference fidelity and final packaging remain separate gates.

For each row record four independent statuses: **functional implementation**, **native app verification**, **reference/visual acceptance**, and **final package verification**. Keep supplied-state QML, injected failures, native QObject/core tests and actual Windows journeys labelled separately. A newer source or DLL does not automatically inherit the older package's results.

## Reference work first

- [ ] Reconcile the successfully read single-page MCP inventory's actual frames/variants with every unique screen, overlay, modal, menu and reusable control in the 32 existing rows. High-fidelity reads now succeed; keep complete surface/state reconciliation open rather than treating metadata inventory as design acceptance.
- [ ] Prioritize complete Global Settings/categories, profile/customizer/shortcuts, conversion/progress/profile selection, subtitle scope/timing interactions, first-run sequence, music category content, sidebar pins and light states. These gaps affect beta scope or backend choices.
- [ ] Export original icons/assets and record exact node, dimensions, tokens, state, source date and hashes. Keep new exports separate from old reference bytes. Do not guess node IDs.
- [ ] Resolve Home's cut-off lower grid, full player/library coexistence, detached queue, narrow/fullscreen and missing-art/error states. If Figma does not specify a necessary state, document a consistent functional adaptation and its provisional fidelity.
- [ ] Confirm whether timeline dragging changes overall delay or individual cues, what its zoom/navigation controls do, and whether text editing/export is actually designed. The preserved strip/narration confirms timing interaction; text editing/export is still unconfirmed.
- [ ] Confirm all conversion/streaming controls and profiles before specifying the final form. The 13:10 material previously labelled conversion is the network dialog. A menu entry is not a form reference.

Preserved authoritative nodes include Queue `463:831`, Bookmarks `6320:13784` and Chapters `6321:14045`, each 400 × 946. The new MCP inventory additionally establishes Home `6261:15078` (1920 × 1080), New Playlist `6259:14954` (713 × 589), subtitle control-bar variant `3744:12361` (1920 × 54), its original “Show Subtitle Timeline on Seekbar” toggle `5187:24487`, and the 13 Global Settings category frames. These IDs are grounded in successful reads, not inferred from the video. Context and screenshots remain required for each implementation.

## Every known design surface

All rows below remain unchecked for **complete beta acceptance**, even where existing checkpoints prove substantial behavior. Each needs the shared state/visual/package gates later in this document.

| Beta surface | Grounded reference | Required complete behavior and current gap |
| --- | --- | --- |
| [ ] Player transport and volume | Walkthrough around 01:30; preserved playback assets | Real previous/play/pause/next, mute/volume, duration/remaining time and available-state feedback; pointer, keyboard, wheel and engine-originated updates. Existing controls are built; complete media variants/focus/visual gates remain. |
| [ ] Main timeline and seek preview | Around 03:30; existing slider/assets | Accurate current/unknown/long duration, held drag/precise seek, preview, chapter/bookmark marks and paused/resumed clock. Core regressions pass; marks, preview fidelity and variants remain. |
| [ ] Queue | Node `463:831`; queue manifest | Native selection/filter mapping, play/remove/reorder/add/drop, jump to playing, sort/repeat/shuffle and save/stream selected entries; retain state across library/player/attached/detached routes. Native lifecycle, blur and final variants remain. |
| [ ] Bookmarks | Node `6320:13784` | Real add/seek/rename/remove/sort and media association; unavailable library/seek states and no stale actions after rapid media switches. Components/source review exist; full native lifecycle remains. |
| [ ] Chapters | Node `6321:14045` | Actual chapter selection/current feedback and timeline marks; keyboard and no-chapter states. Generated chapter fixture exists; native action acceptance remains. |
| [ ] Audio/subtitle track panels | Around 06:10; exact frame/node still needed | Actual selected tracks, primary/secondary roles, external subtitle open, empty/unavailable states, dismiss/focus. Final panel structure and track/media-switch acceptance remain. |
| [ ] Subtitle appearance and synchronization | Settings 07:50; advanced 08:35 | Every confirmed font/color/outline/background/offset/size-mode/scale/embedded-format/duration/delay/FPS field, default track and language. Native style/duration/controller paths pass; defaults/language/persistence/full menu variants remain. |
| [ ] Playback settings | Settings 09:10; advanced 09:08 | Four-tab navigation and native speed/numeric/slider synchronization; inspect reverse, sleep timer, metadata and any additional designed options. Their placement/complete behavior is not established by existing speed controls. |
| [ ] Video defaults and effects | Settings 09:00 | Flip/rotation/zoom/aspect/crop/deinterlace and every additional confirmed HDR/resolution/effects field. Existing rendered transforms pass; remaining fields and hardware/non-square/media-switch/persistence remain. |
| [ ] Audio equalizer and effects | Settings 08:50 | Native stereo/mix/delay, preamp, all bands, preset/two-pass state and other designed audio effects. Real PCM check passes; full desktop/persistence/effects layout remains. |
| [ ] Shared navigation | Library 11:45/11:55 | All labelled destinations, expanded/collapsed rail, native history, scanning/unavailable state, keyboard and stable route/selection. Inferred widths need original measurements/assets and final native variants. |
| [ ] Shared title/window strip | Library 11:45/11:55 | Home/menu/title/window controls, panel coexistence, maximize/fullscreen/minimize/close and focus. Current Home entry works; full original shell presentation remains. |
| [ ] Library header | Library 11:45/11:55 | Native Back/Forward, breadcrumbs/count, sort/view/search/main menu and correct selection under filtering. Built adapters and limited desktop passes; cross-category and variant acceptance remains. |
| [ ] Home and Add Folder | Home 13:47/13:49.3/13:56 | All five real sections: Continue Watching, Listen Again, frequent folders/playlists/artists; See All, activation/resume, valid visits, clear/reset, opt-out, deletion/unavailable IDs and restart. Audio-only bootstrap is integrated/built with 11 native helper/event passes; verify full service/picker/first-folder playback and remaining visit journeys. |
| [ ] Sidebar pins | Home 13:42; narration 13:37–13:43 | Ordered stable media/folder/playlist/artist/source descriptors, pin/activate/reorder/unpin and restart. Native favorites alone do not supply the designed heterogeneous order. |
| [ ] Shared playlists and contents | Library navigation; detailed content reference needed | Real mixed-media collection/content layouts, selection/sort/search, creation/edit/order/removal/open and playback. Creation backend is built; complete content layouts/actions and active-player independence remain. |
| [ ] Video library | Library 11:45/11:55 | Referenced grid/list/folder/group/player variants, genuine metadata/thumbnails, filtering/selection/drag and play/return/resume. Index/search/activation passes; full layouts/groups/native variants remain. |
| [ ] Music category landing | Library 12:10 | Add/Songs/Albums/Artists/Genres routes, counts, sorting/view/search and proper icons. Components/native entry exist; every category transition needs final native/visual acceptance. |
| [ ] Songs, Albums, Artists and Genres content | Library 12:10 plus Figma content inspection | Complete referenced collection/detail views with real audio/album/artist/genre models, selection, drill-down and playback/return. Compact Songs adapter is partial coverage; category/detail layouts remain. |
| [ ] Browse/local sources | Around 13:30; detailed reference needed | Actual source/folder traversal, unavailable sources, history, file activation and return; retain source identity. Existing browser infrastructure is not final redesign acceptance. |
| [ ] Open/source menu | Advanced 13:07 | Original menu structure/icons/shortcuts and real file/multiple-file/directory/disc/network/capture/clipboard/recent routes, save playlist, Convert/Save, Stream, Customize, Plugins/Extensions, Settings and About. Native providers exist; redesigned menu/dialog coverage remains. |
| [ ] Open Network Stream | Advanced 13:12 | Ordered URL validation/append/play and optional named mixed playlist, errors/retry/Cancel/focus; designed menu variants and supported protocol states. Backend/limited native modal passes exist; actual Save/playback and external protocol journey remain. |
| [ ] New Playlist | Figma `6259:14954`, 713 × 589; Advanced 13:28 | Actual picker/drop, ordered/repeated mixed files, title/optional copied thumbnail, busy/error/partial-save routes, no duplicate creation, native cover refresh and restart. Current compiled 670 × 554 layout needs alignment with the newly confirmed reference. Latest 10 native passes and observed picker/save/copied-cover restart pass their recorded scope; active primary, desktop drop/empty/cancel/errors and scaled/light/full visual gates remain. |
| [ ] Conversion and export | Convert/Save at 13:07; form not yet identified | Every confirmed source/profile/output/progress/cancel/error action, playable complete output and source/primary-playback preservation. Pending local audio→FLAC is only one bounded profile; final designed profiles and native journey remain. Do not use the classic route as the independent controller. |
| [ ] Subtitle attachment scope | Advanced 06:06 | This File/This Folder/This Playlist with real persisted subtitle association/default selection and precedence, invalid/moved-source behavior and restart. Current-file loading and per-media style presets do not implement folder/playlist subtitle-file scope. |
| [ ] Subtitle presets | Advanced 08:00 | New Custom/Default/saved selection/edited state, meaningful save/apply/error/restart and actual track/timing restoration. Service tests pass; native popup focus/input/Save/apply/restart remains a blocker. Unshown naming form remains provisional. |
| [ ] Subtitle cue timeline | Advanced 06:59; nearby local strip frames | Mount actual cue service, seek with revision/source guards, real timing-shift interaction and confirmed zoom/navigation controls; retain playback text/identity and supported-state explanation. Source parser is currently bounded UTF-8 external SRT and fails closed for embedded/image/multiple/FPS/duration effects. Inspect advertised design scope; extend native support where required rather than calling the file snapshot a rendered cue bridge. |
| [ ] Music player and waveform | Advanced 12:18/12:20 | Canonical cover/source-folder list/blur, actual artwork/title/artist, real waveform seek/elapsed/future bars, explicit queue fallback and designed lyrics/metadata/settings actions. Core/folder/clock journey passes; native artwork/lyrics/DPI/codec and visual gates remain. |
| [ ] Global Settings shell | Settings 17:50; advanced 18:48 | Real searchable App/Media/System category navigation, shared controls/help, persistent save/reset/error and Advanced Options. All 13 routes require their complete confirmed settings, not empty categories. |
| [ ] Appearance/interface settings | Global General and nearby frames | Language, Dark/Light/System and accent choices, true theme propagation through every surface, meaningful system-theme response and restart. Current dark literals do not establish complete theme support. |
| [ ] First-run theme setup | Settings 19:50 | Distinct Light/Dark/System cards, real preview/accent selection and Back/Next; inspect remaining setup steps and persist completion/selections. Fresh portable launch currently needs separate first-run acceptance. |
| [ ] Profiles/customization/shortcuts | Advanced 19:35 | Movies/Shows/Music/Work/New profile actions, original Customize Player and Keyboard Shortcuts screens; actual layout plus all designed preference scope, save/switch/restart/reset/conflict handling. Existing toolbar profiles are partial support. |

The beta source menu must preserve the entries confirmed in the original design and keep their existing native routes functional. A dedicated redesigned dialog requires an original reference; no unshown Open Disc, Capture Device, Stream or other vanilla-only form is added to beta by inference. Record those broader parity workflows for V1.0. A comparison shot of classic VLC or its Toolbar Editor is not evidence of a redesigned dialog.

## Complete Global Settings routes

The preserved audit confirms **13 category routes**. Inspect each original page, list every visible field/action and map it to native persistent state. All are open beta gates:

| Group | Required categories | Current specification limits |
| --- | --- | --- |
| App | [ ] General; [ ] Interface; [ ] Privacy; [ ] Controls | General visibly contains interface language, Dark/Light/System, accent swatches, confirmation/error dialogs, automatic updates, Developer mode, Save a log, Log file/Browse, Log details and Advanced Options. Other category contents need Figma inspection. |
| Media | [ ] Playback; [ ] Video; [ ] Audio; [ ] Subtitles; [ ] Library | Playback popup fields are partial reusable support; they do not finish global persistent category pages. Default subtitle track/language and library/history/scanning/reset policies need complete mappings. |
| System | [ ] Network; [ ] Performance; [ ] Recording; [ ] Extensions | Network9+Advanced and Performance14 are mounted with focused native controller/consumer/window and component evidence. Full category visual/platform gates remain. Complete original Recording12 (including Browse) and Extensions7 fields/actions are cached; implement their real consumers and checked persistence. |

- [ ] Maintain a per-field mapping: original label/state, native variable/model, units/range/default, configured-versus-live semantics, restart requirement and reset/error outcome.
- [ ] Verify ordinary Save does not unexpectedly replay a media preset or clear activity; Reset must clear the intended settings and live caches without queued-worker resurrection.
- [ ] Preserve signed microsecond synchronization values, inactive style/duration values, unknown media lengths and category inheritance; do not coerce them to unrelated defaults.
- [ ] Settings search must reach the actual fields/categories, retain edits correctly, and support short-window scrolling and keyboard access without leaking actions to playback.

## Milestones 0–8 retained, beta gates expanded

| Milestone | Beta finish criterion |
| --- | --- |
| 0 Reference/state inventory | Browser/Figma/video reconciliation accounts for every original surface/action/variant; unknowns and adaptations are explicit with exact provenance. |
| 1 Runtime foundation | Fresh/warm startup and ordinary launch, real audio/video/paused seek/resume, helper/plugin/resource availability and bounded shutdown pass on the final package. Retain repaired clock/drain regressions. |
| 2 Shared shell/Home | Full original shell, all Home sections and sidebar pins work with real data, visits/reset/opt-out/restart and retained playback. |
| 3 Library/discovery | Complete video/music/category/playlist/source layouts, metadata, search/sort/view/drill-down and all designed opening routes work. |
| 4 Player/queue | Full transport/timeline/tracks/chapters/bookmarks/queue, pinned/floating/detached/fullscreen states and stale-media/focus behavior pass. |
| 5 Settings/effects | Every confirmed popup/effect/synchronization action reaches native state and reflects native changes; persistent/live semantics and native popup usability pass. |
| 6 Advanced surfaces | Complete subtitle scope/presets/timeline, music/waveform/lyrics and confirmed conversion/export workflows pass with actual data and output. |
| 7 Preferences/appearance | All 13 settings categories, profiles/customizer/shortcuts, first-run flow and Dark/Light/System/accent behavior are complete and persistent. |
| 8 Beta integration/package | Final exact Git revision, native/component/visual journeys, fresh relocatable package, credits/licenses/build/reproduction notes and documented beta limits pass together. |

Recommended sequence: inventory and persistent/theme/profile model decisions first; finish native preset popup and full folder-bootstrap desktop journeys while cue/conversion backend tests proceed; port the complete categories/source menu/library details/scope/customizer; integrate cue and conversion presentation using confirmed references; then run the full final package/visual matrix. Keep one writer for the native build and integrate shared glue sequentially. Preserve known-good source/package snapshots between reviewable commits.

## Native, state and visual acceptance

- [ ] Every surface handles its applicable empty, loading, available, selected, disabled, error/retry, long-text, unknown-duration, keyboard-focus, short/narrow and media/source-change states using genuine data. An unshown visual state gets a documented adaptation, not a fidelity claim.
- [ ] Run actual video, audio, chaptered, embedded/external/multiple-subtitle, tagged artwork, missing-art, unknown-length and network fixtures. Verify pointer, keyboard, wheel/drop and accessibility actions on the final app.
- [ ] Exercise active playback through Home/library/queue/settings/pickers/New Playlist/conversion and return; preserve primary queue, selected item, track identity and clock where the action promises independence. Test paused state separately.
- [ ] Exercise native presets Save→edit→apply→quit/restart→Default; scoped subtitle attachment and source changes; chapter/bookmark lifecycle; Home audio-only Add Folder→visit→open→play→restart and reset while indexing; profile/theme/category persistence.
- [ ] Verify timeline source/generation/revision guards with actual normal-lock native tests and app actions. Displayed source-file snapshots must not be labelled decoder-rendered expiry; external file Reload does not reopen VLC's cached subtitle text.
- [ ] Require complete conversion samples and amplitude/output decode, real EOS/error/cancel, active-worker teardown, racing-destination preservation and actual primary independence. Retain the strict 88,200-sample FLAC regression; the existing 150ms duration tolerance cannot prove an intact tail. Native encoder selection and built-in drain/header defects need explicit results before conversion acceptance.
- [ ] Test cold/warm startup, quit with indexing/metadata/cue/conversion work, rapid A→B→A changes, missing/moved files, history opt-out/reset, unwritable persistence and stale queued results. No late callback may target a replacement source or resurrect cleared state.
- [ ] Review native captures at normal/narrow/short sizes and 100/150/200% Windows scaling in Dark, Light and System themes, with nondefault accent and pinned/floating/fullscreen/detached panels. Compare original geometry/typography/icons and native alpha/blur/rounding.
- [ ] Preserve the established visual target: canonical assessed surfaces at least 9.5/10 and assessed variants at least 9/10, with no critical defects. Record unassessed categories separately; a normalized component score cannot stand for whole-app/native fidelity.

## Conserved MCP validation budget

Figma MCP reads were restored on 14 October with a Professional Full seat. Reuse
the saved high-fidelity Home, New Playlist, subtitle-timeline and settings-toggle
context/screenshots before requesting more. The saved browser-site block is a
separate unresolved restriction; no browser read is established and it must not
be bypassed. If MCP becomes quota-limited again, continue from saved references
and retain the outstanding comparison gates. Do not repeatedly retry unchanged
metadata, infer new node IDs or retrieve already measured frames without a
concrete new question.

When read access becomes available, use a proposed ceiling of **8 planned validation calls plus 2 reserved correction calls** for this beta pass, subject to the actual remaining account limit. This is a work budget, not a claim of available quota or a task-wide token budget. Count calls conservatively and record tool, exact node, purpose, result and remaining allowance; follow the Figma skills before any applicable tool call.

| Priority | Proposed pair of validations | Reason |
| --- | --- | --- |
| 1 | Design context + canonical screenshot for a discovered Global Settings/appearance frame | Validate shared tokens/category shell/theme measurements affecting many pages. |
| 2 | Design context + screenshot for a discovered complete Home/library frame | Resolve lower-grid/shell/category geometry and original assets. |
| 3 | Design context + screenshot for the highest-risk discovered player/subtitle/music frame | Check timeline/timing/preset/cover layout after real implementation is reachable. |
| 4 | Design context + screenshot for a discovered conversion/profile/first-run frame | Use the remaining pair for the largest unresolved beta surface identified by browser inventory. |

Choose the actual four nodes only after a successful permitted inventory; unknown IDs stay unknown. Reuse saved context/screenshots for implementation and compare native captures locally. The two reserved calls are for a material unresolved mismatch, not routine repeated screenshots. If access or the budget cannot cover all surfaces, preserve manual provenance and pending validation rather than weakening native/fidelity gates.

## Publishable Git and package checklist

- [ ] Commit all necessary source, original/new documented assets, both build-system registrations, tests and reference evidence; no runtime dependency may exist only under ignored `build-windows`.
- [ ] Build the final selected Windows x64 configuration from the exact source revision with matching core/GUI/plugins/resources. Record toolchain/configuration, source revision and binary hashes. Recheck generated QML/JS dependencies and cache/resources from a clean regeneration.
- [ ] Run appropriate component CI and actual native regressions against that revision/package; record build/test results with no unexplained skips. Earlier checkpoint suite counts are not final-beta acceptance.
- [ ] Launch the unpacked package by its ordinary executable in a fresh directory/profile, without development environment paths or mandatory diagnostic flags. Resolve Qt/preparser/cache/plugins/fonts/assets/helper paths and confirm startup/video rendering without a blank window or missing plugins.
- [ ] The `qt-juxtopposed-controls` option already defaults to true in `modules/gui/qt/qt.cpp`. Verify fresh default launch uses the complete redesigned shell and first-run path; verify the explicit opt-out retains functional traditional controls and queue. Do not introduce a redundant default change without evidence.
- [ ] Verify relocation, Unicode/space paths, normal quit/restart and coexistence with installed VLC. Distribute a clean package, excluding private media-library/preferences, developer paths, logs/dumps and test fixtures.
- [ ] Bundle existing VLC/dependency/font/designer licenses and attribution, build/source instructions, exact Git revision, checksums, supported Windows/package requirements, beta feature matrix and honest known limitations. Preserve original artwork and label provisional derived VLC fallbacks separately.
- [ ] Decide and document portable-versus-installer distribution, package size/dependency completeness, version/beta naming and signing status. Test installation/unpacking/launch/cleanup for the selected artifact; an installer alone is not playback acceptance.
- [ ] Complete independent review of the final change and visual evidence, push the reviewable beta source revision, and create the chosen publishable artifact through Root's workflow once its actual gates pass. No publication or release completion is claimed by this plan.

Beta acceptance requires the confirmed redesign behavior, stability and one coherent verified package under the release boundaries above. Newly confirmed original design requirements update the beta inventory; vanilla-only parity belongs to V1.0, and additional extensions are V1.5 candidates. Unconfirmed text editing/export, unknown conversion geometry and unavailable category references remain explicit design questions. The beta still implements the complete confirmed redesign rather than the earlier one-action-per-surface slice.
