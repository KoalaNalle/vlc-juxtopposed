# Native Windows plugin-cache rescan

PluginCacheRescan runs the installed vlc-cache-gen as an owned child. Its
--output mode scans one explicit source and writes a candidate into a separate
owned directory beside the installation. It does not copy or hard link DLLs,
write the source cache, or unload the running process's module bank.

Core and the generator have an explicit output-protocol symbol dependency so a
new generator cannot initialize an older Core that would write in place. Normal
startup and ordinary in-place cache generation retain their existing behavior.
Windows command-line paths are converted from UTF-16 to UTF-8. In output mode,
the tool returns failure if Core cannot parse a nonempty generated cache.

The worker snapshots the source inventory/cache, holds a rescan lock, and bounds
preparation, child lifetime and verification. It rejects a missing/corrupt result
or changed source. Core's native parser validates the complete metadata; partial
parse failure releases already-created entries. Publication uses QSaveFile with
direct-write fallback disabled. File-lock or write failure preserves the current
cache. A successful rescan applies to subsequent VLC starts.

Build matching Core and the generator before building the native test:

~~~sh
make -C "$build/src" -j1 libvlccore.la
make -C "$build/bin" -j1 vlc-cache-gen.exe
make -C "$build/modules/gui/qt" -f Makefile \
  -f "$repo/modules/gui/qt/tests/native_library/cache-rescan.mk" \
  -j1 PROPOSAL_OUT="$freshBuildOutput" plugin-cache-rescan-native
~~~

The normal Autotools test_qt_plugin_cache_rescan extra-program target and the
Meson target of the same name are also registered. They are explicit integration
targets: the generic test environment cannot supply their owned runtime. No
native Meson compilation is implied by syntax validation.

~~~powershell
python test/libvlc/native_plugin_cache_rescan.py --package $ordinaryPackage --core $newCore --cache-generator $newGenerator --test-exe $nativeTest --output-dir $freshOutput
~~~

The sixteen ordered Qt cases include real generation of the full DLL inventory
under a quoted/Unicode path; native parser corruption rejection; cache-only cold
reload with an owned Qt DLL link absent; actual playing/paused input independence;
real generator output failure; empty/corrupt/nonzero fixture outcomes; locked
publication/retry; timeout, cancellation, stale inventory/cache changes,
simultaneous-scan rejection and worker shutdown. The runner requires all cases,
normal test-process exit, an unchanged source package and zero cleanup warnings.
Negative timeout/cancel/shutdown cases intentionally terminate their owned child.

Private scanner modes in the native test binary exercise failures without
altering an installed scanner or host settings. They are not production paths.
The generated cache and all temporary data remain owned by the chosen runtime.
These backend checks do not accept the Performance page, full theme/DPI behavior,
other settings, every third-party plugin, Windows 11 or a final beta package.
