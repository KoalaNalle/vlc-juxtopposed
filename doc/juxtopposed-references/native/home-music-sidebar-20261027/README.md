# Music sidebar open/closed states - 27 October 2026

Figma's paired music frames expose a missing state: the old native toggle opens
another navigation drawer over the source list. The repaired toggle hides and
reopens the inline music sidebar; video navigation retains its original behavior.

The same component regression fails before and passes after. Ordinary GUI349
verifies small/maximized states, exact paused-position retention, closed-view
Play/Pause, reopened Down/Enter activation and audio/video transitions. Both
ordinary runs exit0/602modules/zero checked errors/all guards.

The [checkpoint](checkpoint.json) retains the original Figma renders, source/asset
identities and stale-capture limitations. This is a scoped interaction repair;
canonical player sizing, complete visual scores, OS-DPI and final-package gates
remain open. Continue from [STATUS](../../../juxtopposed/STATUS.md).
