# Original Light volume reference

The `juxtopposed_light_volume_reference.png` file is an unchanged 89 by 28 pixel
Figma screenshot of the Light player volume control designed by **Juxtopposed**.
It is a test oracle and is never loaded as application artwork.

- Designer: https://www.youtube.com/@juxtopposed
- Original design: https://www.figma.com/community/file/1691465999134370980/vlc-redesign
- Working file: `zdNUYRVOH9lEtIxMYvh8DV`
- Player - Light frame: `2425:13624`
- Screenshot node: `I2425:13645;491:1733`
- License: [Creative Commons Attribution 4.0 International](https://creativecommons.org/licenses/by/4.0/)
- Project artwork credit: `modules/gui/qt/pixmaps/juxtopposed/ATTRIBUTION.md`

The screenshot and original SVGs have not been recolored, redrawn, cropped or
resampled. Their SHA256 identities are recorded in the adjacent asset fixture.
The full Light fill is the complete original 86 by 24.56 SVG from node
`I2425:13645;491:1733;244:229`. The designer's separate 53px clipped preview is
explicitly rejected. The Light background is 89 by 27.3633 from node
`I2425:13645;491:1733;244:226`.

The test compares three opaque interior fill/background pixels at the same
51.5px clip boundary, with a maximum four-channel-value tolerance in each RGB
channel. It intentionally excludes the screenshot's opaque backdrop and
antialiased exterior. These samples supplement strict original-artwork hashes
and rendered geometry, rather than accepting the entire player or native GPU
rendering as visually identical to Figma.

The generated historical comparison changes exactly the two current volume
source expressions back to their old Dark-only filenames. Everything else is
the current production bar, its current dependencies and its current palette.
This narrowly proves that Light asset selection does not change Dark rendering;
it is not a snapshot test for unrelated historical transport/layout behavior.
