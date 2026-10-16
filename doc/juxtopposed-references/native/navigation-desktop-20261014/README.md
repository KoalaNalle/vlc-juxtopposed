# Mounted Windows navigation — 14 October 2026

The rebuilt Windows app opens Home, Videos and Playlists through their native
models and History routes without the earlier unknown `initialIndex` property
errors. Keyboard Open reaches the real empty and mixed playlist details. Back
restores the second playlist's selection: Enter without moving the selection
reopens that playlist with the same three ordered rows. The app quits normally
with WinAPI exit code 0. This is a scoped development checkpoint; the complete
beta, design fidelity and broader Music/Network navigation remain unfinished.

| Actual journey | Observed result |
| --- | --- |
| Home and Videos | Five Home items, two Videos items, Home/Back/Forward routes load |
| Playlist overview and Home | Two authored playlists; Home, Back and Forward load |
| Empty playlist keyboard Open | Its own title/breadcrumb and `No media found` |
| Empty details → Home → Back | The same title and empty details return |
| Second playlist keyboard Open | `alpha-one.flac`, `home-video.mp4`, repeated `home-video.mp4`, in that order |
| Second details → Back → Enter | Restored overview selection reopens the same title and three rows |
| Retained primary | The chapter fixture stays paused at displayed `00:00 / 00:18` during these contents/history operations |
| Quit and log | Normal Ctrl+Q, independently queried exit 0; no recorded QML property/loader errors |

The [empty details](captures/12-empty-playlist-details-keyboard-monitor2.jpg),
[restored empty details](captures/14-back-empty-details-monitor2.jpg),
[mixed contents](captures/15-mixed-playlist-details-monitor2.jpg),
[restored selection](captures/16-restored-second-playlist-monitor2.jpg) and
[Enter reopening that selection](captures/17-restored-selection-reopens-mixed-monitor2.jpg)
are unchanged native captures. The user moved the app to monitor 2 before the
remaining visible tests. Those captures have a 1920×1032 window; the initial
Home observations used the preceding monitor layout. These are not canonical
Figma-sized fidelity comparisons. The fixture media and library are generated
project data; no personal media, library database or preferences are included.

The actual Qt plugin is
`03d93c91c056442d7392866eec4e39ca6f12f0fb745529fadb8a50b8adddd797`.
The source/runtime manifest records its matched core and preparser identities.
The pre-build Qt source manifest freezes all 1,068 current Qt directory files,
including unused assets and tests, excluding Python bytecode. Those hashes were
rechecked before this evidence was copied. This is source identity for the
incremental build, not a claim of a clean final-beta build or complete compiler
provenance. The executed resource regeneration, C++/resource/link log and staging
sequence are retained under `build/`. Libtool/static-link warnings are preserved.
The manifest's base-revision transcription was corrected against Git; the
executed publication script retains that historical transcription error.
The ordinary working package was retained, and the fresh test package used
separate generated-fixture userdata and explicitly disabled ML network discovery.

All seventeen Windows component/source suites pass in the preserved local run.
The separate [loader proof](../navigation-leaf-20261014/README.md) records eleven
adapted QML gates and nine actual QCore History results, including both preserved
negative baselines. They do not substitute for this mounted journey. Likewise,
the [literal conversion proof](../conversion-literal-path-20261014/README.md)
accepts its eleven-case native controller repair separately; conversion GUI on
this exact Qt build was not exercised in this navigation run.

## Input distinctions and remaining gates

Early video-row pointer inputs did not establish selection and were followed by
an unexpected player transition. Individual callback receipt was not captured.
The centered empty-card Play action entered Player while retaining the existing
paused primary; a second click entered fullscreen. The play cover/accessibility
Play path and the actual body double-tap/Open path are distinct. The later
Tab/Right/Return Open journey reached real playlist contents. No native mouse
Open or Videos pointer-index restoration pass is claimed. Filled-playlist Play
and empty-Play handling require separate checks.

A small window size appeared during later keyboard navigation and was expanded
again. Its cause is unestablished; narrow/short/scaled layouts remain unaccepted.
Immediate accessibility reads sometimes described the preceding scene. Settled
captures and displayed native screenshots were used before recording outcomes.
Displayed paused time and retained player do not prove exact native clock or
complete queue equality. Generic Music/Network history, rapid same-route
generations, clean-profile/restart, all-format operation, Figma comparisons and
final packaging remain separate beta requirements.

`desktop-observations.json` preserves the observed sequence, explicit limits,
all eighteen original capture hashes and the normal-exit query. Twelve relevant
captures are copied here unchanged. `evidence-index.json` records twenty-six
original/public pairs. Text/JSON substitute generic checkout, build and user path
labels without changing assertions or source/runtime identities. No screenshot
was resized, cropped or edited for this proof. These results do not complete the
[full beta plan](../../../juxtopposed-beta-plan.md).
