# Current status

Updated 28 October 2026. The beta goal is paused at the owner's request and unfinished; all six gates
in [BETA](BETA.md) remain open. No beta release or merge to master has been made.
The owner requested an earlier stop at about 21:31 Helsinki with about 5% usage
remaining, superseding the planned midnight stop. All owned processes are closed.
Resume only on the owner's request; desktop availability alone does not resume work.

Desktop control reported a physical Escape stop at about 20:48 local. Desktop
testing stopped; no further Computer Use input was sent. That first R2 input
freeze was terminated safely before any native mutation. The active goal then
continued code/native work. R2 stopped before compilation: all 536 files changed
only their WSL device number (70 to 69), while content and other metadata matched.
A single-session launch then passed that full guard. R3 stopped before mutation
on a build-helper dependency parser error. During fresh R4 preparation, a child
diagnostic `make -n -B` unexpectedly rewrote generated Qt dependency records.
**Hold shared native builds until those records are repaired.** No R4 build ran.
The owner has now made the desktop available again. Complete dependency recovery
before the next ordinary app build/test after an explicit goal resume.

Recovery audit: 424 `.Plo` and 707 `.Po` files became 8-byte placeholders;
11 `qml.deps` were regenerated. All 1,142 files have verified incident copies in
`build-windows/beta-record-feedback-20261028/ordinary-preparation-r4/damaged-generated-snapshot-r1/`.
All 1,745 authoritative Qt/include source contents and all 536 preserved ordinary
runtime files are unchanged. Recorded compiled objects, original Qt005 and nine
protected artifacts also match, apart from invocation-specific device numbers.
No Makefile/config/moc/rcc/generated-C++ change was observed in the command window.
Direct compiler `-M` proof now reproduces both selected dependency files exactly
(927/498 prerequisites, original SHA `af923b3127be`/`16dc86373a15`), with stable
second passes and qualified header checks. No installation yet. The full mapping
finds 794 current compiled objects and 337 no-object targets; the latter's history
is unknown. The reviewed full batch was stopped for this pause: 70 first-pass and
70 second-pass files exist out of 794, without a complete generation receipt.
They are unqualified scratch; no shared dependency was installed and no object
was compiled. The nine protected build-file hashes still match. The stop receipt
is `build-windows/beta-record-feedback-20261028/full-qt-deps-recovery-prep-r1/stopped-run.json`.
All 11 QML fragments passed read-only template/resource checks. Next: prepare a
fresh, reviewed continuation of dependency-only generation, validate all compiled
target graphs, then review installation. The **unexecuted** installer proposal in
`qt-deps-install-prep-r1/` has a `Path(None)` bug for 35 unmapped no-object paths;
correct and re-review it before use. Another 302 known no-object paths remain absent.
Never use `-B` in a dry-run diagnostic again.

## Selected checkpoint

- Branch `codex/beta-20261028`, source checkpoint `a0ae6670ca`; source/receipts are committed
  and pushed to the matching origin branch. No release is ready.
  The requested temporary remote backup branch deletion is already complete.
- Alpha fallback: `master` and `codex/alpha-fallback-20261027` at `0694227ab8`.
  Keep those refs and the preserved ordinary runtimes.
- New Home candidate: `build-windows/beta-home-sections-20261028/history-actions-ui-r4/runtime`,
  Core `f09c45fe4075`, Qt `005fbe744643`, media library `e834a41a700b`.
  [Exact identities/scopes](../juxtopposed-references/native/home-video-history-actions-20261028/README.md).
  All 16 native checks pass. Ordinary r2 actually removes an unfinished item and
  clears Watch Again while search hides its entry; r3 cold restart keeps both
  lists empty and all three library videos present. R4 passes Watch Again removal
  and Continue Watching's Home Clear action in the other direction. All exit 0, 603 modules,
  no checked diagnostics, all seven guards. This GUI still predates the newly
  qualified mode-cache repair and Record checkmark; an ordinary rebuild is next.
  Prior ordinary fallback `ordinary_home_r2/runtime` (Core r12/Qt r2), other tested
  runtimes and the alpha refs remain preserved. Results do not transfer wholesale.
