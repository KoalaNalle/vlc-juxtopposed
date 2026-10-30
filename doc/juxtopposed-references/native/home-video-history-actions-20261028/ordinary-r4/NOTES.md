# Ordinary reverse-direction history actions: 28 October 2026

This owner-approved ordinary VLC run tests the opposite combination from
[r2](../ordinary-r2/NOTES.md): individual Watch Again removal, then Continue
Watching's Home section Clear all. It starts from a fresh copy of the original
generated-media profile. VLC exits normally with code 0, loads 603 modules, has
no checked diagnostics, and passes all seven preservation guards.

## Observed actions

1. Home initially contains both history sections. The actual Watch Again item
   menu's **Remove from this list** action is selected. Watch Again becomes empty
   while the Continue Watching entry remains.
2. Continue Watching's Home ellipsis menu offers **Clear all items**. Its named
   confirmation defaults to **No**. After the owner's action-time approval, the
   clear is accepted and both history sections are empty.
3. The Videos route still lists all **three generated library entries**. History
   deletion does not remove those indexed media. The owned window closes normally.

Eight numbered screenshot/accessibility observations preserve the menus, named
confirmation, opposite-section retention and final library. Together with r2,
ordinary per-item deletion is now observed in both sections, as are Home and
full-list clearing routes. R2 specifically checks clearing a search-hidden item;
the [r3 cold restart](../restart-r3/NOTES.md) separately checks persistence of that
earlier post-mutation profile. This r4 run does not add another cold restart.

These actual ordinary actions exercise the production QObject worker success
path. A separate proposed native QObject/worker lifecycle test has not inherited
acceptance from these fast operations. Real storage failures/retry, deletion
while an operation remains busy, modifier-chord delivery, Windows 10/DPI, design
fidelity and final-package acceptance remain separate.

## Preservation and scope

The frozen driver uses the existing R4 hardlink-only staging support. A copy of
that already frozen helper is retained under `support/`, verified against its
recorded preparation identity. It is identified separately from the original
UI producer's seven guards. This public collector does not launch an application
or rerun a producer, and does not inspect current production source identities.

Original capture bytes/names are preserved; the capture helper saved JPEG data
under .png filenames. All images/accessibility states concern the owned VLC
window and generated media. The public verification omits personal configuration
hashes and the copied-profile inventory while retaining preservation booleans
and source/runtime identities. No raw VLC logs, settings databases, binaries or
media are included. `artifact-hashes.json` inventories every receipt file except
itself; `.gitattributes` prevents newline conversion of the frozen evidence.
