# Shared Qt storage and multiple windows

21 October 2026. Selected profile-store and shared appearance checks pass;
the complete multi-instance scenario and beta remain open. This follows the
[input cancellation checkpoint](../home-input-cancellation-20261021/README.md).

The existing production profile store was not changed. A rebuilt Windows test
executable passes 20 cases, including two new separate-process peers with an
initially absent and an existing catalogue. Each peer loads before the other
process commits. Its stale Create and checkpoint are rejected with errors,
without changing its local selection/catalogue or the saved file. Saving an
unrelated window key afterward preserves the accepted catalogue. Explicit reload
and retry succeeds and keeps both processes' new profiles. These are native
store/model checks, not ordinary profile-menu acceptance.

Two ordinary VLC processes share one executable, portable Qt INI and native
config on separate monitors. B explicitly chooses Light in General and the
physical INI records1. B exits normally and1 remains. A stays Dark and keeps
playing, then enters fullscreen and quits directly without editing preferences.
The final saved theme becomes2. This is the unwanted stale shutdown write in
MainCtx, distinct from the already repaired native-config merge. The
[negative receipt](ordinary-appearance-negative-r1.json) preserves both normal
exit codes, strict clean logs, stop-before-Qt ordering, runtime/media identities
and intermediate/final settings hashes. It is not accepted as correct behavior.

The same run observes the existing geometry policy: one shared slot, last closed
window wins. B writes its normal monitor-2 rectangle; A later writes its normal
primary-monitor rectangle even when quitting fullscreen. This does not qualify
all geometry/DPI/unplug scenarios or introduce per-instance startup slots.

The qualified R2 appearance repair removes the unconditional shutdown theme write and
routes explicit legacy choices through General's checked save before publishing
the active theme. Startup restore and already committed General choices use a
separate restore method. A rejected legacy edit republishes accepted check roles
and synchronizes the combo without recursive writes. The actual legacy Interface
panel's combo and CheckableListMenu pass15 keyboard/pointer failure/retry checks
with owned file replacement blocked, then recovered. Both widgets show the
accepted choice after rejection, retry the same choice, and follow a successful
General edit. The panel closes as Cancel and intentionally restores its initial
Dark choice after those successful Light checks; that is outside the prior
stale-instance shutdown reproduction. No test calls its Apply action or changes
the human's settings. The rebuilt General/setup model passes13 cases, including
checked legacy save, restore-without-write and blocked save/retry.

Ordinary GUI `aa6fc26df153…`, Core `d04e9134a43f…`, cache `1190b018b5e8…` passes
both closing orders with shared Qt/native storage. B-Light1 followed by B then
unchanged A fullscreen exit leaves1 saved. A fresh cold process visibly restores
Light, including its selected General button, and leaves1 saved on exit. In the
reverse order B-Light1, A-System0, B-Dark2 are explicit commits; A retains its
active System choice, then its fullscreen exit and B's later exit leave2 saved.
All five ordinary positive processes exit normally with clean logs and unchanged
runtime/source/media identities. The four playing processes acknowledge native
Stop before Qt teardown. Both pairs preserve normal rectangles; the shared
restore record can also restore the last window's fullscreen state. No new
per-window startup identities were introduced.
The explicit theme edits in these ordinary runs are sequential. They do not
qualify simultaneous same-key Qt save/rollback semantics. The cold saved-index
receipt is supplemented by a separate Computer Use observation of Light in the
window and the selected General button; the JSON saved value alone does not
prove active rendering.

R1 built but was not executed: review identified the legacy rejected-selection
gap before qualification. Its frozen inputs/binary identity remain in the
candidate history; R2 is the accepted selected scope. Light player
surfaces observed in the reproduction still need visual work; a correct saved
index alone is not visual acceptance.

See [verification.json](verification.json) for bound results and source inputs.
Raw logs, frozen sources, binaries and owned fixtures remain under
`build-windows/beta-shared-qt-20261021`.

The later [ordinary profile journey](ordinary-profiles-r1.json) qualifies the
selected actual shared-catalogue conflict/restart/retry path in three normal
processes on this GUI. Its clipped error is a preserved negative, repaired and
qualified separately in [profile error visibility](../home-profile-error-20261021/README.md)
on a later GUI. Other Qt keys/exit writers, two physical outputs and the broader
beta remain open.
