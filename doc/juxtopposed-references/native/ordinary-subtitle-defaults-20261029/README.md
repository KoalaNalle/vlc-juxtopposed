# Ordinary subtitle defaults and cold restart — 29 October 2026

Five ordinary VLC processes use the same Core4a/Qt7c7/WebVTT38/TTML9d runtime.
Generated profiles are copied forward only after the preceding child/observer
has exited and its preservation checks pass. No personal media is used.

R20 starts paused with an English default explicitly seeded in its generated
configuration. Saving Finnish through Global Settings > Subtitles preserves the
current English caption and held video. The saved native key is sub-language=fin.
R21 cold-starts with that configuration and automatically renders Finnish; the
settings dropdown also retains Finnish. Saving Enable subtitles Off preserves
the current Finnish caption and displays the restart notice.

R22 cold-starts with captions disabled, at the same generated source position as
R21. Global Settings retains Off. Saving On shows the restart notice and leaves
the current input disabled. The ordinary saved-delay slider writes 1,200 ms,
represented by native sub-delay=12 (tenths of a second).

R23 cold-starts with subtitles enabled and the 1,200 ms delay. At source 20.5 s
it renders SUOMI ENSIMMÄINEN, while R21 with zero delay rendered SUOMI
KESKIMMÄINEN. Both show the same blue video and marker, with visible elapsed
00:00:00 because of the start-time origin. Playback settings reports 1.200 s.
Global Settings retains 1,200 ms. Saving its default back to zero does not show
a restart notice, change the current cue or change the live 1.200 s delay.

R24 cold-starts with the saved zero default, renders SUOMI KESKIMMÄINEN again at
the same source/frame marker, and reports live delay 0.000 s. No timing edits
or manual track selection are needed at these cold starts. The unavailable
secondary-delay control remains disabled with only one selected subtitle.

Each child and observer exits normally with code 0, loads 603 modules and has
no captured error diagnostics or forced cleanup. All fixture, source, runtime,
fallback, previous generated profile, helper-input and human-config guards pass.
The exact commands and hashes are recorded. Only generated target-window
captures, selected nonprivate settings lines and scoped reports are published;
private profiles, media binaries and raw logs remain ignored locally.

This qualifies ordinary embedded-track language selection and cold default
ownership, including an actual cue/held-frame comparison for delay. A cold
process is not proof of every same-player Next/Previous or source-reuse path;
external automatic-language fallback, saved presets and all format/role
combinations remain separate. It does not close beta/design/DPI/package gates.
