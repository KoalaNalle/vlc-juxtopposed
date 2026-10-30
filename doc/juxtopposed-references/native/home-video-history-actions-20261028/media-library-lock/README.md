# Media-library priority wake repair

Qualified 28 October 2026. This receipt covers a dependency lock repair and its
actual VLC plugin integration. Application-level Home action acceptance belongs
to the parent checkpoint; this focused result does not substitute for that work.

The original SWMRLock shares a condition variable between readers and writers.
When the last reader exits, `notify_one` can select a reader blocked by a waiting
writer's priority. That reader cannot proceed; the eligible writer stays asleep
with no active lock owner left to wake it. The patch changes that notification
to `notify_all`, retaining the existing predicates and priority policy.

An earlier Home suite hang had matching waiting source paths. Its lock counters
were not captured before timeout, so this reproduction proves the dependency
defect without claiming to prove that earlier hang's exact state.

## Focused regression

The harness compiles the frozen actual SWMRLock source. Reader A holds the last
read lock; writer W takes priority; reader B queues first; W then queues for its
write lock; A releases. Queue setup is checked using actual lock counters under
the mutex. The deterministic variant uses a test-only compatibility header that
chooses the oldest registered waiter, an allowed `notify_one` selection. The
native variant uses the actual configured standard condition variable on Windows.

| Source | Wake scheduler | Outcome |
| --- | --- | --- |
| Baseline | deterministic | Trial 1 fails, exit 1; explicit diagnostic rescue |
| Candidate | deterministic | 100/100 pass, exit 0; no rescue |
| Baseline | actual Windows standard CV | Trial 1 fails, exit 1; remaining 499 requested trials not run; explicit diagnostic rescue |
| Candidate | actual Windows standard CV | 500/500 pass, exit 0; no rescue |

Both failures record `readers=0 readers_waiting=1 writing=0 writers_waiting=1
priority_owners=1`. The rescue runs only after the failed deadline to join owned
test threads; it cannot count as a pass. All four processes exit normally; none
requires forced termination. The runner exits 0 with producer/source/binary
guards intact. See [verification.json](harness-results/verification.json) and
the preserved per-process outputs beside it.

The compiler is the established GCC 13 POSIX MinGW toolchain; the dependency's
CXX11 threads, mutex and condition-variable options are enabled. Public source
copies, configured headers, the deterministic scheduler and exact frozen
producers are retained here. No test executable or compatibility shim is shipped
with the application.

For a fresh reproduction, compile `harness/swmr_priority_wake_test.cpp` alone:
it includes the selected SWMRLock.cpp in one translation unit. Use C++14,
`-O2 -g -static -pthread -DHAVE_CONFIG_H=1`, the selected `harness/baseline` or
`harness/candidate` include directory, and `harness/configured`. Put
`harness/deterministic` first only for the deterministic variant. Run the
resulting Windows executables with the trial counts in the frozen producer,
using a fresh ignored output directory. The retained historical producers use
the established home paths; adapt output paths before a new run and freeze the
new producer. Reusing an old receipt directory is deliberately rejected.

## Actual dependency and plugin integration

Tracked recipe: `contrib/src/medialibrary/rules.mak`, dependency 0.13.2, with
`swmr-priority-wake.patch` applied after the existing history-only patch. The
tracked patch's empty context line was normalized to avoid trailing whitespace;
it applies with fuzz 0 and produces the exact source qualified above.

The frozen integration producer acquired the exclusive native build lock at
15:12:59 UTC and released it at 15:13:43 UTC, exiting 0 in phase `complete`.
It preserved one original source/archive/plugin backup locally, rebuilt only
SWMRLock.cpp and the dependency archive with two workers, installed without
rebuilding or rewriting the 36 unchanged headers/pkgconfig files, then explicitly
relinked `libmedialibrary_plugin.la`. No Core, Qt or test target was invoked.

Actual configured dependency build:
`/root/vlc-juxtopposed-build/contrib/contrib-win64/medialibrary/vlc_build`.
The installed archive is under
`/root/vlc-juxtopposed-build/contrib/x86_64-w64-mingw32/lib/`.
The VLC plugin target runs from `/root/vlc-juxtopposed-build/win64/modules`.

The plugin makefile does not list the installed static archive as a concrete
prerequisite. After backing it up, the producer removed only the old `.la`
target marker to force a real link. The plugin's own objects were recompiled
but remained byte-identical. The recorded actual link command and before/after
disassembly prove the candidate's `unlock_read` calls `notify_all`, replacing
the original `notify_one`. See [integration](integration/).

| Artifact | Original SHA-256 | Candidate SHA-256 |
| --- | --- | --- |
| SWMRLock.cpp | `f286e33baa44a70253d654f835b713adcccb0246fbee02b0dfc2895005d5090d` | `57a6cfd0d6fa5fab4d9c17e4301c8016cc7bc73b3880880ef930b501504c404e` |
| Installed static archive | `733415868ab4ce98048485c471854b9ba34e0643ef367e2e4afde7bc6da36a3b` | `00e6ee362efa4c0bda2f8362ab2d4466e56e285b6c3d76c856bbc678187c762c` |
| Media-library plugin DLL | `4e7edccd99337fa6b1a862e16ce6dd78c90c1c24ee75c3f279417efd8f6fc948` | `e834a41a700bbc6ffeaa83bfde895f5b60f35c2e11f08cd6526d5122afe5af3f` |

All integration guards passed: producer 1, recipe 2, dependency inputs 229,
plugin inputs/unrelated existing binaries 158, plugin objects 20, and original
backups 5. Their summarized results and original log identities are in
[guards.json](integration/guards.json). Binary backups remain ignored locally;
this receipt includes public sources and generated diagnostics only. Raw private
media, profiles and debugger logs are excluded.

`SHA256SUMS` separately inventories this receipt's exact files. The nested
`.gitattributes` preserves their bytes through Windows checkouts. Subsequent
application tests must identify the candidate plugin separately from their Core,
Qt and executable identities, and retain the baseline failures above.
