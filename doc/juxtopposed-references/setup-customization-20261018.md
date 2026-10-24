# Setup and customization reference reconciliation — 18 October 2026

This uses the already cached Figma metadata and the user's local original video.
No new Figma read was made. Exact-time extracts, source hashes and observations
are retained under `build-windows/beta-compact-interaction-20261018` in
`reference-video-extracts-v2.json` and `next-workflow-reference-inventory-v2.json`.
These references establish requirements; they do not establish implementation
or native/visual/package acceptance.

## First-run flow

The original has two confirmed pages, both 1090 × 759 in the cached metadata:

1. Theme/accent choice with Light, Dark and System cards, a player preview,
   accent choices, Back and Next. Theme-state examples include `1902:3670` and
   the other cached Onboarding variants. The video shows this at 19:50.
2. A basic keyboard-shortcut tour, Back and Finish: `1902:6770`, confirmed in
   the video at 19:52. It covers play/pause, seeking, previous/next item and frame,
   subtitle/audio track cycling, fullscreen, volume, queue, file browser,
   snapshot and recording.

The existing native `FirstRunWizard` provides privacy/folders/theme/layout
foundations. Its current page sequence is not the original two-page flow, and
does not provide the original shortcut-tour page or accent selection. Reuse
the existing services, preserve explicit privacy choices and checked persistence,
and verify Back/Next/Finish and fresh-process completion behavior. Do not
silently replace or approve the user's privacy policy choices.

There is a reference inconsistency to reconcile: the tour shows `<` / `>` for
previous/next item, while the cached Keyboard Shortcuts settings frame
`2437:14832` shows P / N. Displayed bindings must work and stay consistent with
the actual configured shortcut model. The native player already provides real
previous-frame support through `PlayerController::framePrev()` and
`vlc_player_PreviousVideoFrame`; do not substitute a guessed frame duration.

Current implementation checkpoint: the two-page setup now has a separate draft
palette and reuses the actual authored player controls as an inert preview.
Back retains drafts; Finish uses checked native/Qt writes and records completion
last. Escape postpones setup without saving. The native consent page stays
separate and setup waits for its saved decision and closed window. No privacy
choice is approved by the new QML flow. Its first Back is disabled because this
two-page appearance flow follows the separate native policy dialog.

The shortcut tour reads active startup bindings, including the existing
previous-frame default Shift+E. File Browser is honestly shown as unassigned;
the screen does not invent a binding. Completing the remaining shortcut consumers
is part of the broader shortcut work package. Complete modifier/key chords stay
together where space permits, and longer rows scroll; narrow windows
reflow the theme cards, preview, swatches and tour while preserving the footer.
Light/System use the existing native palette where the original does not supply
tokens. These are explicit functional adaptations, not full fidelity claims.

Setup backend R3: 11 native checks, normal exit. Setup component: 65 checks;
shared host: 112 checks, including waiting for the separate policy window,
exclusive modal routing, save failure/retry, postponement and completion.
Shared appearance 43 and Light-volume seven groups still pass. Exact receipts
and current build status are in ignored `build-windows/beta-setup-20261018`.
Private R3 native setup passed Cancel (49 checks), Finish/failure/retry (60)
and full OS-process restart (18), all with normal exits and no Qt diagnostics.
Ordinary R2 also completed Next and Finish using actual Windows keyboard input,
with completion marker 1 confirmed on disk. Its copied isolated profile retained
the privacy choices the user had already handled. These runs do not verify a
new user's actual consent-widget interaction. Native captures exposed three
visual defects: System-card right corners, unselected swatch opacity and split
shortcut chords. Their source repairs now pass private R4's three setup journeys
and both Library-on/off Home-menu journeys (185 each), with normal exits and no
Qt diagnostics. Native captures show all three corrections. Ordinary R3 passes
five playback smokes and actual Windows Shift+F10/Menu input; it restores the
completed setup profile without reopening the popup. The native Controls suite
passes 19 cases including startup-binding snapshot assertions. The
[scoped receipt](native/home-setup-20261019/verification.json) records exact binary
and source identities and five reviewed captures. Complete visual/OS-DPI/package
acceptance stays open; component scaling does not establish Windows DPI behavior.

## Customize Player

The full redesigned palette is visible at 19:15. The earlier 19:03 and 19:10
frames show the legacy VLC toolbar editor as a comparison and must not be used
as the redesigned visual target. Cached `Player - customize` frames include
`2107:5604`, `2160:6137` and their variants, each 1920 × 1080; their instance-only
metadata does not expose the complete palette's internal geometry.

The original palette has Close, Cancel and Save. Items move out into the player
to add/reorder them and back into the palette to remove them. The 31 visible
tiles are:

| Row | Original tiles |
| --- | --- |
| 1 | Spacer, Divider, Playback Speed, Forward 10s, Backward 10s, Forward 15s, Backward 15s, Stop, Repeat |
| 2 | Shuffle, Record, Snapshot, Picture in Picture, HDR, Zoom, Right Sidebar, Quit VLC, VLC Settings |
| 3 | Go Home, Thumbnail, File Name, Artist Name, Folder Name, Playlist Name, Properties, Replay, Chapters |
| 4 | Favorite, Bookmark, Bookmarks, Playlists |

The 19:28 alternative layout moves the timeline above the transport row and
places metadata at lower left. Customization therefore includes the player
layout and metadata widgets, not merely a list of extra toolbar buttons.
Reuse native layout/profile and action services, preserve the primary input,
and verify preview, drag/drop, cancellation, checked saving and restart. A tile
must have a real consumer or an honest capability-dependent unavailable state.

## Profiles

The 19:35 submenu confirms Movies, Shows, Music, Work and New. The cached frame
is `6648:20666` (651 × 633); its text metadata primarily exposes the parent menu.
The existing toolbar profiles are partial infrastructure. The earlier beta
requirements also call for preference state to follow the selected profile;
do not accept a layout-only dropdown as completion of that scope. Map the
native field ownership, global/profile scope, live/restart semantics and
save/switch/reset/conflict behavior explicitly before accepting it.

The user confirmed the profile scope on 19 October: playback, audio, video,
subtitles and player layout follow the selected profile. Appearance, keyboard/
mouse controls, privacy, network access, library locations and credentials stay
global. Profile changes must preserve the native setting's existing live,
next-input, next-snapshot or restart semantics. They must not pause/reopen the
current input merely to make a saved default appear immediately applied.

Original design: [Juxtopposed](https://www.youtube.com/@juxtopposed). Keep the
existing designer attribution and artwork licensing. The video extracts remain
private reference material rather than shipped runtime media.
