# Windows global shortcut key translation

27 October 2026. Source `546eecd8e1957cffc967aa7b0f8476463f330867`. This follows the
[registration feedback receipt](../home-global-shortcut-feedback-20261027/README.md).
The legacy byte fallback could report success while registering a different key:
it interpreted punctuation as a Windows virtual-key number and truncated other
characters. The repaired module translates representable BMP characters through
the registration thread's startup keyboard layout and includes required Shift/
Ctrl/Alt modifiers. Unrepresentable characters, supplementary Unicode, surrogates
and unmapped special keys remain explicitly unavailable. ASCII letter matching
keeps the existing case-insensitive behavior. Explicit Windows mappings now cover
F13–F24, Menu/Print/Pause, browser keys and Volume Mute as well as existing keys.
Reserved F12 and unsupported Meta/Command remain unavailable.

Before r451 reproduces both defects: while the actual semicolon chord is reserved,
the old module reports active; U+10441 also reports active after its low byte
collapses to A. Both negative cases fail as intended, exit2, with all ending guards.
After r457, ten test functions pass with17pass incidents,
zero failures/skips and exit0/all guards. The expanded test retains collision,
release/reacquisition, atom-reference cleanup and bounded-report checks. New cases
exercise punctuation, required layout modifiers, nontruncation and eight extended
key rows against actual Windows registration collisions. These tests reserve
chords without injecting keys or changing OS settings.

The first expanded positive run r453 passed the two repairs and seven extended
rows but could not reserve Ctrl+Alt+Shift+Tab. The final test accepts an existing
Windows error1409 as a real collision and then requires the module to report that
same collision; it does not skip the row or remove the existing owner's binding.
An intermediate r454 test build finished, then the implicit-modifier case was
added before its planned r455 run; r455 was not launched. The final run is r457.

Ordinary GUI441 with Windows-hotkeys452
`28e53fd1` uses the
identified existing Core265 tuple without private hooks. With a saved
Ctrl+Alt+Shift+semicolon Play/Pause mapping, the physical Finnish-layout
Ctrl+Alt+Shift+comma chord starts and pauses the one-hour generated WAV. Controls
shows one active global shortcut; the native editor names the semicolon mapping.
Its paused position remains `0.002681589444444444` across opening, inspection and Cancel.
The player exits0 with602plugins, zero checked errors and all source/runtime/base/
fixture/producer/cache guards. Native config and the complete profile catalogue
stay unchanged. The older GUI441 two-player/restart receipt retains its own tuple;
this module change does not silently replace its exact-binary evidence.

Immediate accessibility snapshots can lag the screen; settled observations07–09
establish the final status and retained clock. The startup layout is used until
restart. Live layout changes, other layouts, every shortcut/action, actual DPI,
screen-reader focus, full visual scoring, integrated regression, independent
review and clean-package/beta acceptance remain open. The setup tour was inspected:
its displayed bindings use local key-* mappings, not failed global-key-* bindings.
No new hint code was needed. No firewall/security/privacy/Defender settings changed.

Microsoft documents the character/layout/modifier result in
[VkKeyScanExW](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-vkkeyscanexw),
the actual key identifiers in
[Virtual-Key Codes](https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes),
and collision behavior in
[RegisterHotKey](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-registerhotkey).
`checkpoint.json` binds compressed evidence to original and compressed SHA-256;
decompression was verified. Generated media/binaries and isolated profiles remain
ignored. This is a frozen scoped record, not full S32 or beta acceptance.
