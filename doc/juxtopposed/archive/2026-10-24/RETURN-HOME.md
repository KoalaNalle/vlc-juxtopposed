# Return home — VLC Juxtopposed weekend, 16–18 October 2026

**Departure checkpoint, 18 October. Development is paused for packing. The beta
is incomplete and the newest source is a work-in-progress checkpoint.**
The user authorized committing and pushing this work. That is a source backup,
not a release or a claim that the final candidate is ready to distribute.
Exact local revision, push status and transfer state are recorded beside the
migration kit in `RETURN-HOME-STATE.json`. Automatic approval review blocked
the outgoing push because it includes private handoff/crash-investigation and
Figma-reference material. The local checkpoint is prepared; the push requires
specific user approval for those contents. No GitHub push has occurred in this
departure step.

## What we achieved

- Moved development to `E:\Modding\vlc-juxtopposed`, with portable tools,
  profiles, caches, tests and Linux disks on E:. Verified the migrated source
  and original transfer archives. Hardware virtualization works; WSL 2 and the
  retained cross-compiler run, so a bootable Linux installation was unnecessary.
- Investigated the 16 October machine blue screen: the dump identifies the WSL
  virtual-GPU teardown path in `dxgkrnl!DXGVIRTUALMACHINE::IsWsl2Guest`. Disabled
  unneeded WSL GPU support and GUI integration; subsequent gentle CPU-only
  boot/shutdown passed. This does not establish the underlying driver cause.
  Kept builds to one worker, one build/playback job at a time, with 4 GiB/two CPUs.
- Improved playback and conversion correctness: live timeline clocks and pause
  handling, ownership of audio visualization output, empty-block file output,
  WAV packet durations through the final sample, and continuous fixed-rate
  conversion. Preserved the strict 88,200-sample FLAC tail regression.
- Brought more of the original settings design to native behavior: Privacy and
  coordinated history policies; Controls with mouse/remote scrolling linked by
  default and independently configurable when unlinked; Network with Windows
  proxy policy and verified TLS; Performance preferences and actual native
  consumers; Recording preserving VLC's native original-stream preference; and
  Extensions with catalog plus local packages, checked storage and failure/retry.
- Improved actual settings accessibility and Light placeholder contrast. Native
  subtitle preset save/edit/retry/restart, menu dismissal, opener focus and
  paused-player key ownership received fixes and separate regression evidence.
- Improved Home's bootstrap/history, audio fallback, resume behavior, asynchronous
  media identity and floating preview bounds, including the default 320×360 case.
- Added ordered, typed sidebar pins with native activation, persistence and retry.
  Pins survive history clear and Library OFF; missing/replaced database identities
  are guarded. Fixed early empty-artwork requests in network delegates.
- Added native Recent views and history access while Library browsing stays OFF,
  including lazy storage, decoded local/HTTP reopen, coordinated clear without
  resurrection, saving opt-out and pending-open destruction checks.
- Mounted the original shared Home/player title strip with nine original SVG
  slots and real native controls/menu. Fixed the media-name notification ordering
  bug found by actual native media switching. The player drawer reuses typed routes.
- Most recently, reproduced and fixed clipped arrow-focused routes in a short
  player drawer. Sixteen native processes passed 3,150 checks on that pre-final
  revision, including actual Windows title double-click/system drag, Add-menu
  focus restoration, and decoded audio/video volume/mute isolation.
- Used the two newly approved Figma reads for the correct original Videos and
  Music icons. Original SVGs, full contexts, previews and hashes are saved and
  their shared-sidebar bindings are committed. The extra two-read budget is spent.

These are scoped checkpoints, not an aggregate test count for the latest source.
The detailed history and proof links remain in `PROJECT_HANDOFF.md` and
`doc/juxtopposed-beta-plan.md`.

## Exact stopping point and unresolved issues

