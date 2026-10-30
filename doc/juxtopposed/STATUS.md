# Current status

Updated 30 October 2026. **The bounded R1/R2 repair, S01-S32 inventory and visual-work
selection are complete. Beta remains unfinished.** [BETA](BETA.md) is the single
scope/checklist: GA1 is complete; GA2-GA6 and all six beta gates remain open.
All three owned ordinary VLC processes exited normally. No native builder,
observer or owned desktop test remains running.

The [saved-state receipt](../juxtopposed-references/native/saved-state-20261030/README.md)
retains six intended original failures, 31 passing repaired saved-state rows and
20 passing profile-store rows. Ordinary Qt `a52ecd5c5c84` shows the stale-pin error,
Retry retaining initial/peer/pending rows, and cold persistence. R2's deterministic
interleaving is native-test evidence; no ordinary UI race is claimed. R3 remains.

The [graphical receipt](../juxtopposed-references/reviews/graphical-inventory-20261030/README.md)
has 43 ordinary observations and all 32 surface assessments. A targeted Figma MCP
read confirms the grouped shortcut-editor design. Major presentation gaps are
the flat source/profile menus, redesigned extra effects and shortcut editor,
video/local-browser context, and integrated online subtitle search. Some collection
and track-selector references remain unresolved. No whole-app fidelity score is
assigned. The selected order is in [BETA](BETA.md#selected-visual-work).

## Next action

On the next implementation resume, start the shared theme/contrast package:
Conversion/New Playlist and Queue/Bookmarks/Chapters need coherent Light surfaces
and foregrounds. Preserve authored Dark values, working consumers and all R1/R2
storage semantics. Pair ordinary states/themes before accepting the repair.
Also reproduce the one recursive-layout warning recorded during music teardown/
source replacement; its root cause is unassigned. An absent-filter
`subsdelay-factor` warning also remains unqualified. Do not call the inventory
warning-free or accept broad stability from normal exits.

The owner authorized explicit Cancel buttons on Firewall/app-permission prompts.
That narrow instruction is added to the installed Computer Use SKILL.md, backed
up under `C:\Users\Koala\.codex\skill-backups\computer-use\20261030-cancel-prompts`.
It does not authorize granting permission or changing security/privacy settings.
Plugin updates may overwrite the local edit. No security/privacy settings were
changed by automation during this checkpoint.

## Checkout and selected runtimes

- Authoritative checkout: `C:\Modding\vlc-juxtopposed`. The 30 October Git
  checkpoint is retained on `codex/beta-20261028` and merged into `master`.
  Next work branch: `codex/graphical-alpha-theme-20261030`, from that merge.
  The owner requested checkpoint/push, older-branch archiving, merge and new-branch
  preparation, then a stop to discuss the next goal. Implementation is paused;
  release, designer-contact and shutdown are not authorized by that request.
  Preserve the four unrelated experiments excluded by AGENTS.md.
- Selected scoped ordinary runtime:
  `build-windows/graphical-inventory-20261030/ordinary-r2/runtime`;
  Qt `a52ecd5c5c84`, Core `ccb6a2005ca5`, WebVTT `38eec724ba22`,
  TTML `9d74556528c6`, ML `e834a41a700b`, avcodec `3022efec963d`.
  Three children exit 0/603 modules, no forced cleanup, all recorded runtime,
  baseline and human-profile preservation guards pass. The isolated profile has
  generated test state and an unfinished appearance tour after inspection.
- Preserved R29 baseline:
  `build-windows/beta-record-notice-20261029/gui-r29/runtime`;
  Qt `7c7db5cb675e`, same selected non-Qt identities. R28 retains avcodec
  `545f715418c4`. Alpha fallback:
  `build-windows/beta-home-sections-20261028/history-actions-ui-r4/runtime`;
  Core `f09c45fe4075`, Qt `005fbe744643`. Pre-merge alpha commit
  `0694227ab8` is preserved by tags `archive/2026-10-30/master` and
  `archive/2026-10-30/codex/alpha-fallback-20261027`. Older development branch
  tips are retained under `archive/2026-10-30/`; the checkpoint branch remains.
  Recover an archived tip with `git switch -c <new-name> <archive-tag>`.
- Local work: `build-windows/beta-saved-state-20261030` and
  `build-windows/graphical-inventory-20261030`. All launchers are one-use; do not
  rerun them or reuse closed window handles. Incomplete staging and failed links
  are retained separately; ordinary-link-r4 is the successful build. Its 33 native
  consumers/two QML units have 3,556 recorded unchanged inputs and no private hooks.
- Reuse WSL `VLC-Juxtopposed-Home-20261018`, tree
  `/root/vlc-juxtopposed-build/win64`, lock
  `/root/vlc-juxtopposed-build/.vlc-build.lock`; one writer/two workers. Shared
  artifacts are preserved; stage exact candidates explicitly. Do not rebuild
  working components without changed dependencies or a concrete concern.
- Windows tools are in ignored root `tools/`; cold archives remain on D: through
  `build-windows/archive`. Active work stays on C:. Preserve the
  399,864,390,452-byte free floor (80% usage ceiling); recheck before large jobs.
  `D:\Media` stays read-only; this checkpoint used synthetic fixtures only.

## Preserved functional continuation

The graphical checkpoint precedes nonblocking W1/W2 depth. A walkthrough crash or
state-loss blocker takes precedence. All earlier functional requirements remain
in [BETA](BETA.md) and the [settings audit](SETTINGS.md), including:

- Finish ordinary output-notice Show Details/Messages and playback recovery.
  R29's notice expired before both attempted actions. Hover/focus holds it open;
  use a fresh current-plugin run. Check record-toggle/probe silence, distinct
  conversion/error paths and W1 estimator/remaining-format journeys afterward.
- Preserve caption/selection/paused-frame/FPS/preset repairs and their exact-tuple
  receipts. Secondary/per-track timing, language fallback, all profiles, embedded/
  container/role combinations, TTML wall-clock/SMPTE drop-frame and complete audio/
  recording tails are not broadly accepted. Do not repeat qualified sweeps.
- Retain [output notices/full generated output](../juxtopposed-references/native/ffmpeg-output-notices-20261029/README.md),
  [preset retry/held-frame repair](../juxtopposed-references/native/preset-retry-held-frame-20261029/README.md),
  [language lifecycle](../juxtopposed-references/native/subtitle-source-lifecycle-20261029/README.md),
  [ordinary subtitle defaults](../juxtopposed-references/native/ordinary-subtitle-defaults-20261029/README.md),
  [repaired-Core roles](../juxtopposed-references/native/repaired-core-subtitle-roles-20261029/README.md),
  [format/recording controls](../juxtopposed-references/native/repaired-core-formats-recording-20261029/README.md),
  [TTML parameters](../juxtopposed-references/native/ttml-time-parameters-20261029/README.md)
  and their linked earlier controls. The [diagnostic review](../juxtopposed-references/native/caption-regression-diagnostics-20261029.md)
  keeps AAC/PCM/teardown limits distinct. Compiled clients, current ordinary Qt and
  full source closure are separate scopes. R26's old-tuple launcher is unstarted;
  do not reuse it unchanged.
- Home close-under-load/storage failures; full consumers/workflows; design,
  accessibility and OS-DPI; multi-instance/physical endpoints; independent final
  review and clean-package checks remain open. Windows 10 AUDIOSES comparison is
  deferred until the original machine returns; preserve its prepared kit.
- The Git checkpoint includes `modules/demux/avformat/avformat_mux_queue.h` and
  `test/libvlc/native_avformat_notice.py`; preserve both with their consumers.
  The mux helper's original 41/5,772 and candidate 51/6,062 checks retain their own
  Core scope. Current guards do not qualify arbitrary earlier binary combinations.
