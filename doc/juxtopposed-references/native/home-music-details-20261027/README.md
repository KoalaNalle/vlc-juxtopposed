# Music detail nightly - 27 October 2026

Core `fdfcfcd5`, ordinary GUI `1a0c6c19` and library `13ccdc36` now keep
artist details within the shared redesigned navigation/search/sort/view header.
The traditional layout retains its custom controls. Genres have a full
breadcrumb path, and albums without a year no longer show doubled separators.

A controlled regression found that Enter on one artist table row both played
and navigated. The repaired route now only navigates, matching the grid; explicit
Play and Enqueue remain separate. Ordinary keyboard/pointer, paused clock,
queue, search, view and history observations cover both layouts. Two final apps
exit0/602modules/zero errors with all source/runtime/fixture/cache guards true.

Eight native query functions qualify actual album/artist/genre identities,
typed tracks, counts, paging and search; the old Track mapping fails its intended
control. The two Evening Album entries are deliberate native containing-folder
identities, supported by source and a read-only generated database observation.

The [checkpoint](checkpoint.json) preserves binaries, frozen source, build/test
reports, captures, the table-Enter before failure and evidence qualifications.
No full surface, visual score, OS-DPI, media-matrix or final-package gate closes
from this scoped receipt. Raw profiles/logs/databases and binaries remain local.
Continue from [STATUS](../../../juxtopposed/STATUS.md).
