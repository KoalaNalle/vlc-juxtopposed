# Revised beta roadmap

18 October 2026. **Status: implementation and scoped validation are substantial;
the complete beta is not accepted.** This is the current execution order. The
[older beta plan](juxtopposed-beta-plan.md) retains historical receipts and
reference requirements; its old pending labels do not override newer evidence.

## Target and boundaries

Bring Juxtopposed's **confirmed redesign** to life with real VLC behavior,
consistent visuals, persistence, usable interaction and one reproducible Windows
x64 package. Complete the designed actions rather than a demonstration action per
screen. Preserve working native routes and traditional-control opt-out.

Beta covers the confirmed design and stability. V1.0 covers broader vanilla VLC
parity and the disc/ISO/Explorer/AutoPlay discussion. V1.5 may explore additional
YouTube/Twitch extensions. A designed menu entry must work; it does not require
inventing a redesigned dialog that the references never show. Conversion's
current functional adaptation must remain clearly identified where no original
form exists. Unconfirmed subtitle text editing/export is a reference question,
not an assumed beta feature. No designer outreach is part of this roadmap.

## Baseline to preserve

- Authoritative checkout: `C:\Modding\vlc-juxtopposed`, branch
  `juxtopposed-playback-bar`, committed checkpoint `5829010294` plus the current
  uncommitted work. Do not overwrite or discard those edits.
- [Home cache/audio receipt](juxtopposed-references/native/home-cache-audio-20261018/README.md):
  native cache/icon/window 16 processes / 3,297 checks; paired old-Qt negative;
  current audio concurrency32, actual WASAPI lifecycle16, ordinary Qt5 and HLS20.
  These are scoped results, not a cumulative feature-completion percentage.
- Protected ordinary baseline:
  `build-windows/beta-native-cache-home-20261018/ordinary-runtime-r3`;
  GUI `f82534592d35…`, MMDevice `0d29a257b562…`, Core `0cdebe2f6950…`.
  [Full hashes](juxtopposed-references/native/home-cache-audio-20261018/verification.json)
  bind results to the actual binaries and source.
- Interim interaction candidate: `build-windows/beta-compact-interaction-20261018/ordinary-runtime-r10`,
  GUI `19ec1a17ac1f…`, with the preserved core/audio/library modules. Six private
  native journeys and five ordinary playback smokes pass in their explicit
  scopes; see the [interaction receipt](juxtopposed-references/native/home-compact-dialogs-20261018/README.md).
  Historical physical Shift+F10 failed in R10. The scoped repair passes direct
  Windows input in ordinary setup R2; see the
  [repair receipt](juxtopposed-references/native/home-compact-dialogs-20261018/native-keyboard-repair.json).
  The later [focus receipt](juxtopposed-references/native/home-keyboard-focus-20261019/verification.json)
  closes the scoped keyboard-return outline defect: Home ON/OFF188 each,
  Recovery321, compact video741/audio742, five ordinary smokes and direct
  Windows keyboard ring proof. Complete accessibility/visual/DPI/package
  acceptance remains open. This is not the final beta package.
- All 13 Global Settings bodies exist with native services and scoped journeys.
  Ordered pins, Library-OFF Recent, the original shared title strip and original
  Videos/Music icons are implemented. Revalidate their remaining states;
  do not restart these features from obsolete TODOs.
- Movable modal headers already exist for Network, New Playlist, Conversion and
  Global Settings. Their component proof is not actual Windows pointer/DPI
  acceptance. New Playlist's source already uses the original 713 × 589 geometry.
- The cleanup preserved source/Git/current candidates, migration backups and the
  Windows 10 kit. Historical identical build outputs now share NTFS storage;
  unlink/copy an old binary before changing it. Active tools and builds were excluded.

The [32-surface readiness snapshot](juxtopposed-references/beta-readiness-20261018.md)
separates known implementation, native evidence and remaining work. Earlier
native results retain their original GUI/platform scope until replayed against
the final candidate. Nothing here accepts the whole application's visuals.

