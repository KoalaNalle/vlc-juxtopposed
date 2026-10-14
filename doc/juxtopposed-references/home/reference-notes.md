# Confirmed Home dashboard references

14 October 2026. Original design/artwork/walkthrough: [Juxtopposed](https://www.youtube.com/@juxtopposed). These unmodified video frames are reference evidence, not application assets or implemented-window captures. [Manifest](manifest.json) retains exact timestamps, dimensions and source/frame hashes.

| Frame | Confirmed content and limits |
| --- | --- |
| [13:47](827.png) | Compact/list Home: heading, Continue Watching with progress, Listen Again. Editorial zoom. |
| [13:49.3](829.3.png) | Lower list sections: Frequently Visited Folders, Frequently Visited Playlists and Frequently Listened Artists. |
| [13:56](836.png) | Clean whole-window grid Home after the preceding editorial scale-in: four video cards, three square audio cards and folder tiles below. Lower grid cards remain cut off. |
| [13:42](822.png) | Collapsed sidebar with pinned media/folder. Pins belong to the sidebar, rather than a fabricated Home section. |

The inferred 2× concept scale suggests a 38px title strip, 204px sidebar, roughly 320×180 video covers, 200×200 music/folder covers and 20px gaps. These are provisional video measurements, not Figma dimensions. Flat decoded colors include content #0F0F0F, sidebar #171717 and folder tiles #1A1A1A; retain authoritative exported tokens where available. Full lower-grid geometry and unshown empty/error/light variants remain unresolved.

## Actual model/action mapping

Continue Watching can use MLRecentVideoModel's native history/progress and normal MediaLib playback, which restores indexed-media resume state. Listen Again needs a recent-audio model around native audio-history APIs and MLAudioModel's artist/artwork roles. Filtering a mixed recent-media model loses required artist metadata. Recency must use default history criteria with explicit descending order.

Folders and playlists have real native IDs, names, media and activation routes but no exposed visit-frequency ordering. Persist actual successful navigation visits, including Back/Forward without duplicate refresh increments, and validate unavailable/deleted IDs. Playback counts of a folder's contents are not folder visits; playlist membership can overlap. A mixed folder-content route is needed for audio/video folders.

Artist frequency can aggregate the complete native audio play-count set by primary artist ID, using recent playback as a tie-breaker and actual artist metadata/routes. The dependency's artist sorting currently rejects PlayCount and falls back to alphabetical, so simply requesting that sort is insufficient. Native play counts describe completed/marked-played media; unfinished playback instead retains recent date/progress.

Ordered heterogeneous sidebar pins need real stable descriptors, activation/unpin actions and persistence. Native favorites support media/folders/playlists/artists but do not establish arbitrary-source pin order. The narration also confirms Recent covering files, playlists and network streams; no distinct Home network-section geometry is shown.

The [dashboard/recent-audio/activity port](implementation-checkpoint.md) now builds and renders all five sections using real library data. Seven native store/snapshot checks and twelve component suites pass. Desktop navigation and persistence remain pending at a Windows firewall prompt; no security permission choice or visual acceptance is claimed. Sidebar pins remain separate work. Continue the full milestones 0–8 plan.
