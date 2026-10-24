# Original Performance settings reference

17 October 2026. One high-fidelity context request for inventory-confirmed node
`6571:15958` returned complete code and its screenshot. This is the fourth
planned beta validation request: four planned and two reserved requests remain.

Original visual design and artwork: **Juxtopposed**, CC BY 4.0. The
[design copy](https://www.figma.com/design/zdNUYRVOH9lEtIxMYvh8DV/VLC-Redesign--Community-?node-id=6571-15958)
contains the natural 1048 × 865 frame. [performance.png](performance.png) is the
unchanged 1024 × 851 tool render, including effect padding. It is reference
evidence and must not be used as a runtime image.

[design-context.json](design-context.json) preserves the complete response.
[visible-text-nodes.json](visible-text-nodes.json) records the original text
nodes; [manifest.json](manifest.json) records screenshot/context identities and
all nineteen downloaded original SVGs, with intrinsic geometry. The information
asset is 18 × 18; the other supplied artwork is 24 × 24.

The fourteen authored settings/actions are hardware-accelerated decoding, video
post-processing, H.264 loop-filter skipping, decoding threads, frame dropping,
real-time priority, process priority, Rescan plugins, Advanced Options, encoder,
decoder, clock synchronisation, clock jitter and clock reference averaging.
Advanced Options is an expanded disclosure here: its Developer section is
visible. It must not be replaced with an unrelated native preferences link.
The final fields extend below the screenshot's viewport and remain present in
the complete context.

This reference does not establish implementation or native acceptance. Verify
actual module/decoder selection, bounded rescan behavior, Windows priority
semantics and clock consumers before claiming those fields functional. Preserve
unavailable hardware outcomes and actual saved values. In particular, confirm
what VLC's native priority options do before mapping the design's real-time
label to a different scheduling class. Keep priority testing confined to owned
processes and preserve the travel computer's one-job build limits.