- Established home WSL toolchain; no new dependencies or security/privacy changes.
  Generated fixtures only in this follow-up; D:\Media remains read-only.
  Cold `native-r11` and FIFO probe R4 are verified on D: with C: junctions.
  Superseded history-action r1 build/native folders are also verified on D: with
  C: junctions, reclaiming about 480 MB. Active/fallback files remain on C:.
  Two older precise-seek r7 folders are also verified on D: with C: junctions;
  together today's two archives reclaimed about 3.32 GB. C: remains below 80%.
  Check headroom against the 399,864,390,452-byte free floor before each build/stage.

## Latest evidence

[Home receipt](../juxtopposed-references/native/home-video-sections-20261028/README.md):
Continue Watching contains unfinished videos and remains first; Watch Again is
secondary and starts completed videos over. Component checks and all 14 native
Home checks pass, including 300-row filtered pagination/count/search and checked
storage failure/retry. Ordinary indexing, natural completion, unfinished replay,
both full-list routes, typed search/clear, resume under general Never, retained
paused player and cold restart passed on their recorded Core/GUI pairs. General
Always was also exercised after restart. Final-package/design/DPI gates remain open.
The [history-action follow-up](../juxtopposed-references/native/home-video-history-actions-20261028/README.md)
adds scoped Clear/Remove, native preservation checks and the media-library wake
repair; its component, native and ordinary menu evidence have separate boundaries.

[Subtitle/recording receipt](../juxtopposed-references/native/home-subtitle-refill-20261028/README.md):
Core r5/libass r2/text demux r1 pass eight scoped native FPS checks, ordinary
live/paused timing and copied-preset cold restoration/file switching. ASS
recording/replay and forced non-ASS Matroska subtitle exports/durations pass with
avcodec r2. Those paired outputs contain all 1500 video frames but lose the same
528 source audio samples. The audio-only comparison preserves the entire tail. The
[FIFO observation](../juxtopposed-references/native/recording-tail-fifo-20261028/README.md)
now confirms the missing 619-byte/11-ms packet is still queued at audio deletion;
output remains the exact 625-packet source prefix. All 1500 video markers and both
subtitle exports pass. This diagnostic receipt remains frozen. One native playing
ASS/FPS/EOF-revival regression also passes on Home Core r12. Other earlier results
do not automatically qualify it.

[Tail repair](../juxtopposed-references/native/recording-tail-drain-20261028/README.md):
codec `79a565a93987` on Core r5 passes 41 native queue cases/5,772 checks and real
Matroska in both actual A/V deletion orders: all 626 FLAC packets, 2,880,528 exact
PCM samples, 1,500 video markers, both SRT exports and timelines. The old control
reproduces the loss. Deliberate header/closed-pipe failures exit normally with
expected errors and intact guards. Separate TS/short-Matroska checks also pass.
The [duration follow-up](../juxtopposed-references/native/home-subtitle-refill-20261028/avformat-duration-regression/README.md)
is now built: source `f2a90481441d`, codec `26e1b8cc539a`. Default MP4 retains all
60 video frames/116,736 PCM samples; TS, short Matroska and both long Matroska
orders also pass source-exact checks. All jobs are closed and guards pass.

[Recording error feedback](../juxtopposed-references/native/recording-error-feedback-20261028/README.md):
a real LibVLC callback control passes, successful avformat output produces no
error, and a deliberately closed output pipe logs failure but produces no error
callback. All three controls exit normally with eight preservation guards.
This confirms a feedback gap; it does not establish final-tail-only failure or
conversion-result propagation. The bounded notification proposal is unimplemented.

