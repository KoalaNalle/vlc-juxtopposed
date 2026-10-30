# Actual history-model worker lifecycle

28 October 2026. The candidate passes all six data rows using the real
`MLRecentVideoModel`, its real QThread edit worker, native media library and
primary player. Setup/cleanup also pass: **8 pass, 0 fail, exit 0**. This adds
coverage beyond the earlier helper/Core suite and adapter-based QML components.
The test found and repaired a live `historyMode` query-cache defect; the original
failures and a strengthened old-source reproduction are retained here.

## What was exercised

Each case runs for both Continue Watching and Watch Again, with three generated
target entries and one opposite entry. These are external media-library IDs;
the harness creates no media payload and uses a fresh isolated profile.

| Case | Checked behavior |
| --- | --- |
| `busyStateAndCapturedMode` | The actual worker remains active while a bounded native history callback is held; the Qt owner processes a heartbeat, busy stays true, a second request is rejected, and completion clears busy once. Changing the model's visible mode does not retarget accepted work. The original model initially loads 3 entries, the opposite section retains 1, and both a fresh opposite model and the switched original model load that 1 afterward. |
| `workerFailureAndRetry` | A real missing-ID lookup fails on the worker, emits an error and clears busy. The same QObject then removes a valid entry successfully. This is a logical lookup failure, not a disk-write/partial-batch I/O fault. |
| `deletionDrainsActiveWorker` | Deleting the model on its Qt owner thread waits for its real gated worker. An independently joined thread releases the callback after 150 ms; the accepted batch completes, the opposite section and all four indexed entries remain, the worker/model QPointers clear, and no completion callback reaches the deleted owner. |

The gate uses the production native HistoryChanged callback path, not a worker
substitute. Source inspection establishes inline callback delivery from the
mutating operation. At runtime the gated callback identifies a running,
unfinished QThread off the owner thread, and that same thread finishes and is
deleted after release. The gate times out after 10 seconds and releases before
unregistering even on assertion failure. Each process also has a bounded outer
timeout and a local 23:35 deadline; no forced cleanup was used.

## The failure and repair

The loader captures its mode at construction. The former setter emitted
`resetRequested`, whose cache invalidation reused that captured loader. After a
live switch, the original model could continue querying the section just cleared.
The minimal [patch](mode-cache-fix/mode-cache.patch) uses the existing base-model
pattern: `resetCache()` and `m_itemLoader.reset()` before `historyModeChanged`.
This rebuilds the visible query and preserves the independently captured edit
scope. It changes no Core, header, worker policy or privacy/history policy.

| Frozen run | Result |
| --- | --- |
| [R1 original](r1-original/run/result.txt) | Full test: 6 pass, 2 fail, exit 2. Both mode-switch rows report original model count 0 rather than 1; the four other lifecycle data rows pass. |
| [R2 strengthened preimage](r2-strengthened-preimage/run/result.txt) | Focused mode-switch test: 2 pass, 2 fail, exit 2. Initial original-model count 3 and fresh opposite-model count 1 pass first; only the switched original model still reports 0. This excludes an unready history-source fixture as the explanation. |
| [R3 candidate](r3-candidate/run/result.txt) | Full test with the same strengthened test source: 8 pass, 0 fail, exit 0. No count assertion or expected data row was removed. |

The failed records deliberately retain `success: false`, producer
`AssertionError()` and `normalExit: false`: the frozen helper defines
`normalExit` as exit code 0 without forced cleanup. R1/R2 reached Qt cleanup and
returned test-failure code 2; their records must not be presented as successful
runs. All three loads report 603 modules, no checked diagnostics, exact raw-XML
row matching, and intact source/runtime/producer guards.

## Identities and preservation

| Artifact | SHA-256 |
| --- | --- |
| Old production model CPP | `3ebb0f169a6104c099eda4bdc9dc104cbccf7ab91cd2a5441d21cda4dadd5362` |
| Candidate model CPP | `cc7ede0057e3e8722a26dcfdb6c28f5a0f77ea8de9550c3c6825325db532d4b9` |
| Strengthened test CPP, R2 and R3 | `afd05778884a8ee8c9f4c2d12bc78cc078fb0982ed5cf633d96d8daa334b00b1` |
| R3 test EXE, not copied | `c3c41ec6dc11740b69f1910c2d743b02b552059ea784688fea01b198b210c6c1` |
| Core, unchanged staged baseline | `f09c45fe4075a02836807f00cab03163cdba1ec79e37ba35a0328f7cb6d5462d` |
| Media-library plugin | `e834a41a700bbc6ffeaa83bfde895f5b60f35c2e11f08cd6526d5122afe5af3f` |

Each build used one writer and two compiler workers in the established home WSL
tree, with 35 real compiler dependency graphs and 1,064 frozen header/source/link
inputs. Actual object dependencies were covered by the pre-build graphs; final
hash checks passed and no writer remained. The test EXEs are stripped of debug
sections (R3 43,847,927 bytes). No dependency was installed. Staging used immutable
hardlinks only and isolated profile/temp directories. The conservative combined
peak was 402,653,184 bytes, guarded against the owner's 80% C: usage limit.

The R3 executable directly compiles the candidate production model/service graph.
The staged ordinary Qt DLL `005fbe744643` remains the older immutable baseline;
this result does **not** assert that it contains the new setter. A later ordinary
GUI build must include the changed CPP and receive its own checks.

[Summary](summary.json), original run/build JSON, raw Qt text/XML, frozen
producers, selected 44-file source snapshots per build, compiler-input identities
and exact before/after patch are included. The historical preflight files describe
preparation, not terminal success; terminal verification records own outcomes.
[Copy verification](copy-verification.json) records byte-identical copies.
The [inventory](artifact-hashes.json) covers this receipt; `.gitattributes` keeps
its bytes unchanged in Git. No executable, DLL, object, database, settings,
personal-media content, raw VLC/device log or human/profile hash map is included.

This qualifies QObject owner-thread deletion under a real held worker, not an OS
window-close test under load. Ordinary UI storage-error/retry, real disk I/O
failure, Windows 10, design/scaling and final-package acceptance remain separate.
Earlier native 16-case and ordinary mutation/cold-persistence receipts retain
their own scopes. No commit, push or release is performed by this collector.
