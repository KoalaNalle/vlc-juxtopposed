# Night mode saved defaults — 22 October 2026

The authored Night mode switch now has an independent native audio consumer.
It softens louder passages through VLC's existing dynamics engine, with its own
`nightmode` filter and parameter names. It preserves the separate Compressor
switch and all seven manual Compressor parameters. Enabling both stacks both
effects deliberately.

[Checkpoint and artifact hashes](checkpoint.json) qualify this selected consumer;
they do not accept the full Audio page or the beta package.

## Behavior and evidence

- The [maintained audio suite](audio-results.txt) passes **35 cases**, with no
  failures, errors, skips or QtTest diagnostics and a normal exit. This final run uses the
  repository's `run_audio_startup.py --suite defaults`, which now generates both
  required synthetic fixtures. It checks actual AMEM output, not a stored flag.
- [Measured PCM](audio-pcm.json) covers Off → On → Off using three native owners.
  Each returns exactly 529,200 mono frames at 44,100 Hz. Four settled one-second
  windows per owner each contain 44,100 samples. The loud On/Off RMS ratio is
  0.243614 (−12.266 dB); quiet passages retain their level, and Off restores the
  original output. Samples remain finite. Attack/release behavior means this is
  compression, not an absolute peak limiter.
- Edits made while each player is paused preserve its current audio pipeline;
  the saved default is consumed by a fresh owner after restart. The independent
  filter can be disabled without removing Compressor. Real file-write failure,
  rollback, retry and cold readback pass.
- [Profile tests](profile-results.txt) pass **15 cases**, including Night mode
  filter ownership, manual Compressor preservation and a separate cold process.
  The original wrapper incorrectly expected at least 16 cases. Its
  [original aggregate](profile-wrapper-original.json) remains false. The
  [reviewed verification](profile-verification-reviewed.json) checks the exact
  ordered inventory: 13 methods plus init/cleanup, all 15 passed. No test
  assertion or result was changed to obtain that review.
- The [provider-absent control](negative-control.json) omits only the new plugin
  and produces exactly three expected module-availability failures, then exits
  normally with test-failure status. It is not a passing product suite. The
  earlier stale-moc setup failure remains in the ignored work directory.
- [Ordinary UI observations](ui-observations.json) cover three clean processes:
  mouse On, Left Off, Right On, native config readback, cold On restoration,
  compact and full-width layouts, checkbox exposure and restart feedback. The
  final help-only revision removes a repeated restart sentence and was checked
  again through the actual tooltip and switch. Raw process receipts keep
  `passed:false`: successful exit alone does not verify UI behavior. The separate
  observation receipt records what was actually reviewed.

## Build and source binding

The final ordinary GUI is `9069f43a5209aaf6…`; Core remains `0404d1f6cd89ca77…`.
The Night mode plugin is `99c4dabc35fb4ebd…`. Full hashes are in the checkpoint.
The ordinary GUI excludes the private hover, UIA and diagnostic hook strings.
All three UI processes exited normally with unchanged source/runtime identities
and no matched native or Qt diagnostics. Base hard-linked binaries were
protected; replacement GUI files were unlinked before copying.

[R5 inputs](source-inputs-r5.json) bind 2,051 files. Actual copies of every input
were verified under `build-windows/beta-night-mode-20261022/source-r5` before the
later documentation update. The profile run retains its own earlier input
manifest. The build used existing dependencies, including the previously tested
Qt scrollbar patch. Cross-libtool emitted its existing uninstalled-host-path
diagnostics; compilation/linking completed with exit 0 and staged binaries ran.
The new Autotools provider and native test targets were exercised. Meson
registration was updated but was not built in this environment.

The maintained native commands are `audio-defaults-native` in
`modules/gui/qt/tests/native_library/audio-startup.mk`, followed by
`run_audio_startup.py --suite defaults` with a matching ordinary package,
executable, input manifest and fresh output directory. The package must include
the registered Night mode plugin. The profile target is
`player-profile-preferences-native` in `profile-preferences.mk`. Both fixtures
now prioritize their own generated moc metadata over stale main-build copies.

## Design and limits

The [Figma row reference](../../figma/20261022/audio-night-mode/README.md)
preserves the original Night mode row, toggle, typography and info icon. The
implementation adds behavior to the existing authored controls; it introduces
no replacement artwork or new layout.

The preset is peak mode, attack 5 ms, release 200 ms, threshold −18 dB, ratio 6,
knee 5 dB and makeup gain 0 dB. Those coefficients are an implementation choice;
Figma specifies the switch rather than a signal-processing preset.

This receipt covers the tested synthetic mono PCM and saved-defaults workflow.
It does not claim physical audibility, receiver passthrough, multiple real audio
outputs, every source format, Windows 10, complete screen-reader/OS-DPI coverage,
resource-cost acceptance or final packaging. Existing memory/performance receipts
retain their own earlier GUI/source scopes. Five Audio controls remain gated:
passthrough, Spatial audio, Spatial audio mode, media role and adaptive
visualization size. The full thirteen-category / 32-surface beta remains open.
