# Current settings component replay — 19 October 2026

All thirteen category components pass their scoped presentation checks. This
does not accept all native field consumers, actual Windows interaction, visual
fidelity, OS DPI or the final package.

The shared modal host now imports its production player preview. Ten older
category workers did not register that dependency, so their component engines
could no longer load the current host. Each now registers the actual production
`JuxtopposedPlaybackBar`; no substitute bar, assertion removal or warning filter
was added. Audio was corrected and checked in its separate
[18-group replay](../home-audio-consumers-20261019/README.md).

The other twelve category workers pass against frozen current production and
fixture inputs. A separate observer forwards Qt messages to each worker's
original handler and records them through interpreter exit. Every run exits0
without forced cleanup or an observed Qt message. The full
[receipt](verification.json) records per-worker hashes and source identity.

| Category | Existing declared groups/checks |
| --- | ---: |
| General | 13 |
| Interface | 10 |
| Playback | 15 |
| Audio, separate receipt | 18 |
| Video | 15 |
| Subtitles | 15 |
| Controls | 8 |
| Library | 11 |
| Network | 7 |
| Performance | 6 |
| Recording | 6 |
| Privacy | 8 |
| Extensions | 2 |

These groups overlap shared-host behavior and use supplied native-shaped
controllers. Counts are not a feature-completion percentage or replacement for
the per-field consumer audit. In particular, the disabled native acceptance
gates remain disabled; a mock's available control does not enable the ordinary
app's setting. Extensions' two checks only establish loading and required-property
semantics, not a real installed-extension lifecycle.

The full inputs, ordinary worker logs, late-message observations and generated
captures are under `build-windows/beta-settings-audit-20261019/component-r1`.
No actual desktop window was opened, no firewall prompt was handled, and no
security setting, dependency, commit or publication changed.
