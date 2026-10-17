# Native add-on regression

`native_addons.py` creates an owned inert VLP ZIP package and uses the actual
Core add-on manager, repository parser/archive extractor, filesystem storage
module and catalog loader. No catalog download or script activation is involved.

Build `test_addons` explicitly with Autotools or Meson, or use
`test/addons-native.mk` with the configured cross-build Makefile and a fresh
`ADDONS_OUT`. The executable is opt-in because it requires owned package and
user-data paths; it is not a no-argument `make check` program.

Example (Windows, use absolute paths on the same volume):

```powershell
python test/libvlc/native_addons.py --runtime E:/owned/source-runtime `
  --probe E:/owned/build/.libs/test_addons.exe `
  --core E:/owned/build/libvlccore.dll `
  --storage E:/owned/build/libaddonsfsstorage_plugin.dll `
  --repository E:/owned/build/libaddonsvorepository_plugin.dll `
  --output E:/owned/fresh-addons-run
```

The driver makes a fresh immutable hard-link runtime, detaches only replacement
files, regenerates its cache and isolates user data, temporary files and logs.
The source runtime is hashed again after execution. Each process has a deadline;
forced cleanup or an assertion rejects the case.

Checks include wrong-direction and repeated requests, reentrant queued removal
preserving the installed catalog until storage removal runs, actual install/remove,
exact installed bytes, catalog reload in a new process, Unicode user-data paths,
a real Windows file handle denying deletion, preserved files/catalog on failure,
retry after unlocking, catalog reload after removal, and 20 immediate manager
shutdowns with a pending install. The native probe uses condition variables for
completion rather than timing sleeps. Raw per-process logs and `results.json`
remain under the supplied output directory.

This does not accept the redesigned Extensions page, external catalog/network
behavior, script activation or the native startup preference. The current suite
also checks malformed package/catalog XML and IDs, canonical and legacy UUIDs,
Unicode uppercase package URLs, reentrant discovery inspection, duplicate-entry
selection, actual selected replacement bytes, isolated storage/catalog results,
catalog retry without reinstalling, preserved existing scripts, missing later
resources, second-file collisions and duplicate manifest resources. Installation
stages all resources and publishes without replacement; failures clean up only
files owned by that attempt. A real physical failure denying rollback deletion
is not injected. POSIX publication is not exercised by this Windows driver.

The first collision baseline modifies the owned pre-existing fixture and the
missing-resource baseline leaves an untracked file. Those are rejected even when
the native failure assertions alone pass: final filesystem checks determine the
case result. Result getters add no fields to the public manager owner/entry ABI;
callbacks capture results before queuing frontend updates. The manager's checked
admission and explicit catalog-save APIs let the frontend report and retry actual
failures. These native checks still require separate model/window acceptance.
