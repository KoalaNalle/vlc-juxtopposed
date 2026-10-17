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
behavior, multi-file installation rollback/collisions, catalog-write failure
reporting, script activation or the native startup preference. Those require
separate checks before the page can be accepted.
