# Juxtopposed design coverage ledger

Updated 14 October 2026. Milestone 0 is **in progress**. This is an initial inventory, not a complete screen audit or an acceptance claim. Original design and artwork: [Juxtopposed](https://www.youtube.com/@juxtopposed).

The Figma copy `zdNUYRVOH9lEtIxMYvh8DV` still reports its Starter-plan MCP quota exhausted. Three existing queue exports remain measured references. The supplied 20:59.915 AV1 walkthrough has now been sampled into 63 frames at a 20-second cadence; local frames and three contact sheets are in `build-windows/reference-video-frames`. Times below are approximate sample locations and must be refined for detailed specifications. Video frames include the original VLC, commentary, advertisements and concept UI: identify the actual redesign before measuring it.

| Surface | Reference / dimensions | Entry and real action | States and present status |
| --- | --- | --- | --- |
| Player transport and volume | Walkthrough around 01:30; preserved original assets in Qt pixmaps | Open media; native playlist transport and player volume | Playing/paused, mute, unavailable transport, focus, narrow layout: component verified; limited native passes, complete visual review pending |
| Timeline and seek preview | Around 03:30; preserved timeline assets | Player; native seek and tooltip | Paused seek now has runtime evidence; chapter/bookmark marks and final preview fidelity remain open: implementing |
| Queue | Figma `463:831`, 400 × 946; [manifest](queue/manifest.json); around 02:50 / 10:50 | Queue toggle; playlist selection/filter/reorder/remove/drop | Empty, selection, filtered mapping, focus, retention: component verified; native lifecycle/blur acceptance pending |
| Bookmarks | Figma `6320:13784`, 400 × 946; queue manifest | Bookmarks tab; native library add/remove/seek | Readiness, empty, media changes: component verified; native add/remove/seek pending |
| Chapters | Figma `6321:14045`, 400 × 946; queue manifest | Chapters tab; native chapter selection | Empty, current chapter, keyboard focus: component verified; real chaptered fixture pending |
| Audio/subtitle track selection | Around 06:10; exact node/dimensions/assets pending | Track entry; native track lists and selection | Selected/unavailable tracks, external subtitle opening, dismissal/focus: reference needed |
| Subtitle appearance | Around 05:30; exact node/dimensions/assets pending | Subtitle panel; native supported rendering settings | Presets, position, color, cue/timeline details need a closer audit: reference needed |
| Playback/video settings | Around 07:50 / 09:10; exact node/dimensions/assets pending | Settings entry; native speed/aspect/crop choices | Existing interim menu works; final sidebar structure, engine updates and focus: reference needed |
| Audio/video effects and sync | Around 07:30 / 08:50; exact node/dimensions/assets pending | Effects/settings entry; native filter and synchronization settings | Enabled/disabled choices and actual applied effect: reference needed |
| Shared header/sidebar | Around 11:10 / 11:50; exact node/dimensions/assets pending | Library navigation; native history and existing destinations | Expanded/collapsed navigation, active route, resizing, focus: reference needed |
| Video library | Around 11:30 / 11:50; exact node/dimensions/assets pending | Sidebar Video; index/browse/play actual files | Grid/list, thumbnails, loading, empty, selection, resume: reference needed |
| Music library | Around 12:10; exact node/dimensions/assets pending | Sidebar Music; native songs/albums/artists/genres | Real audio selection, metadata, persistent route: reference needed |
| Browse/local sources | Around 13:30; exact node/dimensions/assets pending | Browse/navigation; native source browser | Folder traversal, back, unavailable sources and file opening: reference needed |
| Open/source menu | Around 05:10 / 15:30 / 19:10; exact node/dimensions/assets pending | Main menu; actual file/folder/network dialogs | Local file, folder and supplied stream routes: reference needed |
| Conversion | Around 13:10; exact node/dimensions/assets pending | Convert/Save; native stream-output conversion | Source/profile/destination, progress, cancel/failure, playable result: reference needed |
| Preferences | Around 17:30 / 17:50; exact node/dimensions/assets pending | Settings; persistent native preferences | Category navigation, supported settings, restart, short/narrow windows: reference needed |
| Appearance and interface settings | Around 18:30 / 19:30; exact node/dimensions/assets pending | Preferences/appearance; real theme settings | Accent choices, preview, dark/light, restart, contrast/focus: reference needed |

Candidate surfaces from the plan still need confirmation: a distinct Home dashboard, playlist management layouts, library search/results, fullscreen/floating control variants, subtitle cue editing/export and an actual waveform. Preserve their planned backend acceptance paths; neither absence nor implementation has been established by the coarse sampling pass.

Next: inspect exact video moments and the Figma file manually or after MCP access returns, split compound surfaces into individual screens/dialogs, record canonical dimensions and original asset exports, and map concrete native models. Define audio, chaptered and subtitle fixtures alongside the working video/color fixtures. Keep missing references and backend dependencies visible instead of inventing geometry or claiming an entire family complete.
