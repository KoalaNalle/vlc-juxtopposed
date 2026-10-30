# Native history-worker lifecycle: prepared, not executed

Root has reviewed the test/sidecar and the independent reviewer found no blocker.
These producers remain idle until root explicitly releases the writer lane and
the fresh C: budget check passes. No build, GUI process or native test has been
started by this preparation. Native mux work and its recording runs own scheduling.

The opt-in sidecar compiles actual MLRecentVideoModel, its model/service graph and
the owned HistoryEditThread. The history-source factory supplies the real ML and
primary player through a minimal borrowed Qt context; it is not a MainCtx window.
Three cases have Continue/Watch Again rows: busy/reentrant/captured-mode behavior,
real logical missing-ID failure and same-model retry, owner-thread destruction
while the real mutation's synchronous callback is gated. The callback's QThread
must be off-main, running and unfinished while held, and finish before accepted
work/destruction returns. The gate is bounded to ten seconds. This does not
simulate a disk write failure or claim ordinary close-under-load acceptance.

The configured home WSL toolchain and current f09 Core/e834 media-library runtime
are reused. The old005 Qt DLL is staged unchanged for the603-plugin inventory;
this does not bind later unbuilt menu CPP edits to that old binary. No Core or
GUI build is requested, and only one native writer is allowed. The build has
two jobs and a nonblocking lock; only fresh ignored output receives new objects.
Exact compiler -M include dependencies are frozen before compiling; the actual
-MD graph must be a subset with every hash unchanged afterward. Configured flags,
tool executables and resolved link inputs are also guarded. This avoids a broad
whole-Qt source inventory and does not read the four excluded experiments.

Empirical matching production objects total30.72MB CPP+15.61MB moc. The older
477.5MB Home test has446.5MB debug sections and16.9MB nondebug PE sections. The
stripped opt-in executable is expected to occupy tens of MB; the producer rejects
an executable over80MiB or any remaining debug section. A conservative combined
build/run allowance is384MiB (402,653,184bytes). It is rechecked before build;
the runner uses a112MiB additional runtime/profile allowance after build outputs
already exist. Immutable DLLs and the executable are linked, never copied on
hardlink failure. The destination is unlinked before a staged DLL/cache swap.

Both producers enforce the28October23:35Helsinki stop bound. The build refuses
to start without twenty minutes remaining; each owned make step is bounded and
stops with a cleanup reserve. The run refuses to start without six minutes and
owns bounded cache/test processes. Timeout/forced cleanup remains failed evidence.
Raw Qt XML must contain exactly six named data rows plus setup/cleanup, all pass,
normal exit0,603modules and no checked diagnostics. Every terminal failure is
retained in its fresh receipt; never overwrite it or silently relax a guard.

Only after root's lane/storage approval, execute the one-use WSL builder with
`--execute-reviewed-build`, inspect its terminal receipt/lock release, then use
the Windows Python runner with `--execute-reviewed-run`. Preserve the raw XML,
text and failure output. Neither producer installs dependencies or changes
shipping source files. The current storage snapshot is information, not approval;
a failed current budget requires root's storage handling rather than a workaround.
