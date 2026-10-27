# Player profiles — 19 October 2026

The user confirmed playback, audio, video, subtitle defaults and player layout
as profile-owned. Appearance, keyboard/mouse bindings, privacy, network access,
library locations and credentials remain shared.

Current scoped checks pass with the recorded sources and normal native exits:

| Check | Result | Receipt |
| --- | --- | --- |
| Catalog, save/conflict/retry, switch journal, cold process | 18 | [Store R2](store-staged-r2-verification.json) |
| Exact four-group native defaults and one checked write | 13 | [Defaults](defaults-verification.json) |
| Native ControlbarProfile/column draft, 31 IDs, cancel/reset | 10 | [Layout](layout-verification.json) |
| Combined settings/catalog/layout switch and recovery | 13 | [Coordinator](coordinator-verification.json) |
| Profile form, failure/retry/restore, movement/compact focus | 24 | [Component form](form-verification.json) |
| Shared modal host, 20 directed transitions, 16 busy guards | 113 | [Component host](shared-host-verification.json) |

The layout fixture initially exposed a Qt GUI-metatype initialization warning.
Its accepted version uses a real Windows QApplication; production layout code
did not change to suppress the warning. Rejected runs remain in the ignored
work directory. The form's compact focus repair keeps the name editor fully
inside its scrolling viewport.

The combined recovery test found a real new-store defect: after a failed INI
write, a borrowed QSettings cache could overwrite a switch completed by another
process. Physical before/after files established the overwrite. The repaired
INI profile writer stages the existing preferences under a separate filename,
uses both catalog and ordinary Qt file locks, commits atomically and verifies
the physical bytes. Unrelated values are retained. It never places its staged
profile key in the borrowed filename cache. The existing shared preference
helper and four settings controllers retain their save behavior. The exact
previously failing cold-process recovery test now passes; see the
[regression record](recovery-regression.json).

MainCtx wiring, the native checked Profile submenu and a movable New/recovery
form now pass native playing/paused journeys (79 each), with normal exits and no
diagnostics; see the [scoped media receipt](native-media-interim-verification.json).
The matched R3 [native UI receipt](native-ui-verification.json) passes paused
and playing journeys (79 each), plus actual restart (15): it restores the
selected profile/defaults and checks the actual native rate and fresh decoded
media at 1.25. Every process exits normally without Qt diagnostics. The earlier
fixture incorrectly inspected the empty-input GUI rate cache; the production
player controller did not need a change.

The ordinary GUI (`ebcccb603c70…`) passes [five playback checks](ordinary-playback-verification.json)
and [direct Windows New/save/Shows selection](ordinary-input-verification.json)
on monitor 2. Source settings/pins remain preserved. The accessibility tool returned the original
title-bar tree during popup input; distinguish tool targeting/cache from app
behavior before claiming screen-reader acceptance. The new-profile/recovery forms are
functional adaptations: the cached original confirms menu choices but supplies
no form for naming or interrupted-switch recovery. No new Figma read was made.

The 31-tile rendered customizer and actual video/music layout consumption are
still outstanding. The native layout/value tests establish draft storage and
model behavior; they do not establish rendered controls or tile actions. This
is not full-beta, OS-DPI, Windows 10 or final-package acceptance. The earlier
[foundation receipt](store-verification.json) remains historical evidence for
the pre-integration store, superseded by Store R2 above.
