# Native folder picker and editor-focus checkpoint

14 October 2026. This records actual Windows desktop checks after the prior Firewall prompt disappeared. It advances the full slice's native gates without claiming the complete Home/media journey or visual acceptance.

## Reproduced faults and repairs

Home's Add Folder crashed before the picker appeared. The saved dump (`vlc.exe.15952.dmp`, SHA-256 `952b2d5b0e47b85ee43490ac85808b6f41351645132bccb15047b426a1209619`) reports access violation `c0000005`. CDB and the matching MinGW symbols resolve the path to QWidget::setParent → QWidgetPrivate::init → QDialog/QFileDialog construction → DialogsProvider::addMediaLibraryFolder at line 790. The native dialog received the unused, never-initialized `QWidget* root` member. Removing that member and passing `nullptr`, as the existing directory-opening route does, repairs construction.

The shared library header also lacked global shortcut suppression for its editor. A new TextInputFocus component observes the window's actual editable TextInput/TextEdit focus. DialogsProvider publishes that state and ShortcutExt respects it independently of the legacy writable shortcut flag and the modal guard. Leaving the editor automatically restores application shortcuts. This protects library search and other QML text fields from application Paste/Save actions.

The production component regression constructs real TextField/TextArea controls, the actual focus helper and ShortcutExt. Ctrl+V pastes into each editor without invoking the application action; after moving to a button, the action runs again. All ten component suites pass locally. The existing modal/busy/error/reset/keyboard/scaling checks also pass.

## Actual desktop evidence

- The repaired native picker opens. Selecting the generated fixture directory calls actual library registration and displays two videos and one audio item on Home. [Indexed Home](desktop/indexed-home.png) uses plugin `9fb7f7c8a908c2f6af76590f777c67e041eceb734ee97319dd7343b4743a84a4`.
- Normal shutdown completes, and a new process against the same test library retains those items.
- In the final plugin, Videos displays both real items. Direct mouse focus and pasting `chaptered` filters to one actual result; Ctrl+A/Backspace clears it and restores both. [Filtered Videos](desktop/filtered-videos.png) retains the real header/count/result.
- Activating that filtered result opens the actual chaptered video and renders its generated colors. Sending Space early holds a red frame. [Video capture](desktop/held-video.png) establishes actual rendering; it does not prove a numeric frozen clock or successful control-panel rendering.
- Stop returns to Videos. The complete user-facing Back/return/resume path remains unverified.

The [verification record](desktop/verification.json) retains per-build scope. Native accessibility clicks did not reliably focus text fields; coordinate mouse focus was required. The GUI's media-library userdata is isolated, while portable configuration/Qt settings are retained. The user's working package is preserved. No security permission choices were made.

The final Qt plugin SHA-256 is **`2afa0383d817d90987977b33291fb5b0740e6c956021198b281cbd833c804015`**. WSL Ubuntu 24.04 / MinGW x64 POSIX/MSVCRT / Qt 6.8.3 incremental builds complete successfully. The plugin is staged in `build-windows/slice-package`; rollback files are `libqt_plugin.dll.before-folder-picker-20261014` and `libqt_plugin.dll.before-editor-focus-20261014`.

## Open gates

The video snapshots show no playback bars. The real FSM/fade components pass an independent probe; default autohide timing, native input delivery, actual dimensions and compositor behavior still need diagnosis. A pinned/long-timeout variant can distinguish visibility timing from rendering, but must not substitute for acceptance of the normal floating controls. Full Back/return/resume, chapters/bookmarks, settings/media-switch/focus, narrow/DPI and original whole-window fidelity remain open. The Home/music/preset implementations are separately prepared and have not yet received native integration/build acceptance.

The stronger [Home audit](../home/reference-notes.md) confirms five sections and sidebar pins. Native folder/search/playback results above do not finish that dashboard's design. Continue the complete milestones 0–8 plan and keep the PR draft.
