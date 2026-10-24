# Checked add-on catalog and storage — 17 October 2026

Source `d926066e8993ec6ce9ebebbbb544d9d05f95270c`. This accepts the native foundation, **not the Extensions
page or beta**. The original seven-field/action page remains unmounted.

The manager exposes checked discovery admission, provider completion results,
separate operation storage/catalog results and explicit catalog-save retry.
Completion follows publication attempt. Public owner and entry layouts stay
unchanged; result state and available-package ownership live in private storage.
An entry remains busy through publication. Native callbacks run outside manager
and entry locks, and descriptor discoveries retain a stable native identity.

Installation stages every supported resource before publishing any. It refuses
pre-existing destinations and uses exclusive Windows moves for publication.
Failed attempts roll back only their own publications and remove staging files.
If deleting an owned rollback file itself fails, source retains only owned file
metadata, marks the incomplete entry broken/installed and lets Remove retry;
colliding foreign paths are excluded. That physical failure branch is reviewed
but not fault-injected. Catalog publication uses a unique same-directory file,
checks writes/close and uses Windows MoveFileEx replacement without first deleting
the old catalog. A save failure preserves actual storage state and can be retried
without another install.

Each XML reader now has its own binding state. Bad/truncated catalogs return
failure and discard partial managed entries; a missing catalog is a valid empty
profile. Bad package XML/IDs fail discovery. UUID parsing rejects invalid digits
and suffixes, reads older VLC separator positions and writes canonical IDs.
Local Unicode URLs and uppercase .VLP suffixes work. A newer selected package
keeps installed metadata truthful until successful removal, then supplies the
next install instead of silently reusing the older archive.

## Evidence

- **49 actual native cases/948 assertions** pass, including the prior
  lifecycle/direction/Unicode cases, callback reentrancy, malformed inputs,
  preserved existing script bytes, partial-resource cleanup, later-file collision,
  duplicate resources, locked catalog failure, save retry and actual selected
  replacement bytes. Twenty immediate pending-operation shutdowns exit normally.
- Standard Autotools target builds and independently passes **30 cases/
  587 assertions**, with one pending shutdown. The explicit fresh-output
  helper also builds. Meson syntax passes; no native Meson build is claimed.
- Final ordinary tuple passes redesigned/traditional controls, Library, owned
  local live HLS and bounded read-only Downloads playback. All five exit normally
  with no QML diagnostics. Source runtimes/package hashes remain unchanged;
  no private fixture module ships. Raw user media logs and frames remain private.
- All **11 scoped frozen inputs** match. This is an incremental build, not clean
  package/release acceptance. Cross-linker host-path warnings remain in the logs;
  the actual Windows executables run normally.

Core: `9182557b9f4ad0de4cbbfec46e4ef99c66c3a7c240216c74804228c02a66eaa4`

Filesystem module: `1f72cd18278294baccf78d9b26f6e83c7e37544f1d4df46020460d5eef982ef0`

Repository module: `e480f590309f612320abfd614c0cc3af32604f6c122c4af70a675b97eff5d18c`

GUI (unchanged Recording checkpoint): `d2708b57bb1978a9c0da552b5b56dc6ecd6991dc1a718e6fb252dc93508756d2`

## Rejected evidence

`baseline-callback-catalog.json` reproduces success for an invalid catalog and
an assertion when a found callback inspects the entry under its mutex.
`baseline-storage.json` reproduces overwritten owned foreign-script bytes,
an orphan after a missing later resource, and duplicate-resource admission.
The native failure assertions alone pass for the missing resource; final
filesystem checks reject that case. `baseline-selected-package.json` reproduces
the old archive being installed after choosing a newer local version and removing
the installed one. The corrected consumer installs the replacement bytes.

`rejected-fixture-routing.json` retains the first catalog-save test's own routing
error: its mode name entered the catalog-load branch and never exercised install.
The corrected probe explicitly routes save tests through installation; actual
Windows handles then verify failed replacement, preserved catalog and successful
same-manager retry after unlocking. All earlier work directories remain retained.

## Open gates

Consume the new APIs in AddonsModel: stable selected keys, truthful result/error
and busy state, cache invalidation, discovery counts, filtering locks and stale
callback generations. Mount original Extensions7 and use existing native
`qt-autoload-extensions`, preserving active extensions when editing its next-start
policy. Complete actual model/window/chooser/startup/script activation journeys
and external catalog behavior. No YouTube/Twitch integration or unshown screen
is added. Broader designed consumers, themes/accessibility/DPI/platform/regression
and a clean licensed beta package remain open. POSIX publication and a physical
denial of rollback deletion are not exercised by this Windows suite.

See [driver instructions](../../../../test/libvlc/native_addons.md).
The manifest hashes each public file; raw artifacts have separate hashes.
`/mnt/c/Modding` in logs is the established internal alias to the E: checkout.
