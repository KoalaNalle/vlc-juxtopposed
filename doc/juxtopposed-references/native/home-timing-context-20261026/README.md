# Native timing context - 26 October 2026

Core `883f7e51` / ordinary GUI `6e5fa604` is the selected scoped runtime.
Seventeen native subtitle test functions pass, including eight rows that keep
the same actual production Connections hooks and estimators alive across Qt and
direct-native seek/rate events. The before control passes the Qt rows but fails
all four direct-native rows. No timing adjustment is published on cancellation.

The controller now announces native seek starts and paused rate callbacks.
Rate callbacks read the latest actual rate, and direct rate notifications occur
after unlocking. The existing timeline seek-request signal keeps its semantics.
The test uses the actual frozen page hooks, not complete visual page construction;
this native isolation is separate from ordinary modal acceptance. The after test
also strengthens settled setup and actual-result assertions; both sources remain.

The ordinary player starts paused at half speed with the FIRST cue. Selecting
2x and pressing Escape returns to the track list with that rate retained. A paused
seek to 30 seconds shows MIDDLE, resume/re-pause at 54 seconds shows LAST, and Next
opens generated ASS media with a visible FIRST cue when paused around 18 seconds.
Normal Quit exits zero, guards remain unchanged, 602 modules load, and diagnostics
are empty. Only generated media was used.

[checkpoint.json](checkpoint.json) binds exact identities and original gzip
artifacts. Earlier reset and hardware-snapshot receipts retain their distinct
ordinary journeys. This does not close W1, W2 or the beta gates. Continue from
[STATUS](../../../juxtopposed/STATUS.md).
