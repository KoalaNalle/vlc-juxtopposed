# Shared shell and library reference notes

Recorded 14 October 2026. Original design and artwork belong to [Juxtopposed](https://www.youtube.com/@juxtopposed). The user authorized continuing with saved manual Figma exports and the supplied walkthrough video while Figma MCP remains quota-limited. Browser access to Figma was declined; this audit uses local material only.

These are frames from the original design walkthrough, **not captures of the implemented VLC app**. Preserve them as references; do not use screenshots, their thumbnails or embedded media as application artwork.

| Reference | Exact video time | Source pixels | Confirmed content |
| --- | --- | --- | --- |
| [Library beside player](library-player-11m45.png) | 705 seconds / 11:45 | 3840 × 2160 | Compact navigation rail; folder list with thumbnails; breadcrumb/back/forward; sort/view/search; separate queue beside the player |
| [Expanded library](library-expanded-11m55.png) | 715 seconds / 11:55 | 3840 × 2160 | Videos/Music/Network/Playlists navigation with labels; breadcrumb; searchable multi-column compact item layout; floating queue/mini-player |
| [Music categories](music-categories-12m10.png) | 730 seconds / 12:10 | 3840 × 2160, editorial zoom | Music destination; Add, Songs, Albums, Artists and Genres; item count and sort/view/search |

The 11:45 frame fills the video and appears to show a 1920 × 1080 concept at 2× scale. This scale is inferred, not a Figma measurement. At source row y=1000, the rail ends at x=92, the drawer ends around x=852 and the right queue spans about x=3004..3805. At source column x=1800, the title strip ends around y=76 and the player area ends at y=2055. This suggests approximately **46px rail, 380px drawer, 38px title strip and 400px queue** at the inferred concept scale. Use ±2 source pixels for flat-region boundaries and recheck against original exports before visual acceptance.

The 11:55 frame instead uses an expanded label sidebar around 204 logical pixels wide, with an inset around 5 pixels. The detached queue has a preview and media/folder information. This is a distinct variant from the measured 400 × 946 queue tabs already preserved in the queue manifest; do not substitute one for the other.

Video colors are affected by the author's editing and compression. Flat decoded regions include approximately #171717 for the rail and #0f0f0f for the drawer. Retain the measured Figma tokens and bundled Inter wherever available; do not treat these video samples as authoritative color or font measurements. Original rail/sidebar icons still need individual exports. No visual score is assigned here.

## Native integration map

- Navigation and return: `History`, `MainDisplay.qml` page routes and `MainCtx.playerView`. Keep the existing Home reachable, even though a separate redesigned Home dashboard has not yet been confirmed by these frames.
- Video library: `MLVideoModel`, `VideoAll.qml` and existing selection/context-menu/drag adapters. Folder/group discovery has `VideoAllDisplay.qml` and `VideoGroupDisplay.qml`; the concept's actual folder breadcrumb structure needs an explicit folder model mapping.
- Music categories: current `MusicDisplay.qml` exposes artists, albums, tracks, genres and playlists. Display Songs while retaining the real tracks route. Sort/filter must reach each active native model.
- Playlists: VLC has separate video/music playlist routes and `MLPlaylistListModel`; the concept's shared destination needs a mixed-media route and real create/open behavior.
- Add folder: `MLFoldersModel::add(QUrl)` already performs the operation through the media library, but this model currently belongs to the native folder editor and lacks QML registration. The old `mlsettings` page URL in `MainDisplay.qml` points to an absent QML file. Expose a small real folder-opening action or reachable editor instead of relying on that stale route.
- Search, loading and empty states: reuse active page search/sort contexts and media-library loading/count notifications. Keep source indexes correct after filtering.

The next implementation step is the shared navigation/header, then a connected add-folder → browse → play → return/resume path. Exact new artwork, Home reference, expanded/collapsed/narrow states and native evidence remain open.

## Repeatable media fixtures

[generate_slice_fixtures.sh](../../../test/libvlc/generate_slice_fixtures.sh) creates synthetic local media with FFmpeg/FFprobe into a fresh directory. It does not embed the designer's example media or populate the product with demo data.

- 18-second all-intra MPEG-4 video: red, blue and green for six seconds each; verifies real paused frames and resume timing.
- Chaptered Matroska: the same video, 440Hz FLAC audio, three actual chapters at 0/6/12 seconds and embedded SRT cues. The matching external SRT permits subtitle loading tests.
- 30-second PCM audio: amplitude-modulated 220Hz signal with title/artist metadata; permits actual waveform and music-indexing checks.

The local generated files have been probed: chapter count 3, video/audio/subtitle streams present, expected 18/30-second durations. Native chapter, subtitle, music-indexing and waveform paths still require their own acceptance evidence.
