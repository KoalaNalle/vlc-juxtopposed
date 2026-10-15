# Library settings integration checkpoint

15 October 2026. The seven-field Library page, original selected icon, native
preferences model, MainCtx ownership and property, QML registration, shared Host,
resources, Autotools/Meson and component-CI registrations are integrated in source.

Component checks against the actual integrated production QML passed:

- Library: 11 groups.
- Shared Host: 24 checks, 12 directed transitions and 9 busy guards.
- Subtitles regression: 15 groups.

All three runs exited 0 with zero Qt messages; the recorded source hashes still
match this checkpoint. See the [component receipt](verification.json).
Library coverage includes authored
asset identities, keyboard interaction, legacy custom values, save rejection,
missing service, search/focus and scaled/narrow layouts.

The four new native consumer flags remain false: instance policy, opening queue,
Windows hidden attributes and optional metadata fetching. This Library source
checkpoint has not had a native build or desktop acceptance. The complete native
Library fixture depends on private diagnostic Core exports, so its sources are
distributed without registering a broken shipping-linked test target.

The incomplete metadata cancellation/enrichment and untested IPC proposals remain
only in the ignored local build area. They are not applied to production and are
not part of this commit. Resume from
`build-windows/library-beta-rework-backend-20261015/RESUME.md` on the home checkout.
Finish and validate those consumers, then run an ordinary build and real desktop
journeys before enabling their flags. Existing folder-recursion/media-library
controls also require native acceptance. This is not full Library or beta acceptance.