[WebVTT callback qualification](../juxtopposed-references/native/webvtt-fps-20261028/README.md):
source `433da57aa248` passes eight actual demux/serializer cases, 314 checks;
the original source has 38 failing checks. This covers indexed rate/refill, inline
timestamps and finite-clock boundaries. Installed plugin is unchanged: plugin
build, real decoder/pixels, ordinary FPS controls and final package are unqualified.
TTML remains open. Hold the stale ignored plugin draft until shared-tree recovery.

[Earlier follow-up](../juxtopposed-references/native/home-followup-20261027/README.md)
and [upstream integration](../juxtopposed-references/native/home-upstream-integration-20261027/README.md)
retain fullscreen, timing-context, notifications, rebuild and regression evidence.

## Continue here

1. Clear/Remove implementation, ordinary mutation and cold persistence are saved
   in the history-action receipt. All owned Home apps are closed normally (r2
   PID 18992, r3 PID 1244 and r4 PID 31608 gone). Actual production-model worker
   [qualification](../juxtopposed-references/native/home-video-history-actions-20261028/worker-lifecycle/README.md)
   passes six data rows plus setup/cleanup, all guards, no forced cleanup. It
   exposed a real live-mode query-cache defect; minimal setter repair
   `cc7ede0057e3` passes, with both failing controls preserved. Logical missing-ID
   failure/retry does not establish real disk-error recovery.
   Continue live track preference
   changes. Native Menu/reopen passes; synthesized Shift+F10 and Alt+F4 delivery was
   inconclusive. Title/chapter MRL ranges and truncated inputs also remain open.
2. Continue W1: remaining external formats (WebVTT/TTML lack the text demuxer's FPS
   path), defaults/language/role combinations, estimators, new preset save-error/
   retry and secondary-monitor capture. The repaired recording tail is qualified
   on its scoped native tuple; integrated ordinary checks remain open. R4's probe build/run are terminal
   exit 0; shared source/artifacts are restored, all guards pass, no forced cleanup.
   The queue repair and adapter error guards are built/qualified on their scoped
   Core5 tuple; the AV packet-duration follow-up also qualifies default MP4 and
   affected Matroska/TS paths. Its first preflight had a wrong test lookup path,
   with no writer/mutation; fresh `adapter-duration-r1/native-qualification-r2`
   compiled/relinked successfully. Native Record menu checkmark/input feedback
   is source-only in `menus.cpp`. Rebuild ordinary GUI with it and cache repair,
   using the reviewed 512 MiB debug-stripped producer. R1 stopped at its exact
   baseline-inventory guard before any native build or shared-file mutation;
   a later full comparison was equal, so the differing observation is unknown.
   Fresh `beta-record-feedback-20261028/ordinary-preparation-r2` preserves that
   observation before the unchanged guard. Its completed manifest `72bae37bc996`
   covers 1,745 sources and the full 536-file baseline. R2 stopped before native
   mutation because only the invocation-specific WSL device number changed.
   R3 retained one WSL namespace and passed exact metadata checks, then stopped
   before mutation because its dependency parser included compiler phony rules.
   Fresh R4 fixes parsing of the first logical rule and is being checked against
   both real dependency files before another launch. Preserve original full-debug GUI and
   distinguish new Qt/Coref09/codec26 integration from earlier Core5 passes.
   Check selected audio-track timing,
   per-track overrides and seek ordering, then both physical endpoints if available.
3. Continue W2/W3 settings and surface journeys; conversion/library/subtitle actions,
   notification focus/screen reader, popup movement, themes and actual Windows DPI.
   Playback Record needs visible active-state feedback. Interface-owned Home/audio/
   classic F11/Escape and rapid fullscreen toggles remain open. Preserve accepted
   owner layout decisions and the thirteen-category [settings audit](SETTINGS.md).
4. W4/W5: integrated media/multiple-instance regression, independent review and a
   clean reproducible ordinary final package. Windows 10 comparison and unavailable
   physical devices remain explicit external deferrals. Follow every gate in BETA;
   this checkpoint does not establish beta readiness. The active goal requests a
   Git checkpoint/push; release publication remains a separate authorized action.
