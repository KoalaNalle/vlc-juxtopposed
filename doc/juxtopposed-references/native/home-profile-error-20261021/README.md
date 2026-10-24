# Profile conflict recovery-message visibility

21 October 2026. The selected component, rebuilt Windows and ordinary two-window
checks pass. The complete beta remains open.

The [ordinary shared-profile journey](../home-shared-qt-20261021/ordinary-profiles-r1.json)
uses three normal VLC processes: A creates a profile, stale B's competing Create
is rejected, B restarts and successfully creates its profile, then A and B2 quit.
The catalogue remains5/5/5 entries through rejection/B exit and6/6/6 through B2's
create and the final exits. A remains fullscreen and playing through the peer's
restart and retains its local profile selection. This qualifies that selected
functional conflict/restart path on GUI `aa6fc26df153…`, Core `d04e9134a43f…`.
It does not qualify profile-default switching, arbitrary recovery or all outputs.

The journey exposed a P2: the final line of the complete native conflict message
was clipped in the form until the user scrolled. Its accessible text was intact.
The existing ScrollView reveal helper now brings a new error into view after
layout settles, including wrapping/resize and opening with a retained error.
The entered name and focus ownership are preserved; the authored popup size and
buttons are unchanged.

The final component test passes30 checks, including the full recovery-message
bounds at standard size and compact125% in Dark and a supplied native Light
palette. The same final test on the preserved dialog preimage fails: the error
bottom is213px in a202px viewport. Earlier component R1 also passed geometry but
incorrectly changed only a preference value for its Light label; its palette
remained Dark. That intermediate result is not Light evidence. Final R2 supplies
`nativePalette.isDark` and verifies the actual Light state before capture.

The component service is supplied, not a native settings substitute. Its captures
do not establish OS-DPI, arbitrary translation/scale, native desktop or Figma
fidelity acceptance. Very small viewports can require scrolling within a message.
The compiled private native fixture passes30 checks on GUI `373271e3cb58…`,
Core `d04e9134a43f…`, cache `1190b018b5e8…`. The actual MainCtx saves a seed;
an independent production store commits a competing catalogue; the actual form's
Create then produces the native conflict. The complete error fits at1280×900
and650×480/125% app scale in native Dark and Light. The draft, local selection
and accepted catalogue survive rejection and actual Cancel. Native captures
were visually inspected. This idle fixture does not qualify decoded playback,
physical OS scaling or every profile failure.

Ordinary GUI `b27d70c7f249…` uses the same Core/cache and frozen1266 inputs.
Two ordinary VLC processes share one owned Qt INI and native config while playing
separate silent WAVs on two monitors. A creates Monitor Movies; stale B's
Monitor Music draft is rejected. The full two-line instruction is immediately
visible without manual scrolling, with the name retained. After Cancel and
reopening New, the retained error is still fully visible, the new name is empty
and focused, and Create is disabled. Cancel does not modify the catalogue.
B quits normally; A keeps playing, enters fullscreen and quits directly.
Both logs are clean, each loads one plugin cache and opens WASAPI; native Stop
acknowledgment precedes Qt teardown. The catalogue hash is identical after
A's create, B's rejection, B's exit and A's exit (five profiles throughout).
Runtime/source/media identities remain unchanged and no forced cleanup is used.

This closes the selected clipped-conflict-message defect, including reopening
with a retained error. The older three-process restart journey remains scoped
to its earlier GUI and retains its historical negative visual result. Current
ordinary acceptance is standard Dark; native standard/compact Dark/Light checks
are separate. Two distinct physical audio outputs, arbitrary same-key Qt
transactions, all profile-default application and full visual/beta gates remain.
Frozen inputs, preimage, drivers and raw logs live under
`build-windows/beta-profile-error-20261021`.