## Ordered work

| Order | Work package | Concrete completion evidence |
| --- | --- | --- |
| 1 | Finish compact navigation and real dialog interaction | Actual native Tab/Shift+Tab, arrows, wheel over rows and padding, pointer focus, error/Retry and pin fallback at short/narrow sizes. The four modal headers move by real pointer input, remain reachable and preserve focus, playback and busy guards. Keep title drag/double-click, close and volume/mute regressions green. |
| 2 | Complete remaining confirmed workflows | Resolve each open row in the readiness snapshot using existing code first. Prioritize first-run setup and profiles/customizer/shortcuts; remaining library/category/detail and queue/bookmark/chapter/track journeys; subtitle scope/timing and conversion lifecycle. Audit every confirmed Global Settings field for a real consumer, correct live-versus-next-media behavior, persistence, reset and visible failure/retry. Implement gaps that this audit actually establishes. |
| 3 | Complete design fidelity and accessibility | Compare actual VLC captures with cached original frames/assets. Finish Home/library/player/Recent, music, queue, modals, setup and settings states. Check typography, spacing, original icons, colors, blur/alpha, control sizes and focus/accessibility. Resolve reference gaps explicitly. |
| 4 | Consolidate real-media and state regression | Run the designed journeys on one identified current tuple with representative real/generated media, playing and paused states, restart, rapid source changes, missing/moved media, failure/retry and worker shutdown. Retain exact output/PCM tests. Triage every new crash or normal-flow failure. |
| 5 | Produce and validate a clean beta candidate | Build from the selected fixed source using registered dependencies/resources. Stage an ordinary package with no private hooks/data. Replay the required native/component/visual/media checks on those exact packaged binaries. Verify default first run, relocation, installed-VLC coexistence, licenses, attribution, source/build notes and checksums. Prepare reviewable Git changes and the local distributable. |
| Separate lane | Windows 10 comparison | Run the prepared before/after kit when the original guest machine is available. Record OS/AUDIOSES versions and capture the exception context if the crash recurs. Never substitute a Windows 11 pass for this result. |

Work packages are ordered batches, not fixed-duration promises. Close defects
and gather evidence during implementation rather than postponing all validation
to the end. Do not repeatedly rebuild/retest unchanged accepted work. A product
change, new failure, changed binary or unresolved coverage question justifies
the next relevant check.

### First batch for the next goal

1. Check the preserved source/binary baseline and use the readiness snapshot as
   the backlog. Replace obsolete status labels only where a receipt supports it.
2. Extend the maintained owned-window journey for compact Tab/reverse Tab,
   wheel/list-padding behavior and pointer focus. Fix defects demonstrated by
   these checks without changing the authored layout unnecessarily.
3. Exercise actual header dragging and focus restoration in the four existing
   dialogs, including short-window clamping and an operation in progress.
4. Compile only changed targets, run affected native journeys and ordinary
   playback, inspect the native captures, and save a matched source/binary receipt.

This batch should finish with concrete interaction coverage and a reviewed next
gap. It must not consume the entire goal merely writing another plan.

## Acceptance rules shared by every work package

Keep four independent statuses for each surface/action: **implementation,
native verification, reference/visual acceptance, final-package verification**.
A component render or native backend fixture does not establish a full desktop
journey. A new GUI does not inherit every older GUI's pass automatically.

- Exercise applicable empty/loading/selected/disabled/error/retry states, long
  names and unknown durations. Check actual pointer, keyboard, wheel and drop
  behavior where the design offers it, including accessible names and focus.
- Preserve the primary input, queue, track identity and clock during navigation,
  dialogs and background jobs that promise independence. Check paused behavior
  separately from playing behavior and verify real restart persistence.
- Cover video, audio, chaptered media, multiple audio tracks, embedded/external
  subtitles, tagged and missing artwork, unknown-length/live inputs and conversion
  output. Use the authorized `D:\Media` collection read-only; keep private names,
  frames and source bytes out of Git and distribution.
