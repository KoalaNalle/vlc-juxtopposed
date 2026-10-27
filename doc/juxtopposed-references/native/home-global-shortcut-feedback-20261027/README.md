# Windows global shortcut registration feedback

27 October 2026. Source `a6cd73445c3d195ba930e28fa036c691158acb34`. The Windows backend now reports which
global bindings actually registered, including a bounded list of failed keys.
Controls and the native shortcut editor show that running result separately from
saved mappings. A collision identifies the action/key and explains the restart
requirement. Search for Global Shortcuts retains the status and Edit action.
Unsupported Meta/Command modifiers and reserved F12 are reported explicitly.
Every successful global-atom reference is balanced during shutdown, including
multiple bindings of one action. The transient report belongs to one VLC root;
it does not save preferences or claim a new mapping is already active.

## Evidence

- Before r431: the previous plugin lacks the versioned report and the negative
  test fails during initialization as expected. No later scenarios ran.
- Final real Windows module test r437: six functions including initialization/
  cleanup pass, zero failures/skips. It checks inactive defaults, active/partial
  registration, same-chord error1409, another independent binding, release and
  reacquisition, owned atom balancing, explicit unsupported keys and the32-row
  error bound. It loads the real module without synthetic key delivery.
- Native Controls model r440: twenty functions pass. Supplied-report seams check
  status parsing, bounds, malformed/future input, warnings and unchanged running
  feedback after saving a replacement. This consumer seam is distinct from the
  real Windows registration test. Maintained QML r438 passes nine groups,
  including searchable read-only feedback without a preference write.
- Ordinary GUI441 `3f6170f7`,
  Windows-hotkeys432 `33c21ffe`,
  Core265 and the identified existing tuple run without private hooks.
  Player A owns Ctrl+Alt+Shift+F8/F9; B reports0active/2unavailable and names both
  conflicts in Controls and the editor. With B focused, F8 starts A and F9 pauses
  A; B remains paused at zero. A's held position remains
  `0.005539296111111112` during B's editing and until A exits normally.
- B saves Ctrl+Alt+Shift+F10 through the real editor. The saved/restart message
  appears, the running F8/F9 failures remain, and F10 has no pre-restart effect.
  Closing A does not silently activate B's rejected bindings. After normal B
  exit and a cold start with its saved configuration, the status is1active,
  the editor contains F10, and F10 starts/pauses playback. The final held position
  is `0.003674748888888889`. Search retains the feedback and Edit action.
- The three long-fixture runs exit0 with602plugins, zero checked errors and all
  source/runtime/base/fixture/producer/cache ending guards. Only B's
  `global-key-play-pause` native config key changes; the complete player profile
  catalogues remain identical across all three runs. No private media is included.

## Preserved limitations and preliminary outcomes

The first positive test r433 assumed registration had finished as soon as the
report existed. It observed starting; the corrected harness waits for a ready
state. r435 skipped atom balancing because another owner already had a play-pause
atom; the final test uses an initially absent stop atom and does not touch another
owner's reference. The preliminary ordinary r442/r443 runs exit normally with all
guards, but the60-second fixture reached EOF during inspection. They do not
establish the intended key journey. The repeat uses a generated one-hour silent
WAV with its own identity and ending guard.

Immediate and occluded UI captures can lag actual playback; assertions use settled
foreground observations. Native-modal element indexes were sometimes unavailable;
fresh screenshots and visible controls completed the journey. r447 observation06
shows General despite its early filename; observation07 establishes the active
count. Editor Escape was inconclusive through automation; explicit Cancel worked.
UIA still reports a stale focused element, so these captures do not establish
screen-reader/focus acceptance. No OS DPI or complete visual score is claimed.

This closes the scoped registration/conflict feedback gap, not every shortcut or
S32. Audit the legacy mapping of other keys/punctuation and remaining action/hint
paths separately. Actual Windows DPI, themes, accessibility, remaining instance
combinations, integrated regression, independent review and clean-package/beta
gates remain open. Older tested runtimes remain recoverable.

No firewall rules, security/privacy or Defender settings changed. The API basis is
Microsoft's [RegisterHotKey](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-registerhotkey),
[UnregisterHotKey](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-unregisterhotkey)
and [system error codes](https://learn.microsoft.com/en-us/windows/win32/debug/system-error-codes--1300-1699-).
`checkpoint.json` binds source, binaries, fixtures and compressed evidence with
SHA-256; every artifact was decompressed and verified. Binaries, media and isolated
profiles remain in ignored build output. This receipt is a frozen scoped record.
