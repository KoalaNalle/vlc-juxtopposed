# History with Library disabled — 17 October 2026 guest

This is an implementation checkpoint toward the full beta. Native scoped checks,
the incremental ordinary GUI build and decoded-playback checks pass.
Full Privacy, native desktop/DPI, Windows 11 and final clean-package acceptance
remain open. The previous accepted ordinary runtime is preserved.

Saving and clearing playback history can now use the existing database with
Library browsing disabled. `vlc_ml_instance_get()` remains null in that state,
and the Library preference is unchanged. A separate lazy history handle is shared
by the primary player and released during instance cleanup. Asking whether this
capability exists does not open a database. Opted-out automatic playback with
playback-state restoration disabled, and secondary players, do not create the
lazy history database.

The bundled SDK patch adds a default-on background-operation setup flag.
History-only instances disable parser startup, filesystem/device refresh and
network-device invalidation; cached removable paths are reconstructed without
starting their filesystem factory. The VLC wrapper omits metadata extraction,
thumbnailing and network discovery in this mode, and rejects non-history control
operations such as adding roots, reloading or resuming background jobs. Existing
active-Library instances retain normal behavior. Unpatched system SDK builds do
not advertise the extra history-only capability.

Clear retains favorites, named playlists, explicit playback preferences and
roots, and preserves current playback. Native failure leaves epochs and local
stores unchanged; successful clear invalidates pending restores and prevents
current/retiring inputs from restoring cleared activity. A corrupt history-only
database is preserved and reports failure without a modal recovery question
under the player lock. Recovery in the Library-enabled flow remains separate.

When no Library frontend exists, the Qt clear callback also removes persisted
Home folder/playlist visits. Home visit writes/clear now use the common checked
settings transaction: a repaired file-sharing failure can be retried even when
the original borrowed QSettings retains its first error. Cache changes are
accepted only after successful persistence.

The actual loaded core and media-library plugin pass **40 Library-on** and
**52 Library-off** checks. The latter covers lazy startup, no database on opt-out
or secondary playback, shared identity, rejected indexing controls, cached
removable lookup, real SQLite rejection/retry, saved positions, policy changes,
restart and retiring inputs. An owned pending parser task and cached device,
mountpoint and folder rows remain unchanged through repeated
Library-off initialization, reads and clears. These C cases construct actual
player inputs; they are not decoded-playback acceptance.

The native Qt controller passes **17 cases**, including Library-off capability
without database creation, async clear with Library on/off, and actual locked
Home-store persistence failure/retry. Its real SQLite controller pipe mode passes
**9 checks each with Library on and off**. All scoped runs exit normally with
zero failures/skips and no forced cleanup. See [verification.json](verification.json),
[native-library-off.json](native-library-off.json) and
[native-controller.json](native-controller.json).

The final tuple uses core `186d65…2591`, GUI `843365…9e42`, media library
`e3ec5e…c0a`, C fixture `5ea190…c0e1` and Qt fixture `65c786…b051`.
The native fixtures do not load the GUI frontend; the ordinary tests below
exercise it separately. The full SDK patch is maintained under
`contrib/src/medialibrary` and
applied by its normal contrib recipe. One-job builds and all owned storage stay
on E: on this Windows 10 guest.

Real decoded playback exposed a separate restore error: a saved whole-media
percentage was interpreted relative to the new input's start/stop range. With
the same launcher, libVLC, GUI and media-library binaries, the original core
seeks to **3.625 seconds** instead of the saved **12.794 seconds**. It exits
normally but fails the resume check. The repaired core uses a private
whole-media restore command, preserves ordinary range-relative seeking, and
ignores saved points outside the requested clip. Saved title restoration also
retains the position for the later title-selection event. The exact paired
binaries and observed demux seeks are recorded in
[resume-coordinate-pair.json](resume-coordinate-pair.json).

All **six ordinary decoded-history cases** now pass: save, resume, points before
a new clip's start and after its stop, position permission OFF, and history OFF.
The test generates a 117 kB seekable Y4M video. Each process loads Qt/Direct3D 11,
produces decoded frames and quits normally. Library remains OFF, roots and parser
tasks stay empty, and the opted-out fresh profile has no database. Separate
redesigned/traditional/Library-enabled/local-HLS checks pass **four cases** with
zero QML/write/visual/PCR errors. See [decoded-playback.json](decoded-playback.json)
and [ordinary-verification.json](ordinary-verification.json).

On that same final tuple, unchanged native Library executables pass **18/42/9**,
Subtitle preferences **37**, timeline **4**, timer **25** and subtitle controller
**8**, with normal exits and zero failures/errors/skips or forced cleanup.
All **1280** [frozen final source/resource/test hashes](source-inputs.json) match; the Core-only restore
change leaves all **1263 Qt input hashes** unchanged. Meson registration syntax
parses; no native Meson build is claimed. This remains an incremental compatible
runtime, with final clean licensed packaging still open. The detailed regressions
are [Library](library-regression.json), [Subtitle preferences](subtitle-regression.json)
and [timeline/timer/subtitle controller](timeline-regression.json).

Reproduce with the patched contrib SDK, matching core/plugin and native fixtures:

```sh
make -C <build>/src -j1 libvlccore.la
make -C <build>/modules -j1 libmedialibrary_plugin.la
make -C <build>/test -j1 test_modules_medialibrary_history.exe
make -C <build>/modules/gui/qt -f Makefile -f <repo>/modules/gui/qt/tests/native_library/privacy.mk -j1 PROPOSAL_OUT=<fresh-output> privacy-preferences-native
```

Stage `.libs` Windows executables and matching DLLs into an owned portable
runtime. The Qt fixture requires the compiler's `libssp-0.dll` on this build.
Run sequentially under Windows:

```powershell
python test/libvlc/native_history_clear.py --package <runtime> --output-dir <fresh-on>
python test/libvlc/native_history_clear.py --library-off --package <runtime> --output-dir <fresh-off>
python modules/gui/qt/tests/native_library/run_privacy.py --package <runtime> --output-dir <fresh-controller>
```

The maintained decoded-playback runner uses a tiny owned Y4M fixture and ordinary
GUI process:

```powershell
python test/libvlc/native_history_playback.py --package <ordinary-runtime> --output-dir <fresh-playback>
```

The [Controls reference](../../figma/20261017/controls/README.md) was archived
while this GUI build ran. All 18 original SVG staged hashes match their downloads.
Six planned and two reserved Figma validation calls remain. Controls still has
no mounted category body, and its unresolved duplicate Remotes fields are
recorded explicitly. The full beta goal remains active.