- For subtitle timing, distinguish a local SRT snapshot from decoder/renderer
  truth. Confirm source/generation guards and the designed timing interaction;
  do not advertise unverified embedded-cue or editing semantics.
- For export, verify playable output and complete samples, cancellation, failed
  publication, destination preservation and teardown. Retain the strict
  88,200-sample FLAC check; duration tolerance alone cannot prove the tail.
- Use normal, narrow and short windows; Dark, Light and actual System changes;
  nondefault accent/font; 100/150/200% **Windows** DPI and cross-monitor movement;
  and applicable pinned/floating/fullscreen/detached states. Component `uiScale`
  is not OS-DPI evidence. Use representative combinations plus all known risky
  intersections, rather than an unbounded Cartesian product.
- Capture the actual app for visual review. Preserve the prior targets of at
  least 9.5/10 for assessed canonical screens and 9/10 for assessed variants,
  supported by named geometry/asset/type/color/interaction criteria. These are
  review targets, not automatic scores from test counts. Unassessed states stay open.
- Accepted runs require normal termination, no forced cleanup, no unexplained
  native exceptions/assertions or Qt/QML warnings, and no lost/resurrected state.
  Keep intentional negative controls and rejected attempts clearly separate.

## References and practical constraints

Use preserved Figma context/screenshots/assets, the original walkthrough and
exact-time extracts. The additional Figma read allowance is currently **zero**;
the last two approved reads were spent on the original sidebar icons. Do not
infer a renewed allowance from the subscription or historical 8+2 proposal.
Continue local comparison and list the exact unresolved reference questions.
If a further budget is approved, reserve calls for those questions and final
validation, recording node, purpose and outcome. Follow the Figma skill when used.

Use monitor 2 for visible app checks. Follow the Computer Use skill when direct
desktop control is needed. Prefer maintained native harnesses for repeatable
checks. Keep isolated test profiles and normal user settings separate. Do not
lower UAC, disable protection or change firewall policy to make testing easier.
Only request user help for a prompt or decision actually encountered.

Use the registered `VLC-Juxtopposed-Home-20261018` build distro and current home
paths; keep the original distro and migration copies. Start native builds with
two workers and one writer to a build tree. Historical guest E:-only paths and
one-worker/4-GiB limits are not the current home configuration. Do not install
new dependencies without authorization.

## Deferred checks and release decision

The original Windows 10 AUDIOSES exception remains open. A real teardown race
was repaired and tested on Windows 11; the original exception context is missing
and causality is not proven. The user explicitly deferred the unavailable
Windows 10 machine. The verified 28.7-MB comparison kit is ready under
`build-windows/beta-native-cache-home-20261018` with before/after modules and
instructions. This deferral does not stop the Windows 11 work packages.

The next goal can deliver the complete implementation and a Windows 11-tested
beta candidate with this external check clearly deferred. It cannot claim full
Windows 10/11 acceptance or close the historical crash. Before publishing a
beta that claims both platforms, complete that check and resolve any failure,
or obtain an explicit user decision changing the advertised support scope.
Physical audio-device loss and a real cast receiver also need truthful separate
coverage statements when those paths are advertised; do not require every
possible hardware/service combination to be exhaustively proven.

The final local candidate needs a supported-features/limitations matrix, exact
revision and hashes, clean startup and relocation proof, dependency/font/designer
licenses and credit, reproducible source/build instructions and independent
change/visual review. Package size is secondary to a working licensed beta.
Choose portable ZIP versus installer and version/signing status explicitly;
do not spend the implementation goal optimizing package size or adding an installer.

Prepare a concrete reviewable commit/package before any final approval step.
This planning request does not authorize a new commit/push, tag, GitHub release,
external upload or message. Obtain publication authorization when the result is
ready, and never rewrite history or discard unrelated work. The
[prepared goal brief](juxtopposed-beta-goal.md) preserves the full scope and
deferred-platform boundary for the next goal.
