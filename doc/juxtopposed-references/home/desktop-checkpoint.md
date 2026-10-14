# Home desktop checkpoint

14 October 2026. Actual Windows Qt 6.8.3/D3D11 interface, with fixture-only native
library storage and a private portable copy of the existing VLC preferences.
The user's working `build-windows/package` remains preserved. This records
functional desktop evidence; original design geometry and visual acceptance
remain open.

The final Qt plugin SHA-256 is
`364bfe40d5af74435b08421a6fe1032fa8338158c8f208cda3652e1b3a500abe`.
It includes the persistent preset service and the Home history repair. The
matching plugin is saved in both the Home development package and a separate
preset package. Previous plugins remain recoverable.

## Reproduced failure and repair

Continue Watching → See All initially asserted in `MLBaseModel::setMl` at
`mlbasemodel.cpp:455`. `JuxtopposedHomeHistoryPage` explicitly assigned `null`
to its unused video/audio model. The native contract requires a real MediaLib.
Both models now retain MediaLib; only the selected model supplies the view.
The real video and audio collection routes both open after the repair.
Supplied-state component tests had not exercised that native setter contract.

## Actual interface checks

- The five-section dashboard renders real video/audio history and artist rows,
  plus understandable empty folder/playlist states.
- Continue Watching and Listen Again open their full native collections without
  the assertion. Video history searches `chaptered`, returns its single real
  result, and restores both entries when cleared.
- Grid/list switching works. Alphabetic sorting changes the actual video order
  from paused-seek first to chaptered first. Back returns to Home.
- Audio activated from Listen Again opens the real source-folder/waveform player
  paused at zero with its 30-second duration. Seeking yields native position
  `0.40085287846481876`. Brief actual playback and pause hold
  `0.4433532666666667`, displayed as 13 seconds.
- Back retains that exact position in the library mini-player. Open player
  returns to the same paused position. One earlier playback attempt naturally
  reached EOF while the automation was inspecting controls; it is not counted
  as a successful pause/Back check.
- The private-profile restart restores the selected list presentation and
  actual history. Complete folder/playlist/artist visits and restart journeys,
  live Clear History/reset, native narrow/DPI and reference fidelity remain open.

Captures and the assertion are preserved in [desktop evidence](desktop-verification-20261014/verification.json).
The original fixture contains no album art; the music player currently exposes
VLC's opaque white fallback. A separate dark fallback correction is prepared,
with its actual Windows rendering still pending.

The user manually dismissed both Windows firewall prompts. No security choice
was automated. An isolated launch with fresh native preferences triggered the
first-run wizard; that owned test was terminated and relaunched with an exact
copy of the existing preferences, preserving the user's previous choices.

An independent route audit found that video folders and playlists can seed
their first visit through ordinary library routes. Audio-only folders currently
lack such a route. A successful Add Folder → validated native entry-point visit
repair is being prepared separately; no invented folder IDs or completed Home
visit acceptance are claimed here.
