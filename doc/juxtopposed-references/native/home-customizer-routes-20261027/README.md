# First toolbar route and Quit repair - 27 October 2026

The first Playlists tile opened Home because startup mistook a one-entry history
for empty history. Startup now preserves a valid first route and its properties.
The Quit tile called an unavailable method; it now uses the native quit provider.

The callback regression fails five expected cases before and passes all ten
after; fifteen PageLoader groups also pass. Ordinary GUI311 verifies first
Playlists, Home, retained paused input and normal Quit with zero error diagnostics
and all guards intact. Earlier tile and customizer journeys have separate scope.

Recording is still open. One ordinary recording contains no decoded video;
another has only one second of video alongside longer audio. Longer desktop and
native comparison recordings decode AV, which does not establish full duration
or synchronization. No recording fix is claimed here.

The [checkpoint](checkpoint.json) retains exact identities, original failures,
owned captures, probe results and limitations. Continue from
[STATUS](../../../juxtopposed/STATUS.md); all beta gates remain open.
