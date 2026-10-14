# Restored Figma MCP reference inventory

14 October 2026. MCP page metadata, high-fidelity design context, screenshots and read-only prototype inspection now succeed after the user's Professional Full-seat connection. The [manifest](manifest.json) records actual node IDs, natural dimensions, unchanged screenshot hashes and top-level frame inventory for the single `0:1` Page 1. The separate saved browser permission block is unresolved and has not been circumvented.

Original design: [Juxtopposed](https://www.youtube.com/@juxtopposed), [community file](https://www.figma.com/community/file/1691465999134370980/vlc-redesign), [accessible design copy](https://www.figma.com/design/zdNUYRVOH9lEtIxMYvh8DV/VLC-Redesign--Community-). Preserve the existing CC BY 4.0 [designer attribution](../../../../modules/gui/qt/pixmaps/juxtopposed/ATTRIBUTION.md). These are reference images, not shipped UI chrome or actual application captures.

| Original node | Confirmed reference | Implementation implication |
| --- | --- | --- |
| `6261:15078`, 1920 × 1080 | [Home](home.png) | Original expanded sidebar is 217 pixels, title strip 40; current provisional shell requires separate alignment. The visible Home viewport cuts off lower content; complete scroll states remain open. |
| `6259:14954`, 713 × 589 | [New Playlist](new-playlist.png) | 112-pixel thumbnail; 186-pixel dashed file area; 100 × 32 footer actions. Existing compiled 670 × 554 geometry is an earlier video-based adaptation, still awaiting alignment. |
| `3744:12361`, 1920 × 54 | [Subtitle timeline controls](timeline.png) | Same-height alternate control bar: 28-pixel seek strip, 24-pixel cues, 8-pixel playhead, endpoint chevrons and plus/minus controls. It replaces the ordinary seek lane. |
| `5187:24487` | High-fidelity settings row cached locally | “Show Subtitle Timeline on Seekbar” toggles to `3744:12361`; the subtitle icon `3744:12403` returns to ordinary controls `491:1779`. Static cue navigation/zoom buttons have no prototype reactions; detailed timing/pan/zoom semantics remain to be validated against the walkthrough. |

All thirteen Global Settings category frames are now grounded in the inventory, alongside interface/playback/video/audio/subtitle state variants, first-run screens, profiles, customization, shortcut and subtitle-scope dialogs. No redesigned conversion form is confirmed: the scoped text/prototype reads identified three Convert menu labels, not a form or profile specification. Complete reference/state reconciliation remains a beta gate.

Screenshots from design context can include effect padding or fit to a smaller image; their manifest distinguishes capture dimensions from natural node geometry. Full generated context and downloaded original vector assets remain in ignored `build-windows/figma-beta-20261014` for reuse. Runtime media covers/backgrounds remain dynamic native content; reference thumbnails are not shipped as user media. Successful reference access does not establish current native app visual fidelity.
