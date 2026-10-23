# Ordinary queue addition, 23 October 2026

Selected safe addition workflows pass on ordinary GUI f528dd1c on the home
Windows11 computer. No production change or rebuild was needed. Cedar remains
paused at13 seconds with an unchanged slider position through Cancel, one-file
addition, batch addition and addition under an active Birch filter.

The final visible order is Cedar, Birch, Aspen, Dune, Echo, Fir. The existing
native provider sorts each multi-file selection before appending, so the
reverse-typed Echo/Dune batch becomes Dune/Echo. Clearing the filter reveals all
six rows. UIA tree order is not used as evidence of their visible order.

The process exits normally with0, no captured runtime errors/Qt exceptions,
unchanged source/runtime identities and no forced cleanup. Configuration and
media are isolated; all files are generated silent WAVs. The existing2,103-input
R17 source freeze remains authoritative for the ordinary GUI. The new producer
is hashed separately; it is not retroactively added to that build manifest.

Two findings remain open: the expanded filter overlaps the Add toolbar button,
and the initially paused start-time setup lets the thumb move without updating
elapsed time until briefly played. The addition baseline is established after
that brief playback. These observations do not qualify paused-seek consistency
or the overlapping layout. No UI deletion, physical dragging, touch, DPI,
screen-reader, Windows10, complete Queue/design or full beta acceptance is claimed.

- [Checkpoint and hashes](checkpoint.json)
- [Bounded review](ordinary-review.json)
- [Raw process receipt](ordinary-process.json)
- [Raw UI observations](ui-observations.json)
- [Existing source manifest](source-inputs.json)
- [Separate ordinary producer](ordinary-producer.py)
