# Home customizer work — 19 October 2026

This is a scoped work checkpoint, not full beta or package acceptance. The
accepted ordinary GUI for this scope is now `ordinary-runtime-ui-r2` (SHA256
`2c8d2d219d1f9a844a9a81171f8cefdf3a3152dc3531f21bbc1220d0d0d8a663`).
R2 native customizer journeys and five ordinary playback cases pass under ignored
`build-windows/beta-customizer-20261019`.

The later [current-media checkpoint](../home-current-media-20261019/README.md)
adds a tested native Favorite consumer and repairs shared toggle/key semantics.
The unfinished-Favorite notes below describe this older `2c8d…` checkpoint;
they do not override that newer scoped evidence. Full beta gates remain open.

## Verified repair

The existing toolbar wrote Repeat All's enum value into the boolean `repeat`
option and did not update `loop`. The unchanged native GUI reproduced that
failure, exited normally and emitted no Qt diagnostics. The repair writes the
two distinct boolean flags and lets the typed Playback owner observe the queued
native transport changes without introducing new preference writes in runtime
setters. The selected profile then checkpoints those actual native defaults.

The repaired private GUI passes 71 assertions across four normal process runs:
Off → All, restored All → Current, restored Current → Off, and restored Off.
Shuffle, the Playback page values, selected profile snapshots, real decoded
primary input and its continuing clock are covered. See the exact scoped
[receipt](transport-verification.json). This is not ordinary-package proof.

## Customizer implementation in progress

- The existing native layout models expose read-only current/draft snapshots,
  metadata and the authored 31 palette entries. Global VLC Settings has a
  distinct stable ID from current-media Playback settings. Eleven native layout
  checks pass, including model/JSON/draft/descriptor checks; see the
  [layout receipt](layout-verification.json).
- A separate draft palette supports video/music selection, timeline placement,
  pointer add/remove, keyboard alternatives, Cancel and checked Save/retry. Its
  shared preview displays current media state without controlling playback.
  Sixteen component checks pass; they use an owned service contract and do not
  substitute for actual native persistence or media acceptance.
- The sole dialog owner includes the customizer. Component checks pass all 30
  directed transitions among six dialogs and 25 busy-operation guards, plus
  setup and focus checks (115 total, no Qt diagnostics).
- The old default video presentation is preserved. The extracted shared volume
  control retains the existing Light/Dark artwork, clipping, keyboard/wheel and
  native write/unmute contract checks. Edited bars use the same timeline and
  volume presentation. R2 native app checks pass for the tested layouts.
- Existing bookmark/chapter views are reused. Their session tab is shared
  across dock/undock. Fixed 10/15-second tiles use the existing native seek
  policy, separately from configured shortcut jump lengths. New filename and
  local-folder labels come from the native source URL.

Original reference: Juxtopposed's cached design/video, described in
`../../setup-customization-20261018.md`. No new Figma reads were made. Individual
customizer icon exports are absent from the cached material; the existing VLC
font is an explicit functional adaptation. The player selectors, keyboard
alternatives, bounded movement and compact reflow are also adaptations. They
are not claims of exact original pixel fidelity.

Favorite and live HDR switching are unfinished consumers and remain disabled;
the current-queue playlist-name label also needs an authoritative source. The
remaining tile actions need actual native journeys. Full visual, screen-reader,
OS-DPI, ordinary package and broader beta roadmap gates remain open. No commit,
push, tag or release upload is authorized or performed by this checkpoint.

Expanded component regressions reproduced missing keyboard focus after pointer
selection and accepting a drop in clipped viewport padding. Both failures are
preserved under `customizer-pointer-focus-before` / `customizer-clipping-before`
in the ignored work folder. Both are repaired and now covered in the actual
native app. Native captures also exposed duplicate live/draft timelines;
editing now hides only the live presentation while preserving its native seek
objects and the primary input.

The pinned R2 DLL passes paused111 / cold restart15 / playing111, with normal
exits and no Qt diagnostics. See the [exact native receipt](native-ui-verification.json)
and its four owned captures. The expanded checks include real pointer selection
then Delete, an actual drag outside a scrolled viewport, draft/accepted layout
isolation, failed Save/retry, live seek-object reuse, and both restored layouts.
An initial R2 launcher mistakenly referenced the R1 DLL; those runs are rejected.
The accepted R2a runner pins both the exact path and checksum and requires the
new regression check names. The local Qt source checkpoint is
`build-windows/beta-customizer-20261019/source-ui-r2`.

The [ordinary playback receipt](ordinary-playback-verification.json) covers the
redesigned/traditional player, Library enabled, generated live HLS and a read-only
`D:\Media` sample on that exact GUI. The [direct Windows input receipt](ordinary-input-verification.json)
records actual Tools/customizer, mouse add, Ctrl+Return Save, saved INI/reopen,
Escape and normal window closure on monitor2. The original copied-source
profile and saved pins remain unchanged. No raw Computer Use screenshots were
saved. UI Automation exposed the named popup and all31 palette buttons after a
fresh post-menu observation; its reported focus stayed on the old title text.
This does not establish full screen-reader acceptance.
