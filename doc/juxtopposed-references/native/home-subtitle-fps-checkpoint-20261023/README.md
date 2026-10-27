# Subtitle frame-rate checkpoint - 23 October 2026

Work is paused at the owner's request. The ordinary plugin builds successfully,
but this FPS workflow has NOT passed ordinary desktop or rendered-caption acceptance.
The last accepted desktop runtime remains the prior subtitle-accessibility build
(GUI9dd58c64 / Core7ab80750 / MKVf46d8d15 / WASAPI4e4e).

## Change and evidence

The native player uses 0 for normal external-subtitle timing. The old controller
started at 1; the before test reproduces that mismatch. The QML field also used
inconsistent scales and listened to secondary-delay notifications. The candidate
initializes from the native player, validates finite/nonnegative input, exposes
real FPS through an accessibility wrapper, listens to FPS changes and resets to 0.
The existing visual editor keeps integer thousandths internally. The popup help
now explains the external-file and normal-timing contract. Embedded subtitles
cannot establish this consumer's functionality.

The first native build failed to link CheckedPreferencesStore; the maintained
test source list now includes it. r3 and r4 each finish normally with ten passing
functions: setup, two FPS contracts, six existing secondary-delay contracts and
cleanup. These verify properties, queued notifications and native roles, not
visible caption timing. Both retain DX11/alpha-blending errors from the dummy
fixture. The attempted r4 hardware argument is obsolete and produces a warning.
It has been removed from current source; recorded r4 artifacts are unchanged,
and the restored fixture has not been rerun. Diagnostic cleanup remains open.

The inherited input manifest also omits test_player_subtitle_delay.cpp and
vlc_stub_modules.cpp. Its historical sourceInputsUnchanged flag covers only
enumerated files, not these test sources. Current test snapshots are included
here without retrospectively claiming exact frozen native build inputs. Before
accepting a rerun, freeze the complete native test dependency set as well.

The actual tabbed QML component script passes with its supplied model. Images
are retained; this does not establish actual Windows scaling or native behavior.
The ordinary r5 plugin links the regular object list without private hooks or
QtTest and exits 0; its SHA256 is recorded in checkpoint.json. No FPS desktop
process was launched. The earlier subtitle accessibility and paused-delay
acceptance retain their original scope and are not repeated or extended here.

## Resume

Refresh the source freeze after this checkpoint. run-ui-r5.py is saved preparation,
not an executed test, and its old freeze must not be treated as current. Stage
the new regular plugin into a fresh runtime without modifying the accepted one.
Verify actual external-SRT selection before testing 23.976/29.97/50/0 FPS, real-unit
accessibility writes, keyboard focus/arrow behavior, Reset and engine feedback.
Compare visible external cues before/after changing FPS at a held video frame;
immediate paused retiming remains unproven and may require further Core work.
Complete visual secondary-delay checks and wider lifecycle regressions afterward.

Raw runtime binaries and logs stay in the ignored local workspace. Portable
reports retain diagnostic hashes and relevant excerpts. The input manifests refer
to existing immutable local source copies; this receipt does not duplicate the
entire build tree. Historical failures are retained. Full beta, physical-output,
32-surface design/accessibility/OS-DPI, review and clean-package gates stay open;
Windows 10 comparison remains separately deferred. No desktop tests may resume
while the owner is using the computer.
