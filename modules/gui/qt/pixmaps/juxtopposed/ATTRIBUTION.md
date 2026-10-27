# Design and artwork credit

**The VLC interface redesign and the original SVG artwork in this directory
were designed by Juxtopposed. All credit for that visual design belongs to them.**

- Creator: https://www.youtube.com/@juxtopposed
- Original work: VLC Redesign
- Design: https://www.figma.com/community/file/1691465999134370980/vlc-redesign
- Walkthrough: https://youtu.be/ERFeOgqk324
- Artwork license: Creative Commons Attribution 4.0 International
- License: https://creativecommons.org/licenses/by/4.0/

The SVG files are original exports from the Figma design. The implementation
adapts the design into Qt Quick controls with dynamic playback state, volume,
seeking, keyboard focus and a stacked layout for smaller windows. These code
and layout adaptations do not change the authorship of the original design.

The queue tab, move, sort, repeat, shuffle, remove and add SVGs are unchanged
individual exports from Juxtopposed's Queue, Bookmarks and Chapters frames.
Their hashes and the original panel references are recorded in
`doc/juxtopposed-references/queue/manifest.json`. Native selection, accessible
actions, dynamic data and narrow layouts are implementation adaptations.

InterVariable.ttf is the Inter typeface by Rasmus Andersson, distributed under
the SIL Open Font License 1.1. Its source is https://github.com/rsms/inter and
its license is included in Inter-LICENSE.txt.

VLC source code retains its existing GPLv2-or-later license and original author
notices. This fork's implementation is independent of the original designer.

The `global-settings-*.svg` files are unchanged original exports from the
General Global Settings frame (Figma node `999:5047`). The `*-light.svg`
player icons are unchanged original exports from Player - Light (`2425:13624`).
Both frames belong to the same credited VLC Redesign. Native settings binding,
keyboard/modal behavior, shared accent selection and unshown Light surfaces
are implementation adaptations. The design and artwork retain the CC BY 4.0
license and Juxtopposed attribution above.

The `volume-background-light.svg` and `volume-fill-light.svg` files are unchanged
original exports from Player - Light (`2425:13624`), volume background
`I2425:13645;491:1733;244:226` and original full fill vector
`I2425:13645;491:1733;244:229`. The latter is the complete 86px fill inside the
designer's clipped 53px preview group, not a stretched clipped fragment.
Dynamic clipping to native volume and Light/Dark asset selection are functional
adaptations; the original artwork retains Juxtopposed's attribution above.

The playback-settings-*.svg files are unchanged original exports from the
Playback Global Settings frames 3062:12552 and 3072:13408. Their information,
chevron, slider-tick and selected Playback artwork retains the same CC BY 4.0
authorship and license above. Dynamic native settings, byte-labelled time-shift
presets, keyboard focus and small-window layouts are implementation adaptations.

The audio-settings-*.svg files are unchanged original exports from Audio
Defaults 3073:15879 and the expanded Effects 3180:17537 and Advanced
3180:17645 references. Original artwork and design retain Juxtopposed
authorship and the CC BY 4.0 license above. The native value ranges, dynamic
frequencies/device choices, unknown-state controls and narrow layouts are
implementation adaptations.

The video-settings-*.svg files are unchanged original exports from Video
Defaults 3073:14187 and expanded 3073:15375. Original design and artwork
retain Juxtopposed attribution and CC BY 4.0 above. Native defaults, practical
hundredth-step picture controls, explicit reset confirmation and small-window
layouts are implementation adaptations.

The subtitles-settings-*.svg files are unchanged original exports from
Subtitle Defaults 3180:17989 and expanded 6536:13908. Original design and
artwork retain Juxtopposed attribution and CC BY 4.0 above. The supplied native
choices, editable numeric convenience presets and narrow layouts are explicit
implementation adaptations.

The library-settings-*.svg assets are unchanged Juxtopposed exports from
Library frame 6542:15251, under the same CC BY 4.0 attribution above. Existing
assets are reused only when their complete bytes match the supplied originals.

The controls-settings-*.svg assets are unchanged Juxtopposed exports from
Controls frame 6536:14482, under the same CC BY 4.0 attribution above. Their
original 24px intrinsic sizes and complete bytes are preserved. Checked native
settings, authenticated remote setup and narrow layouts are implementation
adaptations; hashes and the original reference are retained under
doc/juxtopposed-references/figma/20261017/controls.

Network settings artwork (`network-settings-*.svg`) is the unchanged original
Juxtopposed design, CC BY 4.0, from Figma node 6543:15649 in file
zdNUYRVOH9lEtIxMYvh8DV. Original hashes and geometry are retained in
`doc/juxtopposed-references/figma/20261017/network/manifest.json`.
The cached screenshot is reference evidence and is not a runtime asset.

Recording settings artwork (`recording-settings-*.svg`) is the unchanged
original Juxtopposed design, CC BY 4.0, from Figma node 6571:16429 in file
zdNUYRVOH9lEtIxMYvh8DV. Original hashes and geometry are retained in
`doc/juxtopposed-references/figma/20261017/recording/manifest.json`.
Native values, checked persistence, truthful recording help and narrow layouts
are implementation adaptations. The screenshot remains reference evidence.

Sidebar artwork (`sidebar-*.svg`) is unchanged Juxtopposed artwork, CC BY 4.0,
from Figma node6261:15094 in file zdNUYRVOH9lEtIxMYvh8DV (18 October 2026).
The original 24px root dimensions and complete source bytes are preserved.
Runtime palette coloring and the documented horizontal reflection reproduce the
interactive icon states using VLC's existing SVG image provider. Reference
screenshots are evidence only. The export's gear placeholder for Videos/Music
is deliberately excluded; those two original assets still require correction.

The inactive Home outline (`sidebar-home-inactive-27471.svg`) is also an
unchanged24px original, from the already-cached Player node247:445, nested
sidebar layer I856:2587;848:1868;177:88;70:829. The filled active Home and
outlined inactive Home use their respective original vector exports.

Title-strip artwork is unchanged Juxtopposed artwork, CC BY 4.0, from the cached
Player node 247:445 and Home node 6261:15078 in file zdNUYRVOH9lEtIxMYvh8DV.
Original 24px roots and the 18px menu root are preserved. Folder and Close reuse
identical existing exports. Runtime palette coloring/horizontal reflection and
scaled wrappers follow the original slots. Cached screenshots remain reference
evidence only; no screenshot is used as a runtime asset.

The unchanged `sidebar-videos-7c4e4.svg` and `sidebar-music-61ad2.svg` exports
come from Videos (71:857) and Songs (71:851), respectively. Their approved
focused-read contexts, reference renders and exact hashes are recorded in
`doc/juxtopposed-references/figma/sidebar-icons-20261018/`.

The unchanged24px `music-{songs,albums,artists,genres,add}-*.svg` originals come
from music sidebar component858:5001, retrieved with Figma MCP on27October2026.
The folder slot in4767:23606 reuses the byte-identical existing
`controls-settings-cf6e1.svg`. Songs, Albums and Folder retain the original
horizontal reflection in their wrappers; the vector files are unchanged.
