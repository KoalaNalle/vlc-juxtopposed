# Plugin rescan backend — Windows 10 guest

The new cache-generator output mode scans one explicit plugin inventory and
writes a separate candidate. It preserves the installed cache. The Qt worker
validates that candidate with Core's native parser, checks source/cache
fingerprints, and publishes through QSaveFile with direct-write fallback
disabled. It holds a rescan lock, handles cancellation and deadlines, and joins
its owned child/worker during shutdown. It never unloads the running bank.
Temporary candidates are beside the installation, so this E: runtime keeps
them on E:. Product code does not copy or hard link plugin DLLs.

The generator converts Windows UTF-16 arguments to UTF-8 and reports output
failure after native validation. An explicit Core/tool protocol import prevents
a new generator from initializing an older Core that could write in place.
Core's malformed-cache path now destroys entries parsed before the error.

| Evidence | Result |
| --- | --- |
| Actual Qt/Core integration | Sixteen ordered cases, zero failures/skips and zero cleanup warnings; normal test-process exit |
| Real generator | Full inventory under quoted/Unicode paths; actual playing and paused input independent; running bank count unchanged |
| Native cache consumer | Parser accepts the complete candidate and rejects corruption; a cold cache-only child sees Qt metadata while its owned DLL link is absent |
| Real write rejection | Output pointing to a regular file fails without changing the source cache or that file |
| Owned negative fixtures | Empty/corrupt/nonzero outcomes, locked publication/retry, timeout/reaping, cancellation/reuse, source/cache changes, simultaneous scan rejection and shutdown |
| Paired Unicode probe | Before: ASCII 1,832 bytes versus Unicode 27-byte header. After: both 1,832 bytes, including a Unicode output directory; source caches preserved |
| Older Core/new generator | Expected entry-point rejection 0xC0000139 before initialization; source cache unchanged and candidate absent |
| Ordinary new-Core/generator runtime | Five normal playback exits: redesigned/traditional, Library-on, local HLS and bounded Downloads D3D11VA sample |

Core is
ea9253fef4f596a29b158b8d331aed05eaff59e4ced0f012c9d19e347b8b7557;
the generator is
6c50e5c447f2c6fe32023379f73d531cf73d43695dca41c47c9f7e2e6ab6d3a7.
The ordinary GUI remains the accepted encoder-checkpoint binary
753feb6236a3e6457260ee98dfd45e93da1244f6251b5a59e371bcceddc1346f.
It does not contain or expose the new Rescan worker. The tested native fixture
compiles the actual production helper; no private fixture/test executable ships
in the ordinary runtime. The ordinary generated cases have no QML, write,
visual-callback or PCR diagnostics. Downloads media/capture remain private and
the source stat is unchanged.

[source-inputs.json](source-inputs.json) records nineteen unchanged scoped
inputs on base e78444c4d420214e99bda826de447734131d62f3. Autotools resolves
the standard explicit integration target; bin, Qt and test Meson syntax parses.
This does not claim a native Meson build or a rebuilt production GUI.
[manifest.json](manifest.json) records raw/public receipt hashes.
[Reproduction](../../../../test/libvlc/native_plugin_cache_rescan.md) describes
the matching Core/tool build and maintained native runner.

Two earlier attempts remain under build-windows/beta-performance-20261017:

- rescan-acceptance-r1 ran thirteen functional cases but left three temporary
  hardlinks to active logger/keystore images. The first runner incorrectly
  reported success despite Qt's cleanup warning. calibration-invalidated.json
  explicitly rejects it; the current runner rejects that warning. Separate
  output removes the DLL staging that caused the problem.
- rescan-acceptance-r2 safely rejected the Unicode source's empty candidate.
  It had ten passing/five failing cases, including failures dependent on the
  missing valid template. The paired native probe isolated the narrow Windows
  argument bug. The accepted r3 suite requires the real positive path and all
  sixteen checks to pass.

Timeout/cancel/shutdown negatives intentionally terminate their owned scanner;
the test process itself exits normally. These checks accept the recorded
backend paths, not every third-party plugin or the redesigned Performance page.
Connecting the action, the remaining Performance fields, Recording/Extensions,
full later-tuple regression, themes/DPI, Windows 11 and the clean final beta
package remain open. Earlier runtimes and the five unrelated untracked files
are preserved. Nothing is pushed or published.
