# Track timing context and ordinary folder picker - 26 October 2026

Core e1767d37 / GUI aa089f75 passes the same ordinary two-file journey that
previously applied a stale timing estimate to the next file. Input changes now
discard pending audio/primary/secondary marks without publishing a delay or
changing per-track inheritance. Native tests also prove cancellation does not
publish an adjustment and that new pairs remain usable; all fifteen subtitle
functions pass. The rate and GUI seek notification hooks are implemented, while
their ordinary discontinuity journeys remain open.

Six timing-marker buttons now connect to the page's navigation root. Before the
repair, all six native binding rows fail; afterward they pass along with the four
existing cancellation rows. Ordinary Voice Heard and Sound Spotted checks return
to the list with Escape; another Escape closes it.

The ordinary Playback settings picker passes initial Cancel, an owned folder with
spaces and an accented character, 1 MiB save, cold restart and reopening at the
saved directory. Cancelling again preserves the path and paused clock. Video's
second granularity entry still uses generic custom feedback for 1 MiB because its
preset list omits that value. Native live-buffer write/rollover/resume evidence is
in the separate integration receipt; no broader live protocol is inferred here.

[checkpoint.json](checkpoint.json) binds reports, original config bytes, frozen
sources, full binary identities and before failures. Every ordinary run exits
normally with unchanged guards, one 602-module load and no error diagnostics.
Initial test compilation trouble and existing libtool diagnostics are preserved.
Full behavior, design, hardware and package gates remain open. Continue from
[STATUS](../../../juxtopposed/STATUS.md).
