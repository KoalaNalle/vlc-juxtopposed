# Home: Continue Watching and Watch Again

The owner chose separate sections on 28 October: unfinished videos are the
primary Home section and resume; completed videos appear below in Watch Again
and start over. Other confirmed sections remain. This is an owner-directed
adaptation, not a new claim of Figma visual acceptance.

## Implementation

The existing recent-video model keeps its default All mode for classic callers.
The redesigned Home and See All routes use separate Unfinished and Completed
modes. A positive saved position below 1 belongs to Continue; a positive native
play count with no saved position belongs to Watch Again. An unfinished replay
takes precedence over previous completions. Merely opening a video does not
qualify it as watched. The native library retains its existing near-end policy;
unknown-length media are not assigned invented completion thresholds.

Filtering happens before the view's offset/limit, preserving native search,
sort and history scope. Count scans the same predicate. History changes reset
the model; filtered views also reset when media updates can change membership.
Watch Again explicitly requests `restore-playback-pos=0`; Continue requests 2.
The dashboard retains its other sections and existing card presentation.

## Evidence and repair

The production dashboard/component suite passes with supplied models, including
separate activation, keyboard, empty/loading/search states, responsive layouts
and collection behavior. [Component capture](home-sections-component.png) uses
generated solid-color covers; it is not an ordinary native app or fidelity test.

The native `nativeVideoHistorySections` test initially failed on the preserved
core r5: the indexed two-second fixture reached actual EOF but persisted progress
about 0.927 and play count 0. Periodic position reporting lagged behind EOF.
The candidate records position 1 for a known-length, non-live, natural EOS,
while retaining existing behavior for user stops and explicit stop-time/run-time
segments. Existing history/position opt-out checks still gate persistence.
It changes the saved history boundary, not the playback timer or displayed clock.

The candidate Core SHA-256 is
`59a2e22490841efc9032bc5ece2266219897be11ae8010d31a253004f37c63c5`.
The passing test executable SHA-256 is
`09e1fa1fb60c713ffd9a3a6811cd5ea88b83808af3c2db6a2643864ad5de2b0e`.
`native-history-r5` exits 0, with setup, selected test and cleanup passing,
no skips and all source/runtime/fixture/test/producer guards intact. It covers:

- Real primary-player completion of the indexed short fixture and native history events.
- Manual Stop after an actual paused seek, plus stop-time and run-time playback segments.
- Completed-to-unfinished replay classification and clearing watched state.
- 300 interleaved native history entries, 100 in each accepted section, with
  correct filtered counts and offsets beyond the 256-entry native query page.
- Alphabetic/reverse ordering, search and history clearing.

The earlier `native-history-r4` test stopped immediately after a clipped
start-time, before establishing a nonzero position within that segment. It
correctly had no completion, but failed the test's expected positive resume
point. The final test explicitly seeks and waits for the desired position
before Stop; this is a fixture correction, not a relaxed completion assertion.
The build also retains a corrected QStringBuilder compile failure and a CRLF
shell-launch failure in ignored output. They are not player failures.

Detailed native results, identities, source patch and frozen producers are
beside this receipt. Local build/test files are under
`build-windows/beta-home-sections-20261028/`. Prior core/runtime copies remain
intact; no dependencies, firewall/privacy settings, private media or Git refs
were changed.

## Wider native suite and ordinary follow-up

`native-full-r3` exits 0 with all 12 Home checks passing, no skips and every
preservation guard intact. Its test executable is
`6cfa7cfb82d752263c917c3e707e54c45485b1f5f0152f3f851a91bc65788643`.
Two older storage tests were updated to the existing checked-store contract:
moving the saved document away changes QSettings' shared cache on sync, and its
first error is not a reliable fresh-save result. Rejected operations must leave
accepted visits and the retained file bytes unchanged; restoring storage must
allow retry, retain unrelated settings, and survive restart. The earlier failing
suite results remain beside this receipt. Production storage code was not changed.