1. **Ordinary local HLS crashed once, and the cause remains unresolved.** The
   newest tested ordinary GUI (`b3233ec9df0bc61b...`) passed three file cases, then
   exited 0xC0000005 after about 3.3 seconds. Windows reports
   `AUDIOSES.DLL 10.0.19041.5794 + 0x1358f`, report
   `21009420-6c1f-445e-bad5-94b70cabcac5`. Subsequent bounded debugger and valid
   private-capture replays quit normally. That does not prove a fix. The original
   temporary Windows dump was already removed; no original AV stack was recovered.
   The Downloads case in that failed five-case run was never reached.
2. **A separate late Qt shutdown warning was diagnosed.** The cache timer belongs
   to `QQuickPixmapCache::instance()::self`, destroyed after the GUI thread exits.
   A maintained QtDeclarative patch purges unused cache entries and stops its
   timer during application cleanup. One SDK object was compiled successfully;
   all 204 other static-library members match the original bytes. The installed
   patched `libQt6Quick.a` SHA-256 is
   `9967abc133210d25c185665fdaafdad56eeedd4226157182a07e42d53b76b3df`.
   This is an incremental dependency change, not a clean SDK rebuild or a fix
   for the AUDIOSES access violation.
3. **The final GUI has not been rebuilt against that dependency.** New private
   cache-lifecycle and original-icon checks, icon bindings and stronger late Qt
   stderr logging are saved but uncompiled/unreplayed. The R5 3,150 checks predate
   those changes. Earlier zero-warning receipts did not catch messages emitted
   after VLC's logger shut down; do not relabel them as strengthened-log results.
4. At departure, the small supplied-model sidebar component check passes **21**
   checks. Python syntax, SVG hash/root-size/resource registration and a reverse
   dry-run of the Qt patch pass. These checks do not establish native acceptance.
5. The five pre-existing unrelated untracked files are unchanged and excluded
   from Git commits, but included in the manual return copy. They are listed in
   `build-windows/beta-recent-history-only-20261018/preserved-untracked.json`.

Last previously accepted ordinary *scoped* runtime:
`build-windows/beta-shared-strip-20261018/ordinary-runtime-r1`.
GUI SHA-256 `63aa786f2b6d228193914007774c1373a1b9c2f5c01fee952ee30913148e2578`;
Core `0cdebe2f6950939228eac94304393ef946b1947c80d1cbb8a8f83abe546a8072`;
media-library plugin `e3ec5ed46c8abb561c99c45864261efb003abfa276ee9b341a50bacfba213c0a`.
It predates the final work and is retained as a comparison, not a final beta.

Current raw work is under `build-windows/beta-window-navigation-20261018`.
Start with `WORK-CHECKPOINT.md`, `qt-cache-sdk-rebuild.json`, `window-r5/verification.json`,
`ordinary-playback-r1/live/process.json`, `ordinary-hls-fault-event.json`, and
`qt-timer-stack-r3/`. The timer dump is a deliberately fatalized-warning diagnostic,
not the original AV dump. `save-proof.py` was prepared before the crash and has
false acceptance assumptions: **do not run it unchanged**. Historical helper
scripts must be reviewed for paths, output revisions and current distro first.

## What I recommend doing at home, in order

1. Verify the copied source/Git state and cold Linux disk, restore the environment,
   then read this report, the latest handoff and `SECURITY-TRIP.md`. The payload
   includes generated fixtures and the original design reference video. Retain
   the departure copy while adapting machine paths. Avoid overwriting a different
   home checkout or importing the disk under an already registered distro name.
2. Finish the interrupted Qt/icon revision first: freeze fresh production/private
   inputs (including the new Qt recipe/patch and both assets), build the private
   GUI once, then run the native cache lifecycle, exact artwork/geometry and all
   existing 16 modes with late stderr diagnostics enabled. Inspect the renders.
   Build a fresh ordinary GUI and repeat file playback, local HLS and the bounded
   Downloads sample sequentially. The ordinary wrapper also needs stronger late
   logging; it has not yet received the runner change. Capture an actual AV stack
   if AUDIOSES recurs; a normal replay alone must not close the issue.
