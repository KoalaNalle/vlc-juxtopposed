# External subtitle selection investigation, 23 October 2026

The combined private candidate selects a newly opened subtitle file and keeps
the paused picture in17 native cases, including SubRip/Matroska, ASS/Matroska,
mov_text/MP4, external ASS and a two-track subtitle-only Matroska file. Tests
cover WASAPI/no audio, previous ID/language/disabled choices, load-only requests,
missing files, explicit selection and resume. All42 native children exit
normally;653 snapshot hashes and2,327 actual frozen input copies verify.

There were three distinct problems. Existing selection preferences could veto
the load-and-select request. Directly forcing the new ES could leave the old
one selected because the main-ES hint was stale. Finally, a subtitle container
could reset the main video's PCR/preroll through its source wrapper. The final
candidate uses the existing exclusive user-selection path and keeps subtitle
source timing controls from changing the main media clock, starting at demux
opening. Generic associated sources keep their existing behavior.

The earlier candidates and failures are preserved. Candidate1 selected both
Finnish and the new file. Candidate2 passed the ten base cases but the MKS case
lost its held picture/time; the same MKS issue appears after explicit selection
on the unchanged build. Caption presence or a selected-ID flag alone therefore
does not qualify the workflow. The final candidate restores the actual marker
and text; reviewed native images show EXTERNAL FIRST CUE.

This is candidate evidence. No production Core source is applied, and no
ordinary build or actual desktop automatic-selection result is accepted here.
Current runtime stays
`build-windows/beta-subtitle-replacement-repair-20261023/ui-frame-marker.mkv-r3/runtime`,
Coreef3e47fd / GUId0ff14c6 / MKVf46d8d15 / WASAPI4e4e. Candidate67541fd9 must
still pass ordinary compilation, maintained regression and desktop gates.

Promote the extra ASS/MKS/missing-file checks into the maintained worker, apply
the four combined sources after preimage checks, then repeat native and visible
app checks. Playing/pending/nonseekable/recording/multi-select/repeated/restart
coverage, cold captions, layout/accessibility/DPI, physical outputs, Windows10,
review and clean packaging remain open under the full13-category/32-surface
beta goal. No install, commit/push, release/upload or shutdown occurred.
Native expected missing-file and dummy-window diagnostics remain visible.

- [Per-case checks, marker values and retained failures](comparison.json)
- [Source reasoning and next acceptance gates](review.json)
- [Combined proposed sources](candidate-r3)
- [Artifact hashes](checkpoint.json)
