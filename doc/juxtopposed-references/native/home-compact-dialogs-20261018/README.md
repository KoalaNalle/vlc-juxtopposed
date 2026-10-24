# Home navigation and modal interaction checkpoint — 18 October 2026

Later setup continuation: the Shift+F10 failure described below has a scoped
repair. Qt's Windows key mapper suppresses Shift-only F10 KeyPress delivery when
no exact matching shortcut is registered. A WindowShortcut enabled only on the
focused visible pin resolves this without changing the SDK. Ordinary setup R2
passes actual Windows-input Shift+F10 and Menu reopening, Escape, plain F10
menubar toggling and the off-pin negative. The isolated window closed normally;
the original profile and pin are preserved. See [native-keyboard-repair.json](native-keyboard-repair.json).
The historical user physical R10 failure remains intact; no new user physical
retest or complete accessibility/DPI/visual/package acceptance is claimed.

The four existing modals pass twelve actual Windows VLC journeys at interface
scales 1, 1.5 and 2: header pointer dragging, resize clamping, close and Escape,
opener-focus restoration and fresh-open centering. The paused primary input,
clock, position, volume and mute remain unchanged. All 508 checks pass; the
process exits normally with zero Qt diagnostics on monitor 2. The 24 captures
and exact source/binary identities are recorded in [verification.json](verification.json).

These are QtTest events in the actual native app. Short-window dimensions honor
the app's current minimum size; capture dimensions are recorded rather than
assuming every requested size was reached. Interface scaling is not actual
Windows DPI transition acceptance. Complete visual fidelity remains open.

The compact journey reproduced two navigation defects: Tab could focus a route
below the visible viewport, and wheel input over drawer padding could change
player volume. The R5 repair keeps focused controls visible and handles wheel
input over the whole pane, including pixel/horizontal/end-of-list events.
Both native video/audio matrices pass: eight size/scale cases, 1,483 checks,
normal exits and zero Qt diagnostics. Actual minimum-constrained sizes are in
the receipt. The accepted R5 source is retained separately. Earlier rejected
matrices, including the empty-list `forceLayout()` test-setup mistake, remain
preserved and are not acceptance results.

A subsequent component probe found that error-state arrows tried to focus the
disabled Add action, while Retry/Dismiss left focus on hidden buttons. Current
source adds enabled error actions to traversal and restores an available footer
action after recovery. Its component suite passes 37 checks. R6 repeats both
compact video/audio matrices successfully. The real owned-INI sharing failure
preserves the exact stored data; keyboard Retry commits both expected pins and
restores Add focus. Reopening a pin context menu then fails, so native Dismiss
and last-pin fallback acceptance remain open. R7 confirms no pin-menu request
signal despite the active, focused target. A separate visible check reproduces
the keyboard problem after closing a mouse-opened menu. No cause is accepted yet.

Actual conversion busy/cancel journeys pass while the primary media is paused
(258 checks, R6) and playing (262 checks, R7). The conversion input really starts,
competing modals remain blocked, dragging works, cancellation leaves no final
output or temporary stage, and primary playback/audio state is preserved.

An ordinary R6 GUI was rebuilt without private test hooks. Five ordinary smoke
cases pass: redesigned playback, traditional playback, Library playback, generated
live HLS, and a read-only D:\Media sample. Processes exit normally without QML or
late Qt diagnostics. These are interim runtime checks, not final package, visual
or DPI acceptance. Remaining workflows and release gates stay open in the
[roadmap](../../../juxtopposed-beta-roadmap.md). No commit, push, release,
dependency installation or security-setting change occurred.

R8 then confirmed that a pin can retain Mouse focus without `visualFocus` after
a native popup closes. The global hotkey filter took its menu/navigation keys.
R9 adds an explicit sidebar scope for those keys and preserves unrelated
shortcuts. The exact production routing fixture passes eight cases. Four native
R9 journeys pass (recovery 316, compact video 741, compact audio 742 and playing
conversion 262 checks), each with a normal exit and no Qt diagnostics.

Those passes are scoped: capture review found that after empty-store Dismiss,
Add retained focus at y276–308 in a 186px viewport. The native assertion had
checked focus but not visibility. R10 rechecks focus after footer geometry settles
and uses sidebar colors for the recovery buttons. Its component suite passes
47 checks; the exact R9 QML preimage fails the new post-Dismiss visibility check.
Native replay is pending. An ordinary R9 Home menu also did not visibly reopen
during Computer Use; a native Home case with no synthetic refocusing is pending
to distinguish app behavior from tool activation/capture timing. Full recovery,
ordinary interaction and visual acceptance remain open until those checks finish.

R10 native replay now passes: Home menu with Library enabled and disabled (185
checks each), error recovery with actual post-layout bounds (319), compact video
(741), compact audio (742), and playing conversion (262). All six processes exit
normally with no Qt diagnostics. Reviewed captures show the corrected empty Add
viewport and dark recovery-button styling. These pass in-process QtTest input.

The ordinary R10 GUI contains no private hooks and passes all five playback
smokes (finite redesigned/traditional/Library, generated live HLS and read-only
D:\Media). Computer Use also visibly opens the pin menu with the dedicated Menu
key. Shift+F10 still differs from in-process delivery, with both documented Shift
aliases and a separate settled capture checked. The user subsequently repeated
right-click, Escape and Shift+F10 and confirmed that the menu does **not** reopen;
see the [manual receipt](physical-keyboard-comparison.json). This is an open
native-input defect; its cause is not yet established. Keyboard-restored focus outlines
and the full accessibility/DPI/visual pass remain open. Full beta acceptance is
false and work continues through the roadmap.

The original local video also confirms the Movies/Shows/Music/Work/New submenu
at 19:35 and the theme/accent setup screen at 19:50. Exact-time extracts, source
identities and cached Figma geometry are retained privately for the next workflow
batch. No additional Figma reads were used. The 19:35 frame does not establish a
complete Customize Player form.