3. Continue compact navigation with Tab, wheel over list/padding, pointer focus,
   pin fallback and error/Retry keyboard paths; retain direct native clock,
   volume and mute checks. Native Windows 10 drag/double-click coverage does not
   accept monitor DPI transitions, multiple displays or Windows 11 Snap.
4. Audit the remaining confirmed design surface/state inventory against the
   original references. Complete Home/player/Recent fidelity, narrow/detached/
   fullscreen layouts, Dark/Light/System, accent, fonts and accessibility. Recent's
   internal composition remains provisional where no full original reference
   was supplied. Reconcile old plan checkboxes with actual scoped evidence.
5. Run relevant playing/paused/restart/save/reset/cancel/error regressions on the
   final GUI and complete platform checks. Produce one clean, matched ordinary
   Windows x64 package from a fixed revision, with resources, licenses, source/
   build instructions and checksums; verify fresh profile, Unicode/space relocation,
   installed-VLC coexistence and required Windows 11 coverage. Exclude all private
   test modules, profiles, personal media and dumps from distribution.

Release boundaries remain: **beta = confirmed Juxtopposed redesign and stability;
V1.0 = vanilla VLC parity** (including the disc/ISO/folder-play/AutoPlay discussion
with Jux); **V1.5 = possible additional YouTube/Twitch extensions**. No designer
outreach or release publication is authorized by the departure push.

## What to copy to the SSD

A compact manual-copy payload is prepared at:

`E:\Codex\migration-returns\20261018-home\payload\`

**Copy that folder's contents to the SSD root**, preserving `Modding`, `Codex`
and `Apps`. With your SSD currently F:, the destination is `F:\`. This handoff
prepares files on E: only; it does not format or write the SSD.

The payload contains:

- `Modding\vlc-juxtopposed\`: source, assets, documentation, hidden `.git`, all five
  unrelated untracked files, selected raw evidence/scripts, essential generated
  fixtures, the original reference video, the original comparison package, the
  prior accepted scoped ordinary runtime and the current private/ordinary GUI
  candidates plus Qt cache diagnostics/backups.
- `Modding\AGENTS.md`: guest path and crash/pacing instructions.
- `Codex\migration-20261016\`: migration records, scripts, profiles and the original
  16 October transfer ZIP. This is a private backup; it is not pushed to GitHub.
- `Apps\WSL\VLC-Juxtopposed-Travel\`: the **current cold `ext4.vhdx`**, about 42 GiB,
  containing the latest Linux build/toolchain and patched dependency.
- `Apps\Git\`, `Apps\Python312\`, `Apps\VlcUiTests\`, `Apps\Wheels\`,
  `Apps\WinDbg\`: the matching portable tools and Qt wheels.
- `RETURN-COPY-MANIFEST.json` and `START-HERE.txt`: file-selection/size receipts
  and the final local revision plus push status.

Do not additionally copy the full raw checkout: repeated hard-linked runtimes
have about **1,008 GiB logical size** and Explorer can expand those duplicates.
The compact payload intentionally omits those reproducible repetitions, caches,
temporary output, swap and old Linux import TARs. Its manifest records selection
and sizes; **a full transport checksum of the large Linux disk is not claimed**.
Verify the SSD copy before removing the E: originals. The requested GitHub push remains blocked pending that specific approval;
do not rely on an online backup yet. Git does not contain build binaries or WSL.

WSL was shut down and its disk opened exclusively to confirm it is closed before
staging. **Keep WSL stopped until copying finishes.** The payload uses same-drive
hard links to avoid consuming E:'s remaining space; it is a departure staging
area, not an independent backup until you copy it to the SSD. Do not restart
work against the original files or modify the staging area during the copy.

At home, preserve the SSD backup and copy the cold disk to its intended working
location first. A `.vhdx` can be registered with `wsl --import-in-place` using a
new unused distro name; review home registrations and drive mappings before
running it. Guest scripts require E: paths. Do not run historical C:-based helpers
or blindly replace the home machine's WSL settings with this guest's limits.
