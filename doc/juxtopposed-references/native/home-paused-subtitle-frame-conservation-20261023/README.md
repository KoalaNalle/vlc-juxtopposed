# Paused subtitle frame conservation, 23 October 2026

The corrected Core candidate restores paused subtitle replacement and preserves
the actual video frame in the generated marker test. Timer video points include
the VLC_TICK_0 origin; demux seeks add it to a relative media date. The candidate
uses the current displayed-video/SMPTE point, removes its origin for the demux
request, and keeps the original point for timer publication. Pending user seeks
retain their own target domain.

An independent11-bit frame marker identifies1500 frames at25fps. The unchanged
build holds frames25/125 but loses selected captions. The previous candidate
restores captions but advances125 to131 without audio. The corrected candidate
holds25 through two language changes and125 through six more changed selections,
with and without WASAPI. Captions remain visible and resume advances the marker.
Five other original/remuxed/colored cases also retain the replacement behavior;
all three no-audio cases conserve their reported paused times.

There are56 successful selected-caption observations,16 actual frame-marker
conservation observations,154 verified snapshots and11 normal native exits.
Source/runtime/fixture hashes remain unchanged. These are candidate results:
no production source has been applied, and the diagnostic DLL is not accepted
as the ordinary build. The working runtime remains
`build-windows/beta-caption-layout-20261023/ui-after-r1/runtime`, Core000f1159.

Next: maintained regression, ordinary Core build and desktop checks, followed
by ASS/other-container and policy coverage. Cold initial captions, automatic
external selection, exact caption layout and stale WASAPI public paused time
remain open. The full beta stays active. No install, commit/push, release/upload
or shutdown occurred. Native diagnostics are retained.

- [Paired frame and caption comparison](comparison.json)
- [Scope, source reasoning and remaining checks](review.json)
- [Source trace](normalization-investigation-r1.json)
- [Checkpoint and artifact hashes](checkpoint.json)
