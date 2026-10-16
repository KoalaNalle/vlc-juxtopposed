# Privacy reference and native field audit — 17 October 2026

One high-fidelity Figma design-context request for the previously inventoried
Privacy frame `3062:11911` returned the complete code and its screenshot. No
browser-site restriction was bypassed. This consumes one of the eight planned
beta validation calls; seven planned and two reserved correction calls remain.
The original 1048 × 865 frame dimensions come from the saved page inventory.

![Original Juxtopposed Privacy frame](privacy.png)

This is the original design reference, not an application capture. The
[archived context](design-context.json) retains original node IDs, styling and
all nine field/action rows; only temporary asset-source prefixes are normalized
to the adjacent local `assets` directory. The [manifest](manifest.json) records
unchanged screenshot/SVG bytes, hashes and SVG root dimensions. Original design
and artwork: [Juxtopposed](https://www.youtube.com/@juxtopposed), CC BY 4.0; see
[designer attribution](../../../../../modules/gui/qt/pixmaps/juxtopposed/ATTRIBUTION.md).
These files are reference material and are not registered as application assets.

The [field map](field-map.json) distinguishes existing native consumers from
missing permissions and services. Remembering a position is a storage policy;
Playback's existing Never/Ask/Always restoration preference cannot replace it.
The obsolete Qt history filter has no current native consumer. Clear needs real
failure feedback, Home activity coordination and protection from stale input or
worker results. Internet metadata must share the existing Library preference.
The Last.fm backend exists, but legacy transport, interface activation, validation
and service behavior remain unaccepted.

The design repeats “Last.fm username” on both credential rows. The backend has
separate username and password options. That mapping is an inference, preserved
as an ambiguity rather than silently rewriting the original label.

The audit found the media-library wrapper discarding `clearHistory()`'s actual
database failure. A native real-database regression and repair are under
[the history-clear checkpoint](../../../native/guest-history-clear-20261017/README.md).
This supplies a reliable native result for the future UI; it does not implement
or accept the complete Privacy controller/page. Search, save/restart/reset,
normal/narrow/light/DPI presentation, all field consumers and final package
verification remain open. Reuse this cache before making another reference call.
