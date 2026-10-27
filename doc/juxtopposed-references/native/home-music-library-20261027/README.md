# Music library nightly - 27 October 2026

Core `fdfcfcd5`, ordinary GUI `41079bd8` and library module `13ccdc36`
implement the original category artwork and Songs -> folder -> tracks route.
Real audio queries exclude mixed video, while All songs retains a flat fallback.
Native Track sort now uses media track identity rather than the unrelated
artist/album collection-count criterion. Disc stays a read-only metadata column.

Keyboard Play survives removal of its source view. Six native functions verify
the real service/queue lifetime, selected order, offset and playlist origin,
cancelled reads, removed/invalid rows and normal shutdown. Six real query functions
pass with the new criterion; the old criterion fails its intended control.

Ordinary keyboard playback passes for a Unicode song, filtered song and video.
Empty search/clear, paused and playing navigation, traditional flat Tracks and
cold folder/Track-sort restoration pass in their recorded scopes. Three accepted
apps exit0/602modules/zero errors with all guards true. The
[checkpoint](checkpoint.json) preserves exact identities, captures, native reports,
Figma nodes and earlier failures, including the corrected base/runtime guard.

Complete album/artist/genre journeys, repeated album-title grouping, independent
Disc sort, initial empty sort selection, search-history retention, full design
scores, Windows DPI and final-package gates remain distinct unfinished work.
Raw profiles, databases, application logs and binaries stay local. Continue from
[STATUS](../../../juxtopposed/STATUS.md).
