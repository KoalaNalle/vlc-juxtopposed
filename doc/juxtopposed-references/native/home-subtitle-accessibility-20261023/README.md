# Subtitle control accessibility and saved-delay help — 23 October 2026

The ordinary GUI now exposes SpinBoxExt numeric bounds through the property
names read by pinned Qt 6.8.3 accessibility. The old Windows value route could
silently ignore nonzero requests because missing bounds became zero. Individual
primary-delay, secondary-delay and speed controls now have accessible names.
The saved default-delay row uses its existing info affordance and backend help
to explain that it applies to the next media. Native player code is unchanged.

## Evidence and limits

- The retained old ordinary desktop observation reports a nonzero write no-op:
  [previous receipt](../home-paused-subtitle-delay-repair-20261023/ui-observations-r4.json).
- Read-only pinned Qt sources show property lookup, Windows range validation and
  the string provider's silent out-of-range return. The isolated stock Qt control
  records missing bounds before the aliases and -10000/10000 after them. It is
  not VLC or Windows UIA. The first no-window preflight failed and is retained.
- Fifteen actual Subtitles/shared-host QML component cases pass without captured
  messages, including clicking the real help control, checking next-media text,
  and proving no settings request is generated. Its supplied model and scale
  variants do not establish native persistence, Windows DPI or visual acceptance.
- Two actual ordinary desktop processes use the same newly compiled GUI and the
  previously accepted ordinary Core. ASS accepts 0/+1200/-1200 ms through Windows
  accessibility with expected paused captions; Reset, 50 ms keyboard adjustment
  and an unchanged value after an excessive +10050 ms request are observed.
  All three controls expose individual names. Actual saved-default help appears
  and its editor stays at1200 while current timing is50; native sub-delay stays12.
- The ASS run exits0 but is NOT a clean audio/playback regression pass. Its log
  records real endpoint removal/default-output disappearance and audio errors.
  It reaches EOF and Home, then quits normally. Physical cause and full device-loss
  recovery are unproven. The failed audio result is not discarded or waived.
- The subsequent MP4 run accepts 0/+1200 ms, changes the held caption correctly,
  resumes with captions at25 seconds and quits directly during playing. It exits0
  with no checked runtime errors. No audibility or endpoint-loss proof is inferred.
- Source, runtime and generated fixtures remain unchanged during both runs.
  Visible held-frame patterns/coarse elapsed are retained; normalized progress
  reconciles by about1us in some adjustments, so exact GUI numeric conservation
  is not accepted. Secondary timing, subtitle speed and screen-reader use remain
  separate unfinished workflows. The unchanged Core's35 prior native passes keep
  their earlier scope; they were not rerun or counted as new GUI passes.

## Build and continuation

The ordinary overlay target recompiles all three changed QML units and links the
regular plugin object list without private hooks or QtTest. Existing static-library
portability warnings are retained. Runtime identity is recorded in both GUI parent
results: GUI9dd58c64 / Core7ab80750 / MKVf46d8d15 / WASAPI4e4e.

The2407-input freeze verifies every current source and its actual snapshot copy.
It reuses2394 unchanged immutable copies and writes only13 new copies to avoid
duplicating whole source snapshots on C:. The four changed source/test preimages
and final files are included here. Raw local logs remain in the ignored workspace;
portable log reviews retain their hashes and redact endpoint identifiers.

Continue per-track/secondary delay, subtitle FPS/speed and estimator contracts,
then the remaining settings/workflows,32-surface design/accessibility/OS-DPI pass,
consolidated media, physical-output checks, independent review and clean package.
The FPS control's initialization and secondary-delay notification use different
scale directions; verify the intended native contract before editing it.

The controlled hover hold passed previously; its original historical loss still
has no confirmed cause. Full thirteen-category/32-surface beta remains ACTIVE.
Windows10 is separately deferred. No install, commit/push, release/upload or shutdown.
