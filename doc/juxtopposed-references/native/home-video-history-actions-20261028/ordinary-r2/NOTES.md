# Ordinary history mutations: 28 October 2026

The owner explicitly approved history deletion through the test interface. This
ordinary VLC run uses a copied profile containing generated media. It exits 0,
loads 603 modules, has no checked diagnostics, and passes all seven preservation
guards. Only history entries are removed; the library and media remain.

## Observed actions

- Continue Watching's individual **Remove from this list** action is selected
  from its actual item menu. That entry disappears; Watch Again's entry remains.
- On Watch Again's full list, an unmatched search hides its entry. The header
  menu still offers **Clear all items**, and the confirmation identifies the
  selected section and includes entries hidden by search. The owner-approved
  confirmation is accepted; removing the search still shows an empty history.
- Home shows both history sections empty while its library header retains
  **3 items**. Both remain empty after navigating away/back and reloading the
  Home models. The owned application closes normally after these operations.

The 12 numbered screenshot/accessibility observations preserve the actual menus,
confirmation and outcomes. The isolated ordinary app's real QObject worker
success path is exercised here; this is not a native-helper-only demonstration.
An ordinary worker failure, retry after real storage failure, or closing while a
large edit is still busy remains outside this run. Opposite-section preservation
is observed for the individual removal; native evidence separately covers both
section directions and large lists. Keyboard modifier delivery, Windows 10/DPI,
design scoring and final-package readiness are not established.

The [separate cold restart](../restart-r3/NOTES.md) opens the saved post-mutation
profile in a fresh process and confirms persistence. Original menu-only evidence
under ordinary-r1 remains a historical receipt; its pending approval was later
answered and does not describe this approved mutation run.

All captures are of the owned test window/menus and generated media. Original
capture filenames and bytes are retained, including JPEG data saved with .png
names by the capture helper. The verification omits personal configuration hashes
and copied-profile inventories while retaining their preservation booleans.
Raw VLC logs, media, binaries and settings databases are not copied. The frozen
producer uses one-use local paths; adapt into a fresh directory before any rerun.
