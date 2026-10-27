# Current status

Updated 27 October 2026. The separate upstream integration is rebuilt and tested.
All owned desktop/native jobs are closed. The beta remains unfinished; all six
readiness gates remain open. No release, push or merge into master was performed
for this integration task.

## Selected checkpoint

- Branch: `codex/upstream-20261027`. Upstream `874d229c18` is merged at
  `06c97dc786`; the toolbar policy repair is `c7cdbdb5d7`.
- Selected ordinary runtime:
  `build-windows/upstream-integration-20261027/runtime-r2`.
  It is a fresh complete debug install from production source `c7cdbdb5d7`,
  without private native hooks. Later branch changes concern tests, build
  declarations, CI and docs. Full binary identities are in the linked receipt.
- Fallback: `master` and `codex/alpha-fallback-20261027` remain `0694227ab8`.
  The older tested runtime is
  `build-windows/beta-single-frame-20261025-night/ui-record-error-fixed-r478/runtime`.
  All 532 fallback file hashes match their pre-integration manifest.
- The established WSL distro and compatible contrib cache remain in use. No
  dependencies, firewall rules or security/privacy settings changed. Human VLC
  settings and generated fixtures are unchanged. The four unrelated experiments
  remain untouched. Freeze fresh producers before the next job.

## Latest verified work

[Upstream integration receipt](../juxtopposed-references/native/home-upstream-integration-20261027/README.md)
records four conflict resolutions, the silent duplicate-transition repair,
source/binary identities, failures and accepted results. The full native build
and install pass after quarantining stale imported objects with dummy dependency
lists. The old objects remain recoverable in WSL; see BUILD before reuse.

Combined local evidence covers 31 Windows and four Linux workflow commands,
49 toolbar cases, four no-library Home variants and five tooltip cases. Native
checks pass 32 MMDevice checks, 16 real WASAPI lifecycle cycles/eight missing-device
fallbacks, 22 recording-start checks and 11 CSD model QtTest cases. Actual recording,
RGB paused seek, MP4/B-frame MP4 and dual-subtitle Matroska playback pass with
normal release. These are local results; this branch has no new GitHub CI run.

Ordinary redesigned and traditional players open/play/pause/seek generated media
and close normally with all guards passing. Redesigned fullscreen idle hide,
pointer exit/control retention and maximize/restore are observed. Traditional
Home's six native split-menu entries, picker and Ctrl+Q pass. The redesigned
Close callback emits one reviewed `window closed` diagnostic; its initial strict
observer failure is retained. Redesigned keyboard shortcuts remain unqualified
in this run. Persisted redesigned image comparison is also missing because the
capture helper only saved PNGs while the tool supplied JPEGs; saved accessibility
observations remain, and traditional JPEG captures were retained.

[Earlier recording-error work](../juxtopposed-references/native/home-recording-error-feedback-20261027/README.md)
and other dated receipts retain their exact earlier binary scopes. The old
[alpha CI receipt](../juxtopposed-references/ci/alpha-ci-upstream-20261027/README.md)
records the already-merged PR and green master checks; it does not qualify the
new integration branch. No beta gate is closed by these scoped results.

## Continue here

1. Review the integration receipt and qualify redesigned keyboard actions and
   persisted fullscreen/window captures on the selected runtime. Keep master
   and the old runtime as fallback until an integration merge is authorized.
2. Return to W3/W2 in [BETA](BETA.md) and [SETTINGS](SETTINGS.md): the error notice's
   Show Details pointer/keyboard/focus/timeout path, remaining settings and
   shortcuts, recording formats and mid-write failures, appearance, accessibility
   and actual Windows DPI. Live System-theme changes remain open. Cover view is
   a documented functional adaptation. HDR/unsupported independent Disc sort stay
   truthfully disabled.
3. W1's older secondary-fullscreen caption capture issue remains separate from
   this primary-caption pass. Physical output versus capture, sparse/long-cue
   cost, per-track combinations, MP4 indexes and live behavior remain open.
4. Online subtitle search/download needs implementation and the owner API
   decision. Windows10/audio endpoints/receiver/casting remain external deferrals.
   Integrated regression, independent review and clean-package/delivery gates
   follow the remaining behavior/design work. No Windows10 compatibility claim.
5. The owner authorizes test firewall rules with an exact cleanup journal; none
   were changed here. Preserve Private/LocalSubnet scope and enabled protections.
   The ignored `build-windows/firewall-ledger-20261027/` inventory and earlier
   backups remain available; refresh executable identities before future changes.

## Saved question

Should Continue Watching retain all recent videos or only unfinished items?
The earlier owned database includes an EOF fixture with position0.995987,
duration-1,play_count0. A resume-only policy needs native count/order/search/events
qualified before limiting. No policy change was made during integration.
