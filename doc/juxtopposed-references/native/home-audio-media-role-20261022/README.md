# Windows Media role consumer — 22 October 2026

Media role now classifies new Windows WASAPI audio streams. Saving a role does
not change the selected endpoint or interrupt an existing paused player. The
ordinary Audio Defaults control is enabled when the selected output supports
this consumer; an explicitly selected different backend remains unavailable.

[Checkpoint and hashes](checkpoint.json) accept this selected consumer. The full
thirteen-category / 32-surface beta remains open.

## Evidence

- [Native Windows startup tests](startup-results.txt): 14 passed, no failures,
  errors, skips or QtTest diagnostics. Twenty real MMDevice/WASAPI sessions
  cover all nine roles. Paused-owner state, input, time and device remain
  unchanged after a saved edit; a fresh owner accepts the new category.
- [COM contract](unit-results.txt): 15 passed against the production helper,
  including all roles, unknown/empty/null fallback, missing/rejected interface,
  property rejection and exactly one release of an acquired interface. This
  fake-COM contract is separate from the actual Windows acceptance above.
- [Audio defaults](defaults-results.txt): 35 passed, including existing actual
  PCM/filter behavior, rollback and retry. [Profiles](profile-results.txt): 15
  passed, including complete Audio groups with Music and a fresh child restoring
  Communication. Each suite exits normally with zero skips or diagnostics.
- The [old-provider negative control](negative-results.txt) still plays actual
  buffers but fails exactly the intended category-acceptance assertion. It
  exits normally with status 1 and one expected failure; it is not a product pass.
- [Reviewed ordinary UI](ui-reviewed.json): four normal exits cover pointer
  Music, focused Down/Up selection, saved config readback, restart feedback and
  help, cold Communication restoration, disabled unsupported backend, and real
  ordinary Communication playback/pause. Its completed trace contains category 3
  accepted exactly once. Raw process receipts retain passed:false because exit
  alone is not UI acceptance. [Direct observations](ui-observations.json) retain
  the actual checks and helper limitations.

The earlier R2 observation failed because Windows buffered its file log until
normal shutdown. The old runner killed its child at 90 seconds before saving an
exit code. [That failure remains preserved](earlier-observer-failure.json).
The corrected observer checks current playback while it runs and reads the
completed classification trace only after normal teardown. It passes the old-
provider negative and new-provider positive controls. No failed run is relabeled
as a product pass, and final tests use no forced cleanup.

## Policy and build

| Saved role | Windows category |
| --- | --- |
| Video | Movie (10) |
| Music | Media (11) |
| Communication | Communications (3) |
| Game | GameMedia (7) |
| Notification | Alerts (4) |
| Animation | SoundEffects (5) |
| Production | Other (0) |
| Accessibility | Speech (9) |
| Test | Other (0) |

The Windows API [sets stream properties before initialization](https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudioclient2-setclientproperties).
The mapping uses the documented [stream categories](https://learn.microsoft.com/en-us/windows/win32/api/audiosessiontypes/ne-audiosessiontypes-audio_stream_category).
Production and Test have no exact category and use Other. The dynamic interface
query and rejected preference leave the existing audio initialization path
available. No offload or stream-options preference is enabled. MMDevice's
existing endpoint-selection role and default-device notification logic are
unchanged; classifying a stream is separate from choosing an output device.

The ordinary GUI is 33f10d62042fb117…, the WASAPI provider 4e4e33c04003ab55…,
and Core remains 0404d1f6cd89ca77…. Full identities are saved in the checkpoint.
The GUI links ordinary sources without private diagnostic translation units or
QtTest. All four UI processes preserve their source/runtime identities and
have no matched native/Qt errors. Base hard links were protected; replacements
were unlinked before copying. No new dependencies or Windows settings changed.

[Source scope](source-scope.json) distinguishes the native R3 inputs from the
ordinary R4 gate-enabling revision. The final R5 freeze has 2,064 verified input
copies. Its only delta from the ordinary source is distribution/build support
for the COM test; production code is identical. Earlier memory/performance
receipts retain their own GUI/source identities.

The opt-in COM target uses the existing MinGW compiler:

    make -f test/wasapi-role-native.mk WASAPI_ROLE_OUT=<fresh-output> wasapi-role-native

Run the resulting Windows executable natively. Integration targets are
audio-startup-native and audio-defaults-native in the maintained
modules/gui/qt/tests/native_library/audio-startup.mk; run_audio_startup.py
requires matching package, executable, source manifest and fresh output, with
--suite startup or --suite defaults. The profile target remains
player-profile-preferences-native. The package must contain the new WASAPI
provider and the previously qualified Night mode provider. Autotools was built;
Meson registration was updated but not built. Existing cross-libtool host-path
warnings remain; build/link completed with exit 0 and staged binaries ran.

## Design and remaining limits

The [Figma reference](../../figma/20261022/audio-media-role/README.md) preserves
the original Media role row, geometry, typography and SVGs. Fresh context is
available, but the source screenshot was clipped to1 × 1; pixel fidelity is not
accepted. No replacement artwork or layout was introduced.

This is classification and saved-defaults acceptance on the home Windows 11
machine. It does not prove audible driver policy, ducking behavior, physical
multi-output routing, receiver passthrough, every audio format, Windows 10,
complete screen-reader/DPI behavior, resource cost or final package readiness.
The helper's file-picker/control-focus limitations remain explicitly unaccepted.
Four Audio controls remain gated: passthrough, Spatial audio and its mode, and
adaptive visualization size. Actual thumbnail backdrop blur, other confirmed
consumers/workflows, broad design/accessibility, independent review and packaging
remain open. No commit, push, release, upload or shutdown was performed.
