# Home: audio-only folder registration and failed persistence

14 October 2026. The rebuilt Qt interface registers an explicit Add Folder
request before native indexing can publish its root-added event. A new root
becomes a Home visit only after that successful event and lookup of its real,
available, unbanned native folder ID. An explicitly added existing indexed root
uses a separate pre-add lookup because it does not emit a second root-added event.

This closes the backend bootstrap gap for audio-only roots: the ordinary Videos
folder browser filters them out, and they previously could not establish their
first Home frequent-folder entry. Unsolicited scans create no visits. URL
comparison preserves encoded Unicode and reserved filename characters. Requests
carry serial/reset epochs; late callbacks and worker results cannot recreate a
cleared visit. Missing native IDs have a bounded monotonic retry. The history
opt-out prevents writes, and callback/worker teardown follows the service lifetime.

The visit store now commits its cached candidate only after a successful settings
sync. A failed write or clear restores the previously accepted document, including
when storage later recovers or the settings object is destroyed. The settings
object's sticky error prevents further writes until settings restart.

## Native evidence

[Results](folder-bootstrap-verification-20261014/results.txt) show **11 passed,
0 failed, 0 skipped, 4,228 ms**. [Verification](folder-bootstrap-verification-20261014/verification.json)
records matching adjacent native libraries, generated media fixtures, fresh
isolated storage, a 150-second outer deadline and an actual 4.3-second run.

The four new cases exercise actual blocked storage and blocked clear; accepted
request/reset/stale-event decisions; and real audio-only indexing with a Unicode,
space, `#` and `%` folder path. The latter receives an actual queued native
root-success event, obtains a stable typed native ID, confirms audio content and
absence from the video-folder list, reads mixed folder contents, restarts the
library/settings and checks unavailable/banned roots and reset-before-event guards.
The original visit, metadata, malformed-data and restart cases also pass.

These checks exercise production store, snapshot, lookup and request helpers with
real native media-library events. They do not instantiate the complete
`MLHomeActivity`/MediaLib worker integration or drive the native folder picker.
Actual Add Folder → indexed Home card → audio route → restart/reset remains a
separate desktop gate. Existing [Home desktop evidence](desktop-checkpoint.md)
retains its own scope.

## Build and acceptance limits

The native Home test and Qt interface both compile. The staged interface DLL has
SHA-256 `88452491563814608b6d4790752aa4ad9875204780e35e95d5886ab69aed99c6`.
It also contains the [New Playlist checkpoint](../advanced/new-playlist-checkpoint.md)
and the unknown-duration correction seen in the actual rebuilt desktop app.
The test executable SHA-256 is
`20775cb2370b657391ac9e0350fff3aca793aa2bdf835c8e9a914281d92666a1`.

This is a compiled checkpoint with native backend evidence. It does not accept
the full Home design, all desktop visits/reset journeys, visual fidelity or the
first publishable beta. Raw decoder logs, private preferences and the generated
media-library databases are excluded from this evidence directory.
