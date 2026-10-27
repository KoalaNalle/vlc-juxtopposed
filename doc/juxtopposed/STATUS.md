# Current status

Updated 27 October 2026. The beta is unfinished; all six readiness gates in
[BETA](BETA.md) remain open. Work is continuing on the separate upstream
integration branch; no beta release or merge to master has been made.

## Selected checkpoint

- Branch `codex/upstream-20261027`; the follow-up builds use base commit
  `5994f80169715db8e4ad28b7492a362304b40bbe` plus the recorded source patch.
  The latest test runtime is
  `build-windows/beta-follow-20261027/runtime-r7`: upstream integration core plus
  rebuilt GUI, source patch `d8492912a94fabe585f7be3783a98b683097449ef6c20df134e86e941f367084`.
  Its manifest binds all binary identities; it is not a clean release package.
- Working alpha fallback: `master` and `codex/alpha-fallback-20261027` at
  `0694227ab8`, with the older runtime documented in the integration receipt.
  Both fallback and prior runtime passed manifest guards during r7 staging.
- The established home WSL toolchain remains in use. No dependencies, firewall
  rules or security/privacy settings were changed in this follow-up. D:\Media
  remains read-only; generated fixtures and private logs stay outside Git.

## Latest evidence

[Follow-up receipt](../juxtopposed-references/native/home-followup-20261027/README.md)
records the fullscreen and Home fixes, ordinary FPS and two-subtitle timing
checks, notification pointer/keyboard checks, source and runtime identities,
negative controls and capture limitations. The r7 native timing-context matrix
passed 16 rows and both audio revision tests, including rate/state ordering;
ordinary testing showed changing an audio delay cancels a pending mark and
permits a fresh pair. A shortcut-based attempt to change audio tracks with the panel open did not switch tracks, so
that ordinary case is inconclusive. Selected-track audio overrides and the
queued seek race still need investigation. The silent generated fixture does
not establish perceptual audio sync.

[Upstream integration receipt](../juxtopposed-references/native/home-upstream-integration-20261027/README.md)
retains the full rebuild and earlier regression scope. Scoped evidence does not
qualify the final package, the remaining surfaces, visual or Windows DPI gates,
or the Windows 10 comparison. Prior alpha and integration results apply only
to their identified binaries.

## Continue here

1. Repair the reproduced first live default-to-50 FPS change: old subtitle cues
   remain visible without a seek. A zero-sentinel flush guard is the leading
   hypothesis, not yet a proven fix. The proposed fresh-process native negative
   control is `build-windows/beta-follow-20261027/proposed-first-fps-playing-native-test.cpp.txt`;
   verify startup with saved FPS and newly added subtitle sources before changing
   the guard. Then continue W1 defaults/restart, formats and secondary-monitor capture.
2. Check the audio timing revision against track changes, per-track overrides,
   and seek notification ordering; reproduce before changing behavior. Verify r7
   on actual playback and both audio endpoints where available.
3. Continue W2/W3: remaining settings, conversion, library and subtitle actions;
   notification focus/screen-reader and visual acceptance, desktop scaling and
   popup movement. F11/Escape on interface-owned Home/audio/classic paths and
   rapid fullscreen toggles remain open. A separate r6 Home exit check showed
   the settled accessibility label correctly returning to Fullscreen.
4. Finish integrated media/multiple-instance regression, independent review and
   a clean reproducible final package. Windows 10 comparison and unavailable
   physical endpoints/receivers remain explicitly deferred until accessible.

## Saved question

Should Continue Watching retain all recent videos or only unfinished items?
The existing EOF fixture needs native count/order/search/event qualification
before adopting a resume-only policy. No policy change has been made.
