# Graphical inventory — 30 October 2026

All S01–S32 are mapped to current presentation, original references or explicit
reference gaps, real services and remaining acceptance. The maintained assessment
and selected work order are in [BETA](../../../juxtopposed/BETA.md#graphical-inventory--30-october-2026).
This dated receipt freezes the observations; it is not another roadmap.

The [inventory](inventory.json) records 32 surface families, 43 observations, source
hashes, screenshot hashes, three normal process exits and limitations. Captures
use ordinary Qt `a52ecd5c5c84` with Core `ccb6a2005ca5`, without private hooks.
The generated collection has two videos, chapters, two embedded audio/subtitle
tracks and four tagged music files. No private media was copied into this receipt.

Representative captures:

| Observation | Evidence |
| --- | --- |
| Saved-pin conflict, Retry and cold restart | [Conflict](captures/saved-pins-conflict-0.png), [three retained rows](captures/saved-pins-retry-three-retained-0.png), [cold start](captures/saved-pins-cold-home-0.png) |
| Light player, Queue, Chapters and Bookmarks | [Player](captures/s01-s02-s12-video-light-0.png), [Queue](captures/s03-queue-light-0.png), [Chapters](captures/s05-chapters-light-0.png), [Bookmark](captures/s04-bookmarks-populated-light-0.png) |
| Shared library and source browsing | [Home](captures/s14-home-history-light-0.png), [video rows](captures/s17-video-library-0.png), [albums](captures/s19-albums-0.png), [local browser](captures/s20-local-browser-empty-light-0.png) |
| Current nested source menu | [Media submenu](captures/s21-media-menu-0.png) |
| Dialogs and a reproduced contrast defect | [Network](captures/s22-network-dialog-0.png), [New Playlist](captures/s23-new-playlist-dark-0.png), [Light Convert](captures/r3-conversion-light-0.png) |
| Online attachment layout gap | [Current handoff](captures/s25-online-attachment-gap-0.png), [original integrated form](../../figma/20261026/subtitle-attachments/file-online.png) |
| Redesigned panel and traditional effects fallback | [Video panel](captures/s09-video-controls-light-0.png), [More video effects opens Audio Effects](captures/s09-s10-more-effects-traditional-audio-1.png) |
| Presets and format-limited timeline | [Preset choices](captures/s26-presets-light-0.png), [embedded-source unavailable state](captures/s27-cue-timeline-embedded-unavailable-0.png) |
| Music player and waveform | [Ordinary music](captures/s28-music-player-0.png), [original video frame](../../advanced/0738-music-player.png) |
| Settings, setup and customization | [Light General](captures/s30-global-settings-light-0.png), [setup appearance](captures/s31-setup-normal-light-0.png), [shortcut tour](captures/s31-setup-shortcuts-light-0.png), [customizer](captures/s32-customizer-light-0.png), [profile naming adaptation](captures/s32-new-profile-light-0.png) |

The [original Keyboard Shortcuts frame](../../figma/20261030/inventory-shortcuts/keyboard-shortcuts.png)
was checked through Figma MCP at node `2437:14832`. It confirms grouped Action,
Hotkey and Global Hotkey rows with search/filter/edit affordances. The current
`controls_shortcuts.cpp` instead builds a QWidget dialog using KeySelectorControl.
That subdialog is source-assessed here; there is no fresh ordinary editor capture.
The screenshot's [manifest](../../figma/20261030/inventory-shortcuts/manifest.json)
retains node, purpose, dimensions and hash, without the expiring download URL.
No Figma content was edited.

The exact-time [video extracts](video-references/manifest.json) distinguish the
11:45 video-folder/player composition from the 12:10 editorial music-category
zoom and the 06:10 subtitle-attachment form. These are different contexts. A full
video library, track selector, or unshown collection detail cannot inherit their
geometry without closer evidence. The inspected 09:10 Playback reference shows
only Speed; missing narrated features must not be invented on that page.

Method and limits:

- Inputs used Computer Use and refreshed observations. Captures are representative
  states, not an exhaustive walkthrough of all subdialogs, every category field,
  error, responsive size or theme. Transient accessibility snapshots sometimes
  precede the settled render; inspected screenshots and later settled observations
  establish the visual assessment. Failed/stale input attempts are not counted
  as successful interactions.
- The ordinary app reached each surface family. Some contain traditional fallbacks
  or missing designed variants, so this does not accept GA2. Native services were
  preserved; no mock save, conversion, download or other success was simulated.
- No numeric visual score is assigned. Ordinary maximized captures are 1920×1032,
  while many originals are 1920×1080 or editorial zooms. Media, state and complete
  typography/layout/artwork/responsive criteria are not all matched. GA3 and broader
  Windows DPI/monitor tests remain open.
- The first run's Firewall and Welcome/privacy prompts were handled by the owner.
  Appearance-only setup was later reopened in the same consented isolated profile.
  Privacy choices, firewall rules and normal user settings were not automated.
  The owner separately requested and received a local skill exception for explicit
  Cancel buttons. No prompt cancellation was counted as a test result here.
- All three owned children exited normally with preservation guards intact. No
  desktop child remains running. This is not clean-package, Windows 10, physical
  endpoint, multi-instance or complete stability acceptance.

Diagnostic review:

- Each process retains `main error: window closed` at normal user-window shutdown.
  [The existing notification](../../../../src/video_output/video_window.c) logs it
  in `vout_display_window_CloseNotify`; teardown immediately follows in these logs.
- Probe messages for unvalidated subtitle codecs, generated AAC zero-sample frames,
  initial window sizing and UPnP search timeout are retained in `inventory.json`.
  They do not supply audio sample or network-discovery acceptance. See the earlier
  [diagnostic scope](../../native/caption-regression-diagnostics-20261029.md).
- Cold-start inventory has one Qt recursive-rearrange warning immediately after
  music stream/waveform teardown and blur-layer release, before source replacement.
  The exact root cause is unresolved. It also has one missing `subsdelay-factor`
  lookup while the corresponding filter is absent. Do not call this a warning-free
  run, blame either on R1/R2 without a control, or accept GA5 from normal exits.
- MKV EOF/missing-cue and dummy-mux end-of-stream messages remain with the short
  generated fixtures. They do not establish broad format/output completeness.

Raw logs and the generated profile stay in the ignored local run directory.
Only the reviewed summaries, generated-media UI captures and reference extracts
are retained here. The original review and previous frozen receipts are unchanged.