Ordinary Qt build `gui-r1` succeeds, plugin SHA-256
`efc198f5b8069f8630003509dba33cc33e4089626f6229466c0988f0d7436b3f`.
[Ordinary evidence](ordinary-r1/) records generated media, exact runtime/source
identities, scoped screenshots and frozen launch/exit observers. The first run:

- Indexes three generated videos without adding unplayed files to either section.
- Adds an actually completed video to Watch Again while Home is visible.
- Moves its stopped replay to Continue, excluding it from Watch Again.
- Resumes through the card and full Continue list despite general resume Never.
- Starts over through the Watch Again card and full list, with correct list counts.
- Filters through normal typed search, shows zero matches, and restores on Clear.
- Retains the paused player/position during Home navigation; its first resume
  record becomes visible after Stop, following current native history persistence.
- Displays distinct unfinished and completed videos in the two ordered sections.

The model-capacity interruption outlasted the first launcher's observation limit;
it left owned PID 10540 running. An observer attached before normal Close records
exit 0 and unchanged source/runtime/fixtures/producer/human configuration. No
checked diagnostics occurred. The timeout is preserved rather than rewritten.
The cold restart uses a copied profile and general resume Always. Both sections
retain their membership; Continue restores the saved point and Watch Again starts
from zero. VLC exits 0 and all seven guards pass. Its strict launcher retains a
failure for `main error: window closed`, emitted by the deliberate Close on paused
video; this is reviewed separately rather than hidden by a log allowlist.

## Completion boundary follow-up

[Boundary evidence](completion-r2/) preserves the later repair, source snapshots,
build identities, failures and matched comparisons. The earlier force-to-1 repair
was replaced by the master's actual final position, reported only after output
drain. History retains the existing native completion policy. Live, unknown,
failed or unassigned demux positions use existing fallback behavior. Accepted
seeks invalidate the cached EOF; pending targets are saved only for seekable,
nonrecording inputs, whose native controls can perform those requests.

Core r12 is
`994453279cfc15154841e64a49b3aa21dce49cb537e74c1bfa478e730f3d56bd`;
test executable is
`f2f325a0fd2c3c7fbeac9b26fd12cfdb7dfab209f46ffc0ebc37ba2d65be1b8f`.
`native-full-r7` exits 0: all 14 checks pass, no skips, every guard intact.
This includes seven completion/seek boundary rows and recording-rejected seek
history (.4 actual retained, .95 ignored target not saved). With the identical
test executable, the retained earlier Core misses three full-source completions
and incorrectly counts an immediate post-EOF seek/Stop as completed. The first
test's exact-zero expectation was corrected to the native unset/cleared sentinel
(`<=0`, including -1); positive rows still require exactly one completed watch,
and negative rows require count 0 and a positive unfinished position.

The first full r12 run stalled in media-library control after the 300-row test,
before starting the next playback. Its 180-second helper stopped the child;
parsing partial XML then failed before terminal verification could be saved.
The original partial result remains, with a supplementary termination review.
A noninvasive stack capture places the test in `Media::removeFromHistory` waiting
for the library write lock. The shared reader/writer condition variable's
`notify_one` is a source-backed lost-wake hypothesis, requiring a focused
reproducer and dependency repair. The later full pass does not erase this stall.

The existing native ASS playing/FPS/EOF-revival regression also passes on Core r12
using test r23; this is one selected regression, not all previous subtitle checks.
Ordinary Qt r2 plugin is
`d068df5888e726cdc4decb2f30d36717e837d5bc253bdd59b5d2b949cf8611df`.
[Ordinary r2](ordinary-r2/) restores the copied unfinished/completed profile,
resumes the saved point, moves the finished video into Watch Again and shows the
new `No unfinished videos` full-list state. VLC exits 0, all seven guards pass,
603 modules load, and no checked diagnostics occur.

## Still to qualify

Investigate the library-lock stall; characterize title/chapter MRL ranges and
truncated input. Recheck relevant history/privacy and remaining subtitle behavior
on the final integrated tuple. These scoped results do not close Windows DPI,
accessibility, visual, clean-package, integrated regression or beta gates. The
new section split is an owner adaptation; no new Figma fidelity score is claimed.
