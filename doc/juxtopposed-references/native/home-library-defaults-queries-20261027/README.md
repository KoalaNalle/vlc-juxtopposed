# Library query races and collection sort feedback - 27 October 2026

Nine production model/service cases pass, including three forced overlapping
search/parent/sort-order A-B-A queries. Abandoned results cannot replace current
rows or emit stale updates. The supplied row loader is separate from native SQL.

Albums, Artists and Genres now show their verified native alphabetical default
when no explicit criterion is saved. Fourteen native query cases, sixteen
component groups and ordinary baseline/fixed GUI344/restart journeys qualify the
repair. Explicit choices retain precedence and survive restart. Other categories
and final beta gates remain open.

See the [checkpoint](checkpoint.json) for identities, preserved baseline failure,
capture/build/report limitations and exact evidence. Continue from
[STATUS](../../../juxtopposed/STATUS.md).
