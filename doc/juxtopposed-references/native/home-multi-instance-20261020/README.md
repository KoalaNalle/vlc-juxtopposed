# Multi-instance shared-settings regression — 20 October 2026

**Historical failed baseline.** The later [Core repair](../home-settings-concurrency-20261021/README.md)
passes this native-settings reproduction in both ordinary closing orders. This
receipt preserves the original failure; the broader multi-instance and audio-
routing qualification remains open.

Two ordinary VLC processes use the same owned native `shared-vlcrc`, separate
portable Qt settings, different generated silent WAVs and different monitors.
Both start paused. A enters fullscreen; B remains independently usable.
No user media/settings are involved. Actual edits and exits use Computer Use.

| Step | Saved notification duration | Saved remember-window policy |
| --- | --- | --- |
| Both processes start | 777ms | On |
| A changes only duration | 5000ms | On |
| B changes only remember-window policy | **777ms: A's change lost** | Off |
| B exits normally | 777ms | Off |
| A exits normally | 5000ms | **On: B's change lost** |

Each snapshot records the real shared file's selected values and SHA256. Both
processes exit normally with no native/Qt errors; source and generated media are
unchanged. Closing B leaves A fullscreen on its own media at the paused clock.
No raw Computer Use screenshot was saved. `verification.json` binds the records
and ordinary GUI identity.

Source review points to `config_SaveConfigFile`: a per-PID temporary and a
process-local mutex serialize the process's full in-memory defaults, then replace
the common file. Atomic replacement does not merge another process's edits.
The normal-exit path also needs coverage; changing only the GUI save path would
leave the second observed overwrite unresolved.

The review agent confirmed explicit setters leave the process-wide dirty flag
set. Exit autosave therefore serializes the stale snapshot again. The checked
Qt writer verifies requested lines only and cannot detect unrelated lost edits.
Its post-save readback also sits outside the persistence lock and must not make
a later successful same-key writer look like a failed earlier commit.

Repair review calls for per-key pending persistence state and a latest-file
merge under an interprocess lock, shared by explicit and exit saves. A successful
commit must acknowledge the exact committed edits, while a failed checked write
restores its prior values and pending state. Keep external saved defaults out of
the currently running media configuration; forced single-instance behavior and
live synchronization would be new product policy. Same-key conflict semantics
and a transaction-level verified result need explicit regression coverage.

Keep current playback/output independent from shared future defaults. Fix and
verify unrelated-key preservation, same-key conflict policy, read-only/locked
write rollback, normal exit and clean single-instance behavior. Preserve explicit
command-line overrides and configuration format/other options. The read-only review supports the bounded Core design, subject to regression
proof. Preserve the startup `--reset-config` full-default save path (it bypasses
setters), explicit resets, duplicate option lines, BOM/CRLF/last-line handling,
unknown content, and real read failures. Use one consistent lock order and
acknowledge edits within the transaction; keep the sidecar lock persistent.
The later receipt records the implementation and selected qualification; no new
settings UI or live synchronization policy was introduced.

Separate active audio endpoints, simultaneous playing/seek/queue/track actions,
shared Qt profile/geometry storage and final-package behavior remain open.
